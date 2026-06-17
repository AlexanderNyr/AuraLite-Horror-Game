#include "game.hpp"
#include "procedural.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "HorrorGame", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "HorrorGame", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#define LOGE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#endif

// Custom clamp helper
static float clamp(float val, float minVal, float maxVal) {
    return val < minVal ? minVal : (val > maxVal ? maxVal : val);
}

// Simple LCG pseudo-random generator for identical forest layout
static uint32_t lcg_seed = 987654321;
static float randFloat() {
    lcg_seed = (lcg_seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)lcg_seed / (float)0x7fffffff;
}

void Game::init(int width, int height, bool mobileMode) {
    screenWidth = width;
    screenHeight = height;
    isAndroid = mobileMode;
    loc.init("lang");

    // Compile 3D shaders
    std::string vert3D = R"glsl(
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;
        layout(location = 3) in vec4 aColor;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;
        out vec4 Color;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            Color = aColor;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )glsl";

    std::string frag3D = R"glsl(
        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;
        in vec4 Color;

        out vec4 FragColor;

        uniform vec3 viewPos;
        uniform vec3 fogColor;
        uniform float fogStart;
        uniform float fogEnd;

        uniform vec3 ambientColor;
        uniform vec3 dirLightColor;
        uniform vec3 dirLightDir;

        // Flashlight (Spotlight)
        uniform vec3 flashPos;
        uniform vec3 flashDir;
        uniform float flashCutoff;
        uniform float flashIntensity;

        uniform sampler2D texture_diffuse;
        uniform int useTexture;

        void main() {
            vec4 baseColor = (useTexture != 0) ? texture(texture_diffuse, TexCoord) * Color : Color;
            if (baseColor.a < 0.1) discard;

            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);

            // Ambient
            vec3 ambient = ambientColor * baseColor.rgb;

            // Directional Light
            vec3 lightDir = normalize(-dirLightDir);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * dirLightColor * baseColor.rgb;

            // Flashlight
            vec3 spotlight = vec3(0.0);
            if (flashIntensity > 0.0) {
                vec3 lightToFrag = normalize(FragPos - flashPos);
                float theta = dot(lightToFrag, normalize(flashDir));
                if (theta > flashCutoff) {
                    float distance = length(FragPos - flashPos);
                    float attenuation = 1.0 / (1.0 + 0.03 * distance + 0.001 * distance * distance);
                    
                    // Soft cone edges
                    float epsilon = 0.04;
                    float intensity = clamp((theta - flashCutoff) / epsilon, 0.0, 1.0) * flashIntensity;

                    vec3 spotLightDir = normalize(flashPos - FragPos);
                    float spotDiff = max(dot(norm, spotLightDir), 0.0);
                    vec3 spotLightColor = vec3(1.0, 0.96, 0.82);

                    spotlight = spotDiff * spotLightColor * baseColor.rgb * attenuation * intensity;
                }
            }

            vec3 finalColor = ambient + diffuse + spotlight;

            // Atmospheric Fog
            float distance = length(viewPos - FragPos);
            float fogFactor = clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);
            
            FragColor = vec4(mix(fogColor, finalColor, fogFactor), baseColor.a);
        }
    )glsl";

    mainShader.compile(vert3D, frag3D);

    // Billboard Shaders
    std::string vertBillboard = R"glsl(
        layout(location = 0) in vec3 aPos;
        layout(location = 2) in vec2 aTexCoord;

        out vec2 TexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            TexCoord = aTexCoord;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )glsl";

    // Procedural Billboard shader for silhouettes and glowing eyes
    std::string fragBillboard = R"glsl(
        in vec2 TexCoord;
        out vec4 FragColor;

        uniform vec3 viewPos;
        uniform vec3 fogColor;
        uniform float fogStart;
        uniform float fogEnd;
        uniform vec3 itemPos;

        uniform int isSilhouette;
        uniform float opacity;

        float distToCapsule(vec2 p, vec2 a, vec2 b, float r) {
            vec2 pa = p - a, ba = b - a;
            float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
            return length(pa - ba * h) - r;
        }

        void main() {
            vec2 uv = TexCoord;
            vec4 pixelColor = vec4(0.0);

            if (isSilhouette != 0) {
                // Procedural Slender-like dark humanoid silhouette
                float dHead = length(uv - vec2(0.5, 0.82)) - 0.07;
                float dTorso = distToCapsule(uv, vec2(0.5, 0.72), vec2(0.5, 0.35), 0.11);
                float dArmL = distToCapsule(uv, vec2(0.39, 0.70), vec2(0.32, 0.32), 0.035);
                float dArmR = distToCapsule(uv, vec2(0.61, 0.70), vec2(0.68, 0.32), 0.035);
                float dLegL = distToCapsule(uv, vec2(0.44, 0.35), vec2(0.41, 0.05), 0.045);
                float dLegR = distToCapsule(uv, vec2(0.56, 0.35), vec2(0.59, 0.05), 0.045);

                float dBody = min(dHead, min(dTorso, min(dArmL, min(dArmR, min(dLegL, dLegR)))));

                if (dBody < 0.0) {
                    pixelColor = vec4(0.02, 0.02, 0.02, opacity);

                    // Add red glowing eyes on head
                    float dEyeL = length(uv - vec2(0.47, 0.83));
                    float dEyeR = length(uv - vec2(0.53, 0.83));
                    if (dEyeL < 0.012 || dEyeR < 0.012) {
                        pixelColor = vec4(1.0, 0.0, 0.0, opacity);
                    }
                } else if (dBody < 0.015) {
                    float alpha = (1.0 - (dBody / 0.015)) * opacity;
                    pixelColor = vec4(0.02, 0.02, 0.02, alpha);
                } else {
                    // Soft glowing ambient eyes bleeding out of dark silhouette
                    float dEyeL = length(uv - vec2(0.47, 0.83));
                    float dEyeR = length(uv - vec2(0.53, 0.83));
                    float glow = exp(-min(dEyeL, dEyeR) * 20.0);
                    if (glow > 0.05) {
                        pixelColor = vec4(1.0, 0.0, 0.0, glow * 0.7 * opacity);
                    } else {
                        discard;
                    }
                }
            } else {
                // Just glowing red eyes in the dark!
                float dEyeL = length(uv - vec2(0.42, 0.5));
                float dEyeR = length(uv - vec2(0.58, 0.5));
                float glowL = exp(-dEyeL * 30.0);
                float glowR = exp(-dEyeR * 30.0);
                float totalGlow = (glowL + glowR) * opacity;

                if (totalGlow < 0.06) discard;
                pixelColor = vec4(1.0, 0.0, 0.0, totalGlow);
            }

            // Apply Atmospheric Fog
            float distance = length(viewPos - itemPos);
            float fogFactor = clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);

            FragColor = vec4(mix(fogColor, pixelColor.rgb, fogFactor), pixelColor.a);
        }
    )glsl";

    billboardShader.compile(vertBillboard, fragBillboard);

    // Create textures
    grassTexture.generateNoise(128, 128, false);
    woodTexture.generateNoise(128, 128, true);

    // Generate meshes
    generateTerrain(terrainMesh, 300.0f, 300.0f, 100, 100);
    terrainMesh.upload();

    generateCabin(cabinMesh);
    cabinMesh.upload();

    generateTree(treeMesh);
    treeMesh.upload();

    // Precompute deterministic forest instances once. Rendering previously regenerated
    // the same pseudo-random positions every frame, which wasted CPU and hid gameplay bugs.
    treePositions.clear();
    treeScales.clear();
    lcg_seed = 12345;
    for (int i = 0; i < 300; ++i) {
        float tx = randFloat() * 190.0f - 95.0f;
        float tz = randFloat() * 190.0f - 95.0f;

        float dCabin = std::sqrt(tx * tx + tz * tz);
        float dWell = std::sqrt((tx + 20.0f) * (tx + 20.0f) + (tz + 50.0f) * (tz + 50.0f));
        float dCar = std::sqrt((tx - 10.0f) * (tx - 10.0f) + (tz - 75.0f) * (tz - 75.0f));
        if (dCabin < 12.0f || dWell < 8.0f || dCar < 8.0f) {
            continue;
        }

        treePositions.push_back(Vec3(tx, getTerrainHeight(tx, tz), tz));
        treeScales.push_back(0.8f + randFloat() * 0.5f);
    }

    generateWell(wellMesh);
    wellMesh.upload();

    generateCar(carMesh);
    carMesh.upload();

    // Create simple quad for Billboard sprites
    billboardQuad.vertices = {
        { Vec3(-1.0f, -1.0f, 0.0f), Vec3(0,0,1), Vec2(0.0f, 0.0f), {1,1,1,1} },
        { Vec3( 1.0f, -1.0f, 0.0f), Vec3(0,0,1), Vec2(1.0f, 0.0f), {1,1,1,1} },
        { Vec3( 1.0f,  1.0f, 0.0f), Vec3(0,0,1), Vec2(1.0f, 1.0f), {1,1,1,1} },
        { Vec3(-1.0f,  1.0f, 0.0f), Vec3(0,0,1), Vec2(0.0f, 1.0f), {1,1,1,1} }
    };
    billboardQuad.indices = {0, 1, 2, 0, 2, 3};
    billboardQuad.upload();

    // Reusable collectible meshes. These used to be generated every frame; keeping
    // them immutable prevents a silent CPU/GPU memory leak and keeps billboards intact.
    float logColor[4] = {0.8f, 0.7f, 0.3f, 1.0f};
    generateBox(logMesh, Vec3(0.3f, 0.3f, 2.0f), Vec3(0, 0, 0), logColor);
    logMesh.upload();

    float pageColor[4] = {0.95f, 0.95f, 0.95f, 1.0f};
    generateBox(pageMesh, Vec3(1.0f, 1.0f, 1.0f), Vec3(0, 0, 0), pageColor);
    pageMesh.upload();

    float crossColor[4] = {0.95f, 0.82f, 0.22f, 1.0f};
    generateBox(crossVerticalMesh, Vec3(0.3f, 3.2f, 0.3f), Vec3(0, 0, 0), crossColor);
    crossVerticalMesh.upload();
    generateBox(crossHorizontalMesh, Vec3(2.0f, 0.3f, 0.3f), Vec3(0, 0.6f, 0), crossColor);
    crossHorizontalMesh.upload();

    // Initialize UI Renderer
    ui.init(screenWidth, screenHeight);
    updateTouchLayout();

    // Initial Spawning & Placement
    spawnEntities();
    setupDaySettings();
    resetPlayer();
}

void Game::resize(int width, int height) {
    screenWidth = std::max(1, width);
    screenHeight = std::max(1, height);
    ui.resize(screenWidth, screenHeight);
    updateTouchLayout();
}

void Game::updateTouchLayout() {
    // Scale touch controls with the actual device/window size instead of hardcoding 1280x720.
    leftRadius = std::max(58.0f, screenHeight * 0.105f);
    rightRadius = std::max(82.0f, screenHeight * 0.16f);

    leftJoyX = screenWidth * 0.13f;
    leftJoyY = screenHeight * 0.78f;
    rightJoyX = screenWidth * 0.86f;
    rightJoyY = screenHeight * 0.78f;

    if (walkTouchId == -1) {
        activeJoyX = leftJoyX;
        activeJoyY = leftJoyY;
    }
    if (lookTouchId == -1) {
        activeCamX = rightJoyX;
        activeCamY = rightJoyY;
    }
}

void Game::resetPlayer() {
    camera.position = Vec3(0.0f, 2.0f, 15.0f); // Spawn right in front of the cabin door
    camera.yaw = -M_PI / 2.0f; // Look facing the door
    camera.pitch = 0.0f;
    camera.updateCameraVectors();

    stamina = 1.0f;
    isSprinting = false;
    flashlightTogglePressed = false;

    logsCollected = 0;
    for (auto& log : woodLogs) log.collected = false;
    diaryPage.collected = false;
    woodenCross.collected = false;
}

void Game::spawnEntities() {
    lcg_seed = 987654321; // fixed layout seed!

    // Reset vectors
    redEyes.clear();
    silhouettes.clear();
    woodLogs.clear();

    // Spawn 15 pairs of Red Eyes scattered in the forest
    for (int i = 0; i < 15; ++i) {
        Monster eye;
        eye.pos.x = randFloat() * 180.0f - 90.0f;
        eye.pos.z = randFloat() * 180.0f - 90.0f;
        
        // Prevent spawning too close to Cabin (0,0)
        float dCabin = std::sqrt(eye.pos.x*eye.pos.x + eye.pos.z*eye.pos.z);
        if (dCabin < 20.0f) {
            eye.pos.x += 25.0f * (eye.pos.x > 0 ? 1 : -1);
            eye.pos.z += 25.0f * (eye.pos.z > 0 ? 1 : -1);
        }

        // Project eye to terrain height
        eye.pos.y = getTerrainHeight(eye.pos.x, eye.pos.z) + 1.6f;
        redEyes.push_back(eye);
    }

    // Spawn 10 Silhouettes around the trees
    for (int i = 0; i < 10; ++i) {
        Monster sil;
        sil.pos.x = randFloat() * 160.0f - 80.0f;
        sil.pos.z = randFloat() * 160.0f - 80.0f;

        float dCabin = std::sqrt(sil.pos.x*sil.pos.x + sil.pos.z*sil.pos.z);
        if (dCabin < 25.0f) {
            sil.pos.x += 30.0f * (sil.pos.x > 0 ? 1 : -1);
            sil.pos.z += 30.0f * (sil.pos.z > 0 ? 1 : -1);
        }

        sil.pos.y = getTerrainHeight(sil.pos.x, sil.pos.z) + 2.0f;
        silhouettes.push_back(sil);
    }

    // Day 3 firewood log piles coordinates (close to cabin)
    woodLogs.push_back({ Vec3(10.0f, 0.4f, -6.0f), false });
    woodLogs.push_back({ Vec3(-12.0f, 0.4f, 10.0f), false });
    woodLogs.push_back({ Vec3(8.0f, 0.4f, 15.0f), false });

    // Day 4 Diary page
    diaryPage.pos = Vec3(-35.0f, 0.3f, -40.0f);
    diaryPage.pos.y = getTerrainHeight(diaryPage.pos.x, diaryPage.pos.z) + 0.1f;

    // Day 5 Sacred Cross position
    woodenCross.pos = Vec3(45.0f, 1.8f, -45.0f);
    woodenCross.pos.y = getTerrainHeight(woodenCross.pos.x, woodenCross.pos.z) + 1.8f;
}

void Game::setupDaySettings() {
    state = STATE_INTRO;
    stateTimer = 0.0f;
    fadeAlpha = 1.0f;
    wellInspected = false;
    fear = 0.0f;
    scareFlashAlpha = 0.0f;
    chaserGraceTimer = 0.0f;
    flashlightBattery = 1.0f;
    for (auto& eye : redEyes) eye.opacity = 1.0f;
    for (auto& sil : silhouettes) sil.opacity = 1.0f;

    if (currentDay == 1) {
        ambientColor = Vec3(0.25f, 0.22f, 0.20f);
        dirLightColor = Vec3(0.95f, 0.80f, 0.65f); // warm late afternoon sun
        dirLightDir = Vec3(0.6f, -0.6f, -0.4f);
        fogColor = Vec3(0.70f, 0.82f, 0.90f); // pleasant atmosphere
        fogStart = 150.0f;
        fogEnd = 400.0f; // very far visibility! (30km in narrative)
        flashlightOn = false;
        flashlightIntensity = 0.0f;

        objectiveText = loc.tr("day1.objective.initial", "Objective: Explore the valley and enjoy the peaceful forest.\nWhen you get tired, enter the cabin (0,0) and use your Bed.");
        diaryText = loc.tr("day1.diary", "DAY 1: REFUGE\n\nI finally made it to the old cabin deep in the wooded valley.\nAway from the chaos of the city, the silence here is therapeutic.\nThe sun is warm, and the air smells of fresh pine needles.\nThis was the right choice. I should rest early tonight...");
    }
    else if (currentDay == 2) {
        ambientColor = Vec3(0.12f, 0.12f, 0.15f);
        dirLightColor = Vec3(0.35f, 0.35f, 0.40f); // cold white sun
        dirLightDir = Vec3(0.2f, -0.9f, -0.1f);
        fogColor = Vec3(0.52f, 0.55f, 0.58f); // white chilly fog
        fogStart = 60.0f;
        fogEnd = 160.0f; // fog starts showing in distance (30km lore)
        flashlightOn = false;
        flashlightIntensity = 0.0f;

        objectiveText = loc.tr("day2.objective.initial", "Objective: Check the old stone well in the northern forest (X:-20, Z:-50).\nReturn to the cabin Bed once inspected.");
        diaryText = loc.tr("day2.diary", "DAY 2: THE WHITE WALL\n\nA dense, chilly fog has rolled into the valley overnight.\nIt feels unusually dead. No birds are singing, and no wind blows.\nI walked out to look at the old stone well today.\nA chilling draft blew up from the deep stone dark.\nI hurried back. It feels safer inside.");
    }
    else if (currentDay == 3) {
        ambientColor = Vec3(0.06f, 0.06f, 0.09f);
        dirLightColor = Vec3(0.12f, 0.12f, 0.16f); // dark grey dreary twilight
        dirLightDir = Vec3(-0.1f, -0.9f, -0.1f);
        fogColor = Vec3(0.32f, 0.34f, 0.38f); // dark dense mist
        fogStart = 30.0f;
        fogEnd = 80.0f; // fog closing in (15km lore)
        flashlightOn = false;
        flashlightIntensity = 0.0f;

        objectiveText = loc.tr("day3.objective.initial", "Objective: Collect 3 logs around the cabin woodpiles for fireplace.");
        diaryText = loc.tr("day3.diary", "DAY 3: CONFINEMENT\n\nThe fog has become a suffocating, physical wall.\nI cannot see past the clearing of the cabin anymore.\nI feel an immense pressure in my head, like a static buzz.\nToday, while walking, I swear I heard the sound of snapping branches\nmimicking my own footsteps. I locked the heavy front door.");
    }
    else if (currentDay == 4) {
        ambientColor = Vec3(0.015f, 0.015f, 0.025f);
        dirLightColor = Vec3(0.0f, 0.0f, 0.0f); // pitch black night
        dirLightDir = Vec3(0.0f, -1.0f, 0.0f);
        fogColor = Vec3(0.02f, 0.025f, 0.05f); // dark midnight fog
        fogStart = 15.0f;
        fogEnd = 40.0f; // extremely close visibility (10km lore)
        flashlightOn = true;
        flashlightIntensity = 1.0f;
        flashlightBattery = 1.0f;

        objectiveText = loc.tr("day4.objective.initial", "Objective: Find the dropped diary page in the western woods (X:-35, Z:-40).\nUse F / LIGHT to manage flashlight battery. RUN BACK once collected!");
        diaryText = loc.tr("day4.diary", "DAY 4: RED EYES\n\nI am not alone in these woods.\nThey are hiding in the fog, standing behind the pines.\nWhen I point my flashlight, I see pairs of glowing red eyes...\nThey do not move. They just stand still in the dark. Staring.\nI found a note in the clearing. Someone else was here.\nI am so scared.");
    }
    else if (currentDay == 5) {
        ambientColor = Vec3(0.01f, 0.0f, 0.0f); // blood red ambient tint
        dirLightColor = Vec3(0.0f, 0.0f, 0.0f);
        dirLightDir = Vec3(0.0f, -1.0f, 0.0f);
        fogColor = Vec3(0.10f, 0.01f, 0.01f); // terrifying blood-red fog!
        fogStart = 8.0f;
        fogEnd = 24.0f; // visibility is extremely bad (5km lore)
        flashlightOn = true;
        flashlightIntensity = 0.9f;
        flashlightBattery = 0.65f;

        objectiveText = loc.tr("day5.objective.initial", "Objective: Find the Holy Cross in the sacred grove (X:45, Z:-45)!\nYour flashlight battery is dying. Escape back to the Cabin!");
        diaryText = loc.tr("day5.diary", "DAY 5: THE COLD SCREAM\n\nThey are scratching at the cabin walls. Whispering my name.\nThe fog is leaking through the wooden planks, smelling of blood.\nMy flashlight is dying. It keeps flickering.\nI found a note mentioning a wooden cross erected in the east.\nI must find it. There is no other way to hold them back...");
    }
    else if (currentDay == 6) {
        ambientColor = Vec3(0.002f, 0.002f, 0.004f);
        dirLightColor = Vec3(0.0f, 0.0f, 0.0f);
        dirLightDir = Vec3(0.0f, -1.0f, 0.0f);
        fogColor = Vec3(0.005f, 0.005f, 0.008f); // pure charcoal black (1.5km lore)
        fogStart = 3.0f;
        fogEnd = 12.0f; // absolute zero visibility!
        flashlightOn = false; // Flashlight is DEAD!
        flashlightIntensity = 0.0f;

        objectiveText = loc.tr("day6.objective.initial", "Objective: ESCAPE! RUN TO YOUR CAR PARKED AT THE ROAD END (X:10, Z:75)!");
        diaryText = loc.tr("day6.diary", "DAY 6: NO ESCAPE\n\nMy flashlight is dead. Complete darkness.\nI can hear heavy panting and grinding teeth right behind my neck.\nIt's actively tracking me. I can feel its cold breath.\nMy car is parked on the northern lane.\nI have to run. I have to make it. RUN. RUN. RUN.");

        // Spawn chaser far away initially
        chaser.pos = Vec3(0.0f, 1.8f, -20.0f);
        chaser.active = true;
        chaser.opacity = 1.0f;
        chaserGraceTimer = 3.0f;
    }
}

void Game::handleEvent(void* sdlEvent) {
    SDL_Event* event = (SDL_Event*)sdlEvent;

    if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.scancode < 512) {
            keys[event->key.keysym.scancode] = true;
        }
        if ((event->key.keysym.sym == SDLK_e || event->key.keysym.sym == SDLK_RETURN) && event->key.repeat == 0) {
            actionPressed = true;
        }
        if (event->key.keysym.sym == SDLK_f && event->key.repeat == 0) {
            flashlightTogglePressed = true;
        }
        if (event->key.keysym.sym == SDLK_F2 && event->key.repeat == 0) {
            languageCyclePressed = true;
        }
    }
    else if (event->type == SDL_KEYUP) {
        if (event->key.keysym.scancode < 512) {
            keys[event->key.keysym.scancode] = false;
        }
    }
    else if (event->type == SDL_MOUSEMOTION) {
        if (state == STATE_GAMEPLAY && !isAndroid) {
            // Mouse look
            camera.processMouseMovement((float)event->motion.xrel, -(float)event->motion.yrel);
        }
    }
    else if (event->type == SDL_FINGERDOWN) {
        // Handle touch input for Android
        float fx = event->tfinger.x * screenWidth;
        float fy = event->tfinger.y * screenHeight;
        long long touchId = event->tfinger.fingerId;

        // Flashlight toggle button (upper-right, Days 4-5)
        if (fx > screenWidth - 150.0f && fx < screenWidth - 30.0f &&
            fy > 95.0f && fy < 215.0f) {
            flashlightTogglePressed = true;
        }
        // Check for ACTION/INTERACT button (lower-right area)
        else if (fx > screenWidth - 170.0f && fx < screenWidth - 50.0f &&
            fy > screenHeight - 330.0f && fy < screenHeight - 210.0f) {
            actionPressed = true;
        }
        // Left half: movement joystick
        else if (fx < screenWidth * 0.5f) {
            walkTouchId = touchId;
            activeJoyX = fx;
            activeJoyY = fy;
        }
        // Right half: camera look drag area
        else {
            lookTouchId = touchId;
            lastTouchCamX = fx;
            lastTouchCamY = fy;
            activeCamX = fx;
            activeCamY = fy;
        }
    }
    else if (event->type == SDL_FINGERMOTION) {
        float fx = event->tfinger.x * screenWidth;
        float fy = event->tfinger.y * screenHeight;
        long long touchId = event->tfinger.fingerId;

        if (touchId == walkTouchId) {
            activeJoyX = fx;
            activeJoyY = fy;

            // Clamp active stick to joystick radius
            float dx = activeJoyX - leftJoyX;
            float dy = activeJoyY - leftJoyY;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist > leftRadius) {
                activeJoyX = leftJoyX + (dx / dist) * leftRadius;
                activeJoyY = leftJoyY + (dy / dist) * leftRadius;
            }
        }
        else if (touchId == lookTouchId) {
            float dx = fx - lastTouchCamX;
            float dy = fy - lastTouchCamY;

            // Look sensitivity for touch controls
            camera.processMouseMovement(dx * 0.35f, -dy * 0.35f);

            lastTouchCamX = fx;
            lastTouchCamY = fy;
            activeCamX = fx;
            activeCamY = fy;
        }
    }
    else if (event->type == SDL_FINGERUP) {
        long long touchId = event->tfinger.fingerId;

        if (touchId == walkTouchId) {
            walkTouchId = -1;
            activeJoyX = leftJoyX;
            activeJoyY = leftJoyY;
        }
        else if (touchId == lookTouchId) {
            lookTouchId = -1;
            activeCamX = rightJoyX;
            activeCamY = rightJoyY;
        }
    }
}

void Game::triggerNextDay() {
    currentDay++;
    if (currentDay > 6) {
        currentDay = 1; // loop back or restart
    }
    setupDaySettings();
    resetPlayer();
}

void Game::update(float deltaTime) {
    if (deltaTime > 0.1f) deltaTime = 0.1f; // Clamp delta to avoid massive teleports

    stateTimer += deltaTime;

    if (languageCyclePressed) {
        loc.cycleLanguage();
        languageCyclePressed = false;
        if (state == STATE_INTRO) {
            setupDaySettings();
        }
    }

    if (state == STATE_INTRO) {
        // Fade in text overlay
        if (fadeAlpha > 0.0f) {
            fadeAlpha -= deltaTime * 0.5f;
            if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        }

        // If action is pressed, advance to active gameplay!
        if (actionPressed) {
            actionPressed = false;
            state = STATE_GAMEPLAY;
            fadeAlpha = 0.0f;
        }
        flashlightTogglePressed = false;
        return;
    }

    if (state == STATE_SLEEP_FADE) {
        // Fade to black screen
        fadeAlpha += deltaTime * 0.8f;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            if (stateTimer > 2.0f) {
                triggerNextDay();
            }
        }
        flashlightTogglePressed = false;
        return;
    }

    if (state == STATE_JUMPSCARE) {
        // Red flashing screen + shake camera
        camera.position += Vec3((randFloat() - 0.5f) * 0.2f, (randFloat() - 0.5f) * 0.2f, (randFloat() - 0.5f) * 0.2f);
        if (actionPressed) {
            actionPressed = false;
            setupDaySettings(); // Restart day 6
            resetPlayer();
            state = STATE_GAMEPLAY;
        }
        flashlightTogglePressed = false;
        return;
    }

    if (state == STATE_ESCAPE_WIN) {
        if (actionPressed) {
            actionPressed = false;
            currentDay = 1;
            setupDaySettings();
            resetPlayer();
            state = STATE_INTRO;
        }
        flashlightTogglePressed = false;
        return;
    }

    // STATE_GAMEPLAY UPDATE
    // Capture a one-frame interaction token before it is consumed. This fixes Android
    // ACTION: the previous code reset actionPressed before objectives could read it.
    const bool interactPressed = actionPressed || keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_RETURN];

    const bool keyboardMoving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D];
    float joyDx = activeJoyX - leftJoyX;
    float joyDy = activeJoyY - leftJoyY;
    float joyDist = std::sqrt(joyDx * joyDx + joyDy * joyDy);
    const bool mobileMoving = isAndroid && walkTouchId != -1 && joyDist > 10.0f;

    const bool wantsSprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ||
                             (isAndroid && mobileMoving && joyDist > leftRadius * 0.82f);
    isSprinting = wantsSprint && (keyboardMoving || mobileMoving) && stamina > 0.05f;
    const float fearSlowdown = 1.0f - fear * 0.18f;
    camera.movementSpeed = (isSprinting ? 8.8f : 4.5f) * fearSlowdown;

    // Keyboard controls using scancodes (guaranteed safety, < 512)
    if (keys[SDL_SCANCODE_W]) camera.processKeyboard(1, deltaTime);
    if (keys[SDL_SCANCODE_S]) camera.processKeyboard(2, deltaTime);
    if (keys[SDL_SCANCODE_A]) camera.processKeyboard(3, deltaTime);
    if (keys[SDL_SCANCODE_D]) camera.processKeyboard(4, deltaTime);

    // Mobile Virtual Joystick movement processing
    if (mobileMoving) {
        float vx = joyDx / joyDist;
        float vy = joyDy / joyDist;

        float velocity = camera.movementSpeed * deltaTime;
        Vec3 horizontalFront = Vec3(camera.front.x, 0.0f, camera.front.z).normalized();
        Vec3 horizontalRight = Vec3(camera.right.x, 0.0f, camera.right.z).normalized();

        // Move player based on joystick axes
        camera.position -= horizontalFront * vy * velocity;
        camera.position += horizontalRight * vx * velocity;
    }

    if (isSprinting) {
        stamina = clamp(stamina - deltaTime * (0.22f + fear * 0.12f), 0.0f, 1.0f);
    } else {
        stamina = clamp(stamina + deltaTime * (0.14f - fear * 0.05f), 0.0f, 1.0f);
    }

    // Constrain player position to valley boundaries (keep inside play space)
    float dCabin = std::sqrt(camera.position.x*camera.position.x + camera.position.z*camera.position.z);
    camera.position.x = fmaxf(-120.0f, fminf(120.0f, camera.position.x));
    camera.position.z = fmaxf(-120.0f, fminf(120.0f, camera.position.z));

    // Force player height to follow terrain height
    camera.position.y = getTerrainHeight(camera.position.x, camera.position.z) + 1.8f;

    // Flashlight battery + toggling. Light is powerful against the fog, but not free.
    const bool flashlightDay = (currentDay == 4 || currentDay == 5);
    if (flashlightDay && flashlightTogglePressed && flashlightBattery > 0.03f) {
        flashlightOn = !flashlightOn;
    }
    flashlightTogglePressed = false;

    if (flashlightDay) {
        const float drainRate = (currentDay == 5) ? 0.052f : 0.026f;
        const float recoverRate = (currentDay == 5) ? 0.006f : 0.018f;
        if (flashlightOn) {
            flashlightBattery = clamp(flashlightBattery - deltaTime * drainRate, 0.0f, 1.0f);
            if (flashlightBattery <= 0.001f) {
                flashlightOn = false;
                scareFlashAlpha = std::max(scareFlashAlpha, 0.45f);
            }
        } else {
            flashlightBattery = clamp(flashlightBattery + deltaTime * recoverRate, 0.0f, 1.0f);
        }
    }

    if (flashlightOn) {
        if (currentDay == 5) {
            flashlightFlickerTimer += deltaTime;
            if (flashlightFlickerTimer > 1.5f) {
                const float lowBatteryPenalty = 1.0f - flashlightBattery;
                if (rand() % 5 == 0) {
                    flashlightIntensity = clamp((rand() % 10) / 10.0f - lowBatteryPenalty * 0.35f, 0.08f, 0.9f);
                } else {
                    flashlightIntensity = clamp(0.82f - lowBatteryPenalty * 0.45f, 0.15f, 0.9f);
                }
                if (flashlightFlickerTimer > 2.5f) flashlightFlickerTimer = 0.0f;
            }
        } else {
            flashlightIntensity = flashlightDay ? clamp(0.35f + flashlightBattery * 0.75f, 0.25f, 1.1f) : 1.0f;
        }
    } else {
        flashlightIntensity = 0.0f;
    }

    // Sound logic: visual heartbeat rate increase
    heartbeatTimer += deltaTime;
    float distToDanger = 200.0f;

    if (currentDay == 6) {
        distToDanger = (camera.position - chaser.pos).length();
        // Heartbeat speed multiplies based on monster proximity
        float factor = clamp((35.0f - distToDanger) / 35.0f, 0.0f, 1.0f);
        heartbeatRate = 0.25f + (1.0f - factor) * 0.95f; // faster beats
    }

    // Fear / sanity pressure. Looking straight at things in the fog is costly;
    // the cabin calms the player down.
    float fearDelta = -0.055f * deltaTime;
    auto isLookingAt = [&](const Vec3& pos, float dotThreshold) {
        Vec3 toTarget = (pos - camera.position).normalized();
        return Vec3::dot(camera.front, toTarget) > dotThreshold;
    };

    if (currentDay == 4 || currentDay == 5) {
        if (flashlightDay && !flashlightOn) fearDelta += 0.045f * deltaTime;
        if (dCabin < 5.0f) fearDelta -= 0.12f * deltaTime;

        for (auto& eye : redEyes) {
            float distEye = (camera.position - eye.pos).length();
            bool seen = distEye < 55.0f && isLookingAt(eye.pos, 0.965f);
            bool lit = seen && flashlightOn && flashlightIntensity > 0.15f;
            if (lit) {
                eye.opacity = clamp(eye.opacity - deltaTime * 0.75f, 0.18f, 1.0f);
                fearDelta += 0.045f * deltaTime;
            } else {
                eye.opacity = clamp(eye.opacity + deltaTime * 0.20f, 0.18f, 1.0f);
                if (seen) fearDelta += 0.18f * deltaTime;
            }
        }

        for (auto& sil : silhouettes) {
            float distSil = (camera.position - sil.pos).length();
            bool seen = sil.opacity > 0.05f && distSil < 65.0f && isLookingAt(sil.pos, 0.955f);
            if (seen) {
                fearDelta += (flashlightOn ? 0.12f : 0.28f) * deltaTime;
                if (flashlightOn && currentDay == 4) {
                    sil.opacity = clamp(sil.opacity - deltaTime * 0.35f, 0.25f, 1.0f);
                }
            }
        }
    }

    if (currentDay == 6) {
        fearDelta += clamp((42.0f - distToDanger) / 42.0f, 0.0f, 1.0f) * 0.32f * deltaTime;
    }

    fear = clamp(fear + fearDelta, 0.0f, 1.0f);
    scareFlashAlpha = clamp(scareFlashAlpha - deltaTime * 0.7f, 0.0f, 1.0f);
    heartbeatRate = std::min(heartbeatRate, 1.1f - fear * 0.65f);

    // ENTITIES & OBJECTIVES LOGIC BASED ON DAYS
    if (currentDay == 1) {
        // Interactive Sleep
        if (dCabin < 3.5f) {
            objectiveText = loc.tr("day1.objective.sleep_prompt", "Press E (or Tap ACTION) on Bed inside cabin to sleep.");
            if (interactPressed) {
                actionPressed = false;
                state = STATE_SLEEP_FADE;
                stateTimer = 0.0f;
            }
        } else {
            objectiveText = loc.tr("day1.objective.explore", "Objective: Explore the valley.\nEnter the Cabin (0,0) to sleep when tired.");
        }
    }
    else if (currentDay == 2) {
        // Inspect the old stone well (X:-20, Z:-50)
        float dWell = std::sqrt((camera.position.x + 20.0f)*(camera.position.x + 20.0f) + (camera.position.z + 50.0f)*(camera.position.z + 50.0f));
        if (!wellInspected) {
            objectiveText = loc.tr("day2.objective.find_well", "Objective: Check the old stone well (X:-20, Z:-50) in northern woods.");
            if (dWell < 6.0f) {
                wellInspected = true;
                objectiveText = loc.tr("day2.objective.well_inspected", "Well inspected. Cold wind blows from below... RETURN TO CABIN BED.");
            }
        } else {
            objectiveText = loc.tr("day2.objective.return", "Objective: Well inspected. Run back to Cabin Bed to sleep.");
            if (dCabin < 3.5f) {
                objectiveText = loc.tr("objective.sleep_prompt", "Press E (or Tap ACTION) to sleep.");
                if (interactPressed) {
                    wellInspected = false;
                    actionPressed = false;
                    state = STATE_SLEEP_FADE;
                    stateTimer = 0.0f;
                }
            }
        }
    }
    else if (currentDay == 3) {
        // Gather 3 wooden log piles around the cabin
        for (int i = 0; i < 3; ++i) {
            if (!woodLogs[i].collected) {
                float dLog = (camera.position - woodLogs[i].pos).length();
                if (dLog < 3.5f) {
                    woodLogs[i].collected = true;
                    logsCollected++;
                }
            }
        }

        if (logsCollected < 3) {
            std::stringstream ss;
            ss << loc.tr("day3.objective.gather_prefix", "Objective: Gather logs for fireplace around cabin (")
               << logsCollected
               << loc.tr("day3.objective.gather_suffix", "/3 gathered).");
            objectiveText = ss.str();
        } else {
            objectiveText = loc.tr("day3.objective.done", "Objective: Log gathering completed. Return to Bed to sleep.");
            if (dCabin < 3.5f) {
                objectiveText = loc.tr("objective.sleep_prompt", "Press E (or Tap ACTION) to sleep.");
                if (interactPressed) {
                    actionPressed = false;
                    state = STATE_SLEEP_FADE;
                    stateTimer = 0.0f;
                }
            }
        }
    }
    else if (currentDay == 4) {
        // Find missing diary page (X:-35, Z:-40)
        float dPage = (camera.position - diaryPage.pos).length();
        if (!diaryPage.collected) {
            objectiveText = loc.tr("day4.objective.find_page", "Objective: Retrieve the lost notebook page in Western Woods (X:-35, Z:-40).\nFlashlight drains battery: press F / LIGHT to toggle.");
            if (dPage < 4.0f) {
                diaryPage.collected = true;
                objectiveText = loc.tr("day4.objective.page_collected", "Page Collected! '...THEY ARE IN THE FOG...'. RUN BACK TO CABIN BED!");
            }
        } else {
            objectiveText = loc.tr("day4.objective.return", "Objective: RUN BACK! Enter the Cabin Bed and LOCK the door!");
            // Make shadows stare and fade on Day 4
            for (auto& sil : silhouettes) {
                float dSil = (camera.position - sil.pos).length();
                if (dSil < 14.0f) {
                    sil.opacity -= deltaTime * 1.5f;
                    if (sil.opacity < 0.0f) sil.opacity = 0.0f;
                }
            }

            if (dCabin < 3.5f) {
                objectiveText = loc.tr("day4.objective.lock_prompt", "Press E (or Tap ACTION) to LOCK THE DOOR & sleep.");
                if (interactPressed) {
                    actionPressed = false;
                    state = STATE_SLEEP_FADE;
                    stateTimer = 0.0f;
                }
            }
        }
    }
    else if (currentDay == 5) {
        // Holy Cross in the sacred grove (X:45, Z:-45)
        float dCross = (camera.position - woodenCross.pos).length();
        if (!woodenCross.collected) {
            objectiveText = loc.tr("day5.objective.find_cross", "Objective: Cabin is compromised! Find the Holy Cross in Eastern Grove (X:45, Z:-45).\nConserve flashlight battery. Panic slows you down.");
            if (dCross < 5.0f) {
                woodenCross.collected = true;
                objectiveText = loc.tr("day5.objective.cross_collected", "The Cross glows! Red eyes scream in pain! Escape back to Cabin!");
            }

            // Silhouettes actively slide towards player when back is turned on Day 5
            for (auto& sil : silhouettes) {
                Vec3 dirToPlayer = (camera.position - sil.pos);
                float dist = dirToPlayer.length();
                if (dist > 15.0f && dist < 60.0f) {
                    // Check if player is facing away from the silhouette
                    float angle = Vec3::dot(camera.front, dirToPlayer.normalized());
                    if (angle > -0.3f) { // backing away / side look
                        sil.pos += dirToPlayer.normalized() * (1.8f * deltaTime);
                        sil.pos.y = getTerrainHeight(sil.pos.x, sil.pos.z) + 2.0f;
                    }
                }
            }
        } else {
            objectiveText = loc.tr("day5.objective.return", "Objective: Cross acquired. It burns them back -- run to Cabin and bar the door!");
            fear = clamp(fear - deltaTime * 0.16f, 0.0f, 1.0f);
            for (auto& sil : silhouettes) {
                Vec3 away = (sil.pos - camera.position);
                float dist = away.length();
                if (dist < 38.0f && dist > 0.1f) {
                    sil.pos += away.normalized() * (2.4f * deltaTime);
                    sil.pos.y = getTerrainHeight(sil.pos.x, sil.pos.z) + 2.0f;
                    sil.opacity = clamp(sil.opacity - deltaTime * 0.25f, 0.25f, 1.0f);
                }
            }
            if (dCabin < 3.5f) {
                objectiveText = loc.tr("day5.objective.bar_prompt", "Press E (or Tap ACTION) to bar the door.");
                if (interactPressed) {
                    actionPressed = false;
                    state = STATE_SLEEP_FADE;
                    stateTimer = 0.0f;
                }
            }
        }
    }
    else if (currentDay == 6) {
        // Day 6 ESCAPE to CAR (X:10, Z:75)
        float dCar = std::sqrt((camera.position.x - 10.0f)*(camera.position.x - 10.0f) + (camera.position.z - 75.0f)*(camera.position.z - 75.0f));
        objectiveText = loc.tr("day6.objective.escape", "Objective: NO LIGHTS! NO PATH! REACH THE PARKING ROAD (X:10, Z:75)!");

        // Active Stalker chases the player. The first seconds are a grace period,
        // then the monster dynamically pressures but avoids hopeless long-distance desync.
        Vec3 dirToPlayer = (camera.position - chaser.pos);
        float dist = dirToPlayer.length();
        if (chaserGraceTimer > 0.0f) {
            chaserGraceTimer -= deltaTime;
            Vec3 behind = camera.position - Vec3(camera.front.x, 0.0f, camera.front.z).normalized() * 20.0f;
            chaser.pos = Vec3(behind.x, getTerrainHeight(behind.x, behind.z) + 2.0f, behind.z);
            dist = 20.0f;
        } else {
            float speed = 5.45f + fear * 1.1f + clamp(stateTimer / 120.0f, 0.0f, 1.0f) * 0.75f;
            if (isLookingAt(chaser.pos, 0.94f) && dist < 30.0f) {
                speed *= 0.72f; // looking back buys time, but raises fear elsewhere
                fear = clamp(fear + deltaTime * 0.10f, 0.0f, 1.0f);
            }
            chaser.pos += dirToPlayer.normalized() * (speed * deltaTime);
            chaser.pos.y = getTerrainHeight(chaser.pos.x, chaser.pos.z) + 2.0f;

            if (dist > 55.0f) {
                // Fog shortcut: if the player outpaces pathing too hard, the stalker reappears behind.
                Vec3 behind = camera.position - Vec3(camera.front.x, 0.0f, camera.front.z).normalized() * 38.0f;
                chaser.pos = Vec3(behind.x, getTerrainHeight(behind.x, behind.z) + 2.0f, behind.z);
                scareFlashAlpha = std::max(scareFlashAlpha, 0.22f);
            }
        }

        // If chaser catches player, screen jumpscare!
        if (dist < 2.5f && chaserGraceTimer <= 0.0f) {
            state = STATE_JUMPSCARE;
            stateTimer = 0.0f;
        }

        if (dCar < 5.0f) {
            objectiveText = loc.tr("day6.objective.car_prompt", "CAR REACHED! Press E (or Tap ACTION) to Start Engine and Escape!");
            if (interactPressed) {
                actionPressed = false;
                state = STATE_ESCAPE_WIN;
                stateTimer = 0.0f;
            }
        }
    }

    // Touch/key one-shot interactions are consumed once per gameplay frame.
    actionPressed = false;
}

void Game::drawEntityBillboard(const Shader& shader, const Vec3& pos, float scaleX, float scaleY, bool isSil, float opacity) {
    shader.use();

    // Standard cylindrical billboard calculation: billboard faces camera on Y axis
    Vec3 camPos = camera.position;
    Vec3 dir = (camPos - pos);
    float angle = std::atan2(dir.x, dir.z);

    Mat4 modelMat = Mat4::translation(pos) * Mat4::rotationY(angle) * Mat4::scaling(Vec3(scaleX, scaleY, 1.0f));
    shader.setMat4("model", modelMat);
    shader.setMat4("view", camera.getViewMatrix());
    
    float ratio = (float)screenWidth / screenHeight;
    shader.setMat4("projection", Mat4::perspective(45.0f * M_PI / 180.0f, ratio, 0.1f, 1000.0f));

    shader.setVec3("viewPos", camera.position);
    shader.setVec3("fogColor", fogColor);
    shader.setFloat("fogStart", fogStart);
    shader.setFloat("fogEnd", fogEnd);
    shader.setVec3("itemPos", pos);

    shader.setInt("isSilhouette", isSil ? 1 : 0);
    shader.setFloat("opacity", opacity);

    billboardQuad.draw();
}

void Game::render() {
    // Clear screen
    glClearColor(fogColor.x, fogColor.y, fogColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (state == STATE_JUMPSCARE) {
        // Red flashes and black bars
        float flash = std::sin(stateTimer * 40.0f) * 0.5f + 0.5f;
        float bloodColor[4] = {flash, 0.0f, 0.0f, 1.0f};
        ui.drawRect(0, 0, screenWidth, screenHeight, bloodColor);

        // Creepy static noise text overlay
        float textCol[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        ui.drawText(loc.tr("jumpscare.title", "THE FOG CLAIMED YOU"), screenWidth * 0.5f - 180.0f, screenHeight * 0.4f, 3.0f, textCol);
        ui.drawText(loc.tr("jumpscare.subtitle", "THEY ARE ALIVE INSIDE THE FOG"), screenWidth * 0.5f - 240.0f, screenHeight * 0.5f, 2.0f, textCol);
        ui.drawText(loc.tr("jumpscare.retry", "Press ACTION (or E) to retry Day 6."), screenWidth * 0.5f - 210.0f, screenHeight * 0.65f, 1.5f, textCol);
        return;
    }

    if (state == STATE_INTRO) {
        // Draw the creepy story screen
        float black[4] = {0.02f, 0.02f, 0.03f, 1.0f};
        ui.drawRect(0, 0, screenWidth, screenHeight, black);

        float textCol[4] = {0.85f, 0.85f, 0.90f, 1.0f};
        ui.drawText(diaryText, screenWidth * 0.12f, screenHeight * 0.2f, 1.6f, textCol);

        float promptCol[4] = {0.6f, 0.0f, 0.0f, 1.0f + std::sin(stateTimer * 4.0f) * 0.3f};
        ui.drawText(loc.tr("intro.prompt", "Press ACTION / ENTER to begin..."), screenWidth * 0.12f, screenHeight * 0.8f, 1.5f, promptCol);
        return;
    }

    if (state == STATE_ESCAPE_WIN) {
        float deepBlack[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        ui.drawRect(0, 0, screenWidth, screenHeight, deepBlack);

        float goldText[4] = {0.95f, 0.85f, 0.45f, 1.0f};
        ui.drawText(loc.tr("win.title", "YOU SURVIVED"), screenWidth * 0.5f - 110.0f, screenHeight * 0.3f, 3.0f, goldText);
        
        float descText[4] = {0.8f, 0.8f, 0.8f, 1.0f};
        ui.drawText(loc.tr("win.description", "You managed to start the old sedan and drive out of the valley.\nThe red eyes faded behind the rear view mirror.\nBut the fog inside your head remains...\n\nThank you for playing!"), 
                    screenWidth * 0.15f, screenHeight * 0.45f, 1.5f, descText);

        float replayText[4] = {0.5f, 0.5f, 0.5f, 1.0f};
        ui.drawText(loc.tr("win.restart", "Press ACTION (or E) to restart the horror loop."), screenWidth * 0.15f, screenHeight * 0.85f, 1.2f, replayText);
        return;
    }

    // STATE_GAMEPLAY RENDER PIPELINE
    mainShader.use();
    mainShader.setMat4("view", camera.getViewMatrix());

    float ratio = (float)screenWidth / screenHeight;
    mainShader.setMat4("projection", Mat4::perspective(45.0f * M_PI / 180.0f, ratio, 0.1f, 1000.0f));

    mainShader.setVec3("viewPos", camera.position);
    mainShader.setVec3("fogColor", fogColor);
    mainShader.setFloat("fogStart", fogStart);
    mainShader.setFloat("fogEnd", fogEnd);

    mainShader.setVec3("ambientColor", ambientColor);
    mainShader.setVec3("dirLightColor", dirLightColor);
    mainShader.setVec3("dirLightDir", dirLightDir);

    // Flashlight Spot settings
    mainShader.setVec3("flashPos", camera.position);
    mainShader.setVec3("flashDir", camera.front);
    mainShader.setFloat("flashCutoff", std::cos(14.5f * M_PI / 180.0f)); // narrow focus cone
    mainShader.setFloat("flashIntensity", flashlightIntensity);

    // 1. Draw Terrain
    Mat4 terrainModel = Mat4::identity();
    mainShader.setMat4("model", terrainModel);
    mainShader.setInt("useTexture", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTexture.id);
    terrainMesh.draw();

    // 2. Draw Cabin
    Mat4 cabinModel = Mat4::identity();
    mainShader.setMat4("model", cabinModel);
    mainShader.setInt("useTexture", 1);
    glBindTexture(GL_TEXTURE_2D, woodTexture.id);
    cabinMesh.draw();

    // 3. Draw Well (X:-20, Z:-50)
    Mat4 wellModel = Mat4::translation(Vec3(-20.0f, getTerrainHeight(-20.0f, -50.0f), -50.0f));
    mainShader.setMat4("model", wellModel);
    mainShader.setInt("useTexture", 0);
    wellMesh.draw();

    // 4. Draw Car (X:10, Z:75)
    Mat4 carModel = Mat4::translation(Vec3(10.0f, getTerrainHeight(10.0f, 75.0f), 75.0f)) * Mat4::rotationY(M_PI / 4.0f);
    mainShader.setMat4("model", carModel);
    mainShader.setInt("useTexture", 0);
    carMesh.draw();

    // 5. Draw Scattered Trees
    mainShader.setInt("useTexture", 0);
    for (size_t i = 0; i < treePositions.size(); ++i) {
        const Vec3& pos = treePositions[i];
        float treeScale = treeScales[i];
        Mat4 treeModel = Mat4::translation(pos) * Mat4::scaling(Vec3(treeScale, treeScale, treeScale));
        mainShader.setMat4("model", treeModel);
        treeMesh.draw();
    }

    // 6. Draw Day 3 Logs
    if (currentDay == 3) {
        mainShader.setInt("useTexture", 0);
        for (const auto& log : woodLogs) {
            if (!log.collected) {
                Mat4 logModel = Mat4::translation(log.pos) * Mat4::rotationX(M_PI / 2.0f) * Mat4::scaling(Vec3(0.5f, 0.5f, 1.8f));
                mainShader.setMat4("model", logModel);
                logMesh.draw();
            }
        }
    }

    // 7. Draw Day 4 Notebook Page
    if (currentDay == 4 && !diaryPage.collected) {
        Mat4 pageModel = Mat4::translation(diaryPage.pos) * Mat4::scaling(Vec3(0.4f, 0.05f, 0.5f));
        mainShader.setMat4("model", pageModel);
        mainShader.setInt("useTexture", 0);
        pageMesh.draw();
    }

    // 8. Draw Day 5 Wooden Cross
    if (currentDay == 5 && !woodenCross.collected) {
        const float glowScale = 1.0f + std::sin(stateTimer * 6.0f) * 0.04f;
        Mat4 crossModel = Mat4::translation(woodenCross.pos) * Mat4::scaling(Vec3(glowScale, glowScale, glowScale));
        mainShader.setMat4("model", crossModel);
        mainShader.setInt("useTexture", 0);

        // Horizontal and vertical planks
        crossVerticalMesh.draw();
        crossHorizontalMesh.draw();
    }

    // RENDER BILLBOARDS (REDEYES & SILHOUETTES)
    // Blend mode for sprite/alpha opacity drawing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // transparent quads should not block each other via depth writes

    // Render Red Eyes (Day 4 & Day 5)
    if (currentDay == 4 || currentDay == 5) {
        for (const auto& eye : redEyes) {
            float dist = (camera.position - eye.pos).length();
            if (dist < 45.0f) {
                // Eyes face camera, scale size nicely
                float opacity = eye.opacity;
                if (dist < 18.0f) {
                    opacity *= (dist - 5.0f) / 13.0f; // fade away when getting close!
                    if (opacity < 0.0f) opacity = 0.0f;
                }
                if (opacity > 0.0f) {
                    drawEntityBillboard(billboardShader, eye.pos, 0.5f, 0.3f, false, opacity);
                }
            }
        }
    }

    // Render Silhouettes (Day 4 & Day 5)
    if (currentDay == 4 || currentDay == 5) {
        for (const auto& sil : silhouettes) {
            if (sil.opacity > 0.01f) {
                float dist = (camera.position - sil.pos).length();
                if (dist < 60.0f) {
                    drawEntityBillboard(billboardShader, sil.pos, 1.2f, 2.5f, true, sil.opacity);
                }
            }
        }
    }

    // Render Chaser Stalker on Day 6
    if (currentDay == 6 && chaser.active) {
        drawEntityBillboard(billboardShader, chaser.pos, 1.4f, 2.8f, true, chaser.opacity);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // DRAW UI OVERLAYS (HUD, INSTRUCTIONS, HEARTBEAT SCREEN SHAKE)
    float textCol[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    if (currentDay >= 5) {
        // Red pulse on HUD if stalking is close. Day 6 uses the actual chaser distance;
        // Day 5 keeps a constant dread vignette.
        float danger = 0.22f;
        if (currentDay == 6) {
            danger = 1.0f - clamp((camera.position - chaser.pos).length() / 45.0f, 0.0f, 1.0f);
        }
        const float pulse = 0.55f + 0.45f * std::sin(heartbeatTimer * (6.28318f / std::max(0.12f, heartbeatRate)));
        float redBorder[4] = {0.8f, 0.0f, 0.0f, (0.08f + 0.18f * pulse) * danger};
        ui.drawRect(0, 0, screenWidth, screenHeight, redBorder);
    }

    if (currentDay >= 4) {
        float panicTint[4] = {0.02f, 0.0f, 0.0f, fear * 0.18f};
        ui.drawRect(0, 0, screenWidth, screenHeight, panicTint);
        if (fear > 0.58f) {
            float staticCol[4] = {1.0f, 1.0f, 1.0f, (fear - 0.58f) * 0.10f};
            float y1 = std::fmod(stateTimer * 97.0f, (float)screenHeight);
            float y2 = std::fmod(stateTimer * 151.0f + 80.0f, (float)screenHeight);
            ui.drawRect(0.0f, y1, (float)screenWidth, 2.0f, staticCol);
            ui.drawRect(0.0f, y2, (float)screenWidth, 1.0f, staticCol);
        }
        if (scareFlashAlpha > 0.01f) {
            float scareCol[4] = {1.0f, 1.0f, 1.0f, scareFlashAlpha * 0.35f};
            ui.drawRect(0, 0, screenWidth, screenHeight, scareCol);
        }
    }

    // Draw Crosshair
    float white[4] = {1.0f, 1.0f, 1.0f, 0.5f};
    ui.drawRect(screenWidth * 0.5f - 2.0f, screenHeight * 0.5f - 2.0f, 4.0f, 4.0f, white);

    // Text HUD
    std::stringstream ssDay;
    ssDay << loc.tr("hud.day", "DAY") << ": " << currentDay << " / 6";
    ui.drawText(ssDay.str(), 20.0f, 20.0f, 2.0f, textCol);

    ui.drawText(objectiveText, 20.0f, 60.0f, 1.2f, textCol);

    // Stamina bar: sprint becomes a tactical resource instead of a free escape button.
    float staminaBg[4] = {0.02f, 0.02f, 0.02f, 0.65f};
    float staminaCol[4] = {0.15f + (1.0f - stamina) * 0.75f, 0.85f * stamina, 0.20f, 0.85f};
    ui.drawRect(20.0f, screenHeight - 34.0f, 210.0f, 14.0f, staminaBg);
    ui.drawRect(25.0f, screenHeight - 31.0f, 200.0f * stamina, 8.0f, staminaCol);
    float staminaText[4] = {0.8f, 0.8f, 0.8f, 0.7f};
    ui.drawText(isSprinting ? loc.tr("hud.sprint", "SPRINT") : loc.tr("hud.stamina", "STAMINA"), 20.0f, screenHeight - 56.0f, 1.0f, staminaText);

    if (currentDay >= 4) {
        float fearBg[4] = {0.02f, 0.0f, 0.0f, 0.65f};
        float fearCol[4] = {0.85f, 0.05f, 0.05f, 0.82f};
        ui.drawRect(20.0f, screenHeight - 76.0f, 210.0f, 12.0f, fearBg);
        ui.drawRect(25.0f, screenHeight - 73.0f, 200.0f * fear, 6.0f, fearCol);
        ui.drawText(loc.tr("hud.panic", "PANIC"), 20.0f, screenHeight - 96.0f, 1.0f, staminaText);
    }

    if (currentDay == 4 || currentDay == 5) {
        float batteryBg[4] = {0.02f, 0.02f, 0.02f, 0.65f};
        float batteryCol[4] = {0.95f, 0.86f, 0.35f, flashlightOn ? 0.90f : 0.45f};
        ui.drawRect(250.0f, screenHeight - 34.0f, 210.0f, 14.0f, batteryBg);
        ui.drawRect(255.0f, screenHeight - 31.0f, 200.0f * flashlightBattery, 8.0f, batteryCol);
        ui.drawText(flashlightOn ? loc.tr("hud.light_on", "LIGHT ON") : loc.tr("hud.light_off", "LIGHT OFF"), 250.0f, screenHeight - 56.0f, 1.0f, staminaText);
    }

    if (isAndroid && (currentDay == 4 || currentDay == 5)) {
        float lightButton[4] = {0.9f, 0.75f, 0.2f, flashlightOn ? 0.55f : 0.28f};
        ui.drawRect(screenWidth - 150.0f, 95.0f, 120.0f, 120.0f, lightButton);
        float lightText[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        ui.drawText(loc.tr("hud.light_button", "LIGHT"), screenWidth - 140.0f, 145.0f, 1.3f, lightText);
    }

    // Display coordinate trackers on Android or for easier guidance
    std::stringstream ssCoords;
    ssCoords << loc.tr("hud.coords", "COORDS") << ": X:" << (int)camera.position.x << " Z:" << (int)camera.position.z;
    float coordsColor[4] = {0.8f, 0.8f, 0.8f, 0.6f};
    ui.drawText(ssCoords.str(), screenWidth - 250.0f, 20.0f, 1.2f, coordsColor);

    std::stringstream ssLang;
    ssLang << loc.tr("hud.lang", "LANG") << ": " << loc.currentName() << " (F2)";
    ui.drawText(ssLang.str(), screenWidth - 250.0f, 68.0f, 1.0f, coordsColor);

    // Lightweight target tracker: keeps the horror navigation tense, but not confusing.
    Vec3 targetPos;
    std::string targetName;
    bool hasTarget = true;
    if (currentDay == 1) { targetPos = Vec3(0.0f, 0.0f, 0.0f); targetName = loc.tr("target.cabin", "CABIN"); }
    else if (currentDay == 2) { targetPos = wellInspected ? Vec3(0.0f, 0.0f, 0.0f) : Vec3(-20.0f, 0.0f, -50.0f); targetName = wellInspected ? loc.tr("target.cabin", "CABIN") : loc.tr("target.well", "WELL"); }
    else if (currentDay == 3) {
        targetPos = Vec3(0.0f, 0.0f, 0.0f);
        targetName = loc.tr("target.cabin", "CABIN");
        for (const auto& log : woodLogs) {
            if (!log.collected) { targetPos = log.pos; targetName = loc.tr("target.log", "LOG"); break; }
        }
    }
    else if (currentDay == 4) { targetPos = diaryPage.collected ? Vec3(0.0f, 0.0f, 0.0f) : diaryPage.pos; targetName = diaryPage.collected ? loc.tr("target.cabin", "CABIN") : loc.tr("target.page", "PAGE"); }
    else if (currentDay == 5) { targetPos = woodenCross.collected ? Vec3(0.0f, 0.0f, 0.0f) : woodenCross.pos; targetName = woodenCross.collected ? loc.tr("target.cabin", "CABIN") : loc.tr("target.cross", "CROSS"); }
    else if (currentDay == 6) { targetPos = Vec3(10.0f, 0.0f, 75.0f); targetName = loc.tr("target.car", "CAR"); }
    else { hasTarget = false; }
    if (hasTarget) {
        float distToTarget = Vec3(camera.position.x - targetPos.x, 0.0f, camera.position.z - targetPos.z).length();
        std::stringstream ssTarget;
        ssTarget << loc.tr("hud.target", "TARGET") << ": " << targetName << " " << (int)distToTarget << "m";
        ui.drawText(ssTarget.str(), screenWidth - 250.0f, 44.0f, 1.2f, coordsColor);
    }

    // Draw Touch Controls on Android Screen
    ui.drawVirtualJoysticks(leftJoyX, leftJoyY, leftRadius, activeJoyX, activeJoyY,
                            rightJoyX, rightJoyY, rightRadius, activeCamX, activeCamY,
                            isAndroid, loc.tr("hud.action_button", "ACTION"));

    // Day Transition fading logic
    if (state == STATE_SLEEP_FADE) {
        float blackFade[4] = {0.0f, 0.0f, 0.0f, fadeAlpha};
        ui.drawRect(0, 0, screenWidth, screenHeight, blackFade);
    }
}

void Game::cleanup() {
    mainShader.cleanup();
    billboardShader.cleanup();
    grassTexture.cleanup();
    woodTexture.cleanup();
    terrainMesh.cleanup();
    cabinMesh.cleanup();
    treeMesh.cleanup();
    wellMesh.cleanup();
    carMesh.cleanup();
    billboardQuad.cleanup();
    logMesh.cleanup();
    pageMesh.cleanup();
    crossVerticalMesh.cleanup();
    crossHorizontalMesh.cleanup();
    treePositions.clear();
    treeScales.clear();
    ui.cleanup();
}
