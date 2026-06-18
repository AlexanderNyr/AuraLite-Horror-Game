#include "audio.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

AudioSystem audio;

// Helper to get random float between -1 and 1
static float randNoise() {
    return (float)std::rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

void SDLCALL AudioSystemCallback(void* userdata, Uint8* stream, int len) {
    AudioSystem* sys = (AudioSystem*)userdata;
    sys->audioCallback(stream, len);
}

void AudioSystem::init() {
    SDL_AudioSpec want, have;
    std::memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1; // Mono is perfectly atmospheric and robust for our procedural synth
    want.samples = 1024;
    want.callback = AudioSystemCallback;
    want.userdata = this;

    deviceId = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (deviceId == 0) {
        std::cerr << "Warning: Failed to open SDL audio device: " << SDL_GetError() << std::endl;
        return;
    }

    initialized = true;
    SDL_PauseAudioDevice(deviceId, 0);
}

void AudioSystem::update(float fogDensity, int day, bool moving, bool sprinting, float stamina, float fatigue) {
    if (!initialized) return;

    std::lock_guard<std::mutex> lock(audioMutex);
    targetFogDensity = fogDensity;
    currentDay = day;
    isMoving = moving;
    isSprinting = sprinting;
    playerStamina = stamina;
    playerFatigue = fatigue;
}

void AudioSystem::playSound(const std::string& name) {
    if (!initialized) return;

    std::lock_guard<std::mutex> lock(audioMutex);
    ActiveSound s;
    s.type = name;
    s.time = 0.0f;

    if (name == "pickup") s.duration = 0.8f;
    else if (name == "sleep") s.duration = 3.5f;
    else if (name == "repair") s.duration = 1.5f;
    else if (name == "intro") s.duration = 2.5f;
    else s.duration = 1.0f;

    activeSounds.push_back(s);
}

void AudioSystem::audioCallback(Uint8* stream, int len) {
    std::lock_guard<std::mutex> lock(audioMutex);

    int samples = len / sizeof(Sint16);
    Sint16* out = (Sint16*)stream;
    float dt = 1.0f / 44100.0f;

    for (int i = 0; i < samples; ++i) {
        timeMaster += dt;

        // 1. Spooky Wind / Fog Drone Synthesis
        // Wind pitch & intensity scales with fog density and day
        windLfo += dt * (0.2f + targetFogDensity * 0.5f);
        float lfoVal = std::sin(windLfo * 2.0f * M_PI) * 0.5f + 0.5f;

        // Generate raw noise and pass through a low-pass/band-pass resonant filter
        float rawNoise = randNoise();
        float cutoff = 150.0f + targetFogDensity * 250.0f + lfoVal * 150.0f;
        float w = 2.0f * M_PI * cutoff * dt;
        
        // Simple 2-pole resonant filter
        float damping = 0.4f;
        windFilterVel += (rawNoise - windFilterPos) * w;
        windFilterVel *= (1.0f - damping * w);
        windFilterPos += windFilterVel;

        // Wind drone volume
        float windDrone = windFilterPos * (0.3f + targetFogDensity * 0.5f);

        // Add a low eerie hum for later days
        windPhase += dt * (65.0f + currentDay * 3.0f);
        float lowHum = std::sin(windPhase * 2.0f * M_PI) * (0.05f * currentDay);

        // 2. Footsteps
        float footstepVol = 0.0f;
        if (isMoving) {
            float stepInterval = isSprinting ? 0.32f : 0.55f;
            footstepTimer += dt;
            if (footstepTimer >= stepInterval) {
                footstepTimer = 0.0f;
                footstepEnv = 1.0f;
            }
        } else {
            footstepTimer = 0.0f;
        }

        if (footstepEnv > 0.0f) {
            footstepEnv -= dt * 15.0f; // Fast decay
            if (footstepEnv < 0.0f) footstepEnv = 0.0f;
            
            // Crunch/rustle sound for footsteps
            float rustle = randNoise() * footstepEnv * footstepEnv;
            footstepVol = rustle * (isSprinting ? 0.25f : 0.15f);
        }

        // 3. Heartbeat (when fatigued or out of stamina)
        float heartbeatVol = 0.0f;
        bool pulseActive = (playerStamina < 0.35f || playerFatigue > 0.4f);
        if (pulseActive) {
            float hbInterval = playerStamina < 0.2f ? 0.6f : 0.9f;
            heartbeatTimer += dt;
            if (heartbeatTimer >= hbInterval) {
                heartbeatTimer = 0.0f;
            }

            // Two subtle thuds: at t=0.0 and t=0.2
            float thud1 = std::max(0.0f, 1.0f - heartbeatTimer * 12.0f);
            float thud2 = std::max(0.0f, 1.0f - std::abs(heartbeatTimer - 0.22f) * 12.0f);
            float env = thud1 + thud2;

            // Low 60Hz pulse
            heartbeatVol = std::sin(heartbeatTimer * 60.0f * 2.0f * M_PI) * env * 0.4f;
        }

        // 4. One-shot sound effects
        float sfxVol = 0.0f;
        for (auto it = activeSounds.begin(); it != activeSounds.end();) {
            it->time += dt;
            if (it->time >= it->duration) {
                it = activeSounds.erase(it);
                continue;
            }

            float progress = it->time / it->duration;
            float remain = 1.0f - progress;

            if (it->type == "pickup") {
                // Bright wooden chime / click
                float wave = std::sin(it->time * 880.0f * 2.0f * M_PI) + 
                             std::sin(it->time * 1200.0f * 2.0f * M_PI);
                sfxVol += wave * remain * remain * 0.3f;
            } else if (it->type == "sleep") {
                // Deep mysterious lingering gong
                float wave = std::sin(it->time * 110.0f * 2.0f * M_PI) + 
                             std::sin(it->time * 220.0f * 2.0f * M_PI);
                sfxVol += wave * remain * 0.4f;
            } else if (it->type == "repair") {
                // Stone grinding thump
                float noise = randNoise() * 0.5f;
                float low = std::sin(it->time * 80.0f * 2.0f * M_PI);
                sfxVol += (noise + low) * remain * 0.35f;
            } else if (it->type == "intro") {
                // Eerie wind swell
                float wave = std::sin(it->time * 140.0f * 2.0f * M_PI) * randNoise();
                sfxVol += wave * remain * 0.25f;
            }

            ++it;
        }

        // Combine all audio streams
        float finalSample = windDrone + lowHum + footstepVol + heartbeatVol + sfxVol;

        // Hard clip to keep within Sint16 range (-32768 to 32767)
        if (finalSample > 1.0f) finalSample = 1.0f;
        if (finalSample < -1.0f) finalSample = -1.0f;

        out[i] = (Sint16)(finalSample * 30000.0f);
    }
}

void AudioSystem::cleanup() {
    if (deviceId != 0) {
        SDL_CloseAudioDevice(deviceId);
        deviceId = 0;
    }
    initialized = false;
}
