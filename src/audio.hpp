#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <mutex>

struct ActiveSound {
    std::string type;
    float time = 0.0f;
    float duration = 1.0f;
};

class AudioSystem {
public:
    void init();
    void update(float fogDensity, int currentDay, bool isMoving, bool isSprinting, float stamina, float fatigue);
    void playSound(const std::string& name);
    void cleanup();

    // Callback used by SDL
    void audioCallback(Uint8* stream, int len);

private:
    SDL_AudioDeviceID deviceId = 0;
    bool initialized = false;

    // Synthesis states
    float timeMaster = 0.0f;
    float windPhase = 0.0f;
    float windLfo = 0.0f;
    float windFilterPos = 0.0f;
    float windFilterVel = 0.0f;

    // Game state parameters passed to synth
    float targetFogDensity = 0.0f;
    int currentDay = 1;
    bool isMoving = false;
    bool isSprinting = false;
    float playerStamina = 1.0f;
    float playerFatigue = 0.0f;

    // Footsteps
    float footstepTimer = 0.0f;
    float footstepEnv = 0.0f;

    // Heartbeat
    float heartbeatTimer = 0.0f;
    float heartbeatEnv = 0.0f;

    // Sound effects
    std::vector<ActiveSound> activeSounds;
    std::mutex audioMutex;
};

// Global audio instance
extern AudioSystem audio;
