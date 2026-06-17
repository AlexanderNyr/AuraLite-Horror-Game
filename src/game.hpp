#pragma once
#include <vector>
#include <string>
#include "math3d.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "ui.hpp"
#include "localization.hpp"

enum GameState {
    STATE_INTRO,
    STATE_GAMEPLAY,
    STATE_SLEEP_FADE,
    STATE_JUMPSCARE,
    STATE_ESCAPE_WIN
};

struct Collectible {
    Vec3 pos;
    bool collected = false;
};

struct Monster {
    Vec3 pos;
    float yaw = 0.0f;
    bool active = true;
    float opacity = 1.0f;
};

class Game {
public:
    GameState state = STATE_INTRO;
    int currentDay = 1;
    bool isAndroid = false;

    // Window and view settings
    int screenWidth = 1280;
    int screenHeight = 720;

    // Engine subsystems
    Camera camera;
    UIRenderer ui;
    Localization loc;

    // Rendering pipeline shaders and assets
    Shader mainShader;
    Shader billboardShader;
    Texture grassTexture;
    Texture woodTexture;

    // Procedural Meshes
    Mesh terrainMesh;
    Mesh cabinMesh;
    Mesh treeMesh;
    Mesh wellMesh;
    Mesh carMesh;
    Mesh billboardQuad; // simple quad for billboards

    // Reusable objective item meshes (avoid rebuilding GPU buffers every frame)
    Mesh logMesh;
    Mesh pageMesh;
    Mesh crossVerticalMesh;
    Mesh crossHorizontalMesh;

    // Precomputed forest instances (deterministic, no per-frame random regeneration)
    std::vector<Vec3> treePositions;
    std::vector<float> treeScales;

    // Environment and fog parameters
    Vec3 ambientColor;
    Vec3 dirLightColor;
    Vec3 dirLightDir;
    Vec3 fogColor;
    float fogStart = 500.0f;
    float fogEnd = 1000.0f;

    // Flashlight parameters
    bool flashlightOn = false;
    float flashlightIntensity = 0.0f;
    float flashlightFlickerTimer = 0.0f;
    float flashlightBattery = 1.0f; // 0..1, Days 4-5 survival resource

    // Psychological pressure: seeing entities, darkness and the chase raise fear.
    // Fear affects stamina, heartbeat and screen effects.
    float fear = 0.0f;
    float scareFlashAlpha = 0.0f;
    float chaserGraceTimer = 0.0f;

    // Objectives and Story details
    std::string objectiveText;
    std::string diaryText;
    float fadeAlpha = 1.0f;
    float stateTimer = 0.0f;

    // Day 2 Well inspection
    bool wellInspected = false;

    // Day 3 Wood Logs
    std::vector<Collectible> woodLogs;
    int logsCollected = 0;

    // Day 4 Diary Page
    Collectible diaryPage;

    // Day 5 Wooden Cross
    Collectible woodenCross;

    // Entities (Eyes and Silhouettes)
    std::vector<Monster> redEyes;
    std::vector<Monster> silhouettes;
    Monster chaser; // Day 6 Active Chaser

    // Player inputs
    bool keys[512] = {false};
    bool actionPressed = false;
    bool flashlightTogglePressed = false;
    bool languageCyclePressed = false;

    // Mobile inputs
    bool touchActive = false;
    float leftJoyX = 150.0f, leftJoyY = 570.0f, leftRadius = 80.0f;
    float rightJoyX = 1130.0f, rightJoyY = 570.0f, rightRadius = 120.0f;
    float activeJoyX = 150.0f, activeJoyY = 570.0f;
    float activeCamX = 1130.0f, activeCamY = 570.0f;
    int walkTouchId = -1;
    int lookTouchId = -1;
    float lastTouchCamX = 0.0f;
    float lastTouchCamY = 0.0f;

    // Player stamina and visual/audio cues
    float stamina = 1.0f;       // 0..1 normalized
    float heartbeatRate = 1.0f; // seconds between visual heartbeat pulses
    float heartbeatTimer = 0.0f;
    bool isSprinting = false;

    void init(int width, int height, bool mobileMode);
    void resize(int width, int height);
    void handleEvent(void* sdlEvent);
    void update(float deltaTime);
    void render();
    void cleanup();

private:
    void setupDaySettings();
    void updateTouchLayout();
    void spawnEntities();
    void triggerNextDay();
    void resetPlayer();
    void drawEntityBillboard(const Shader& shader, const Vec3& pos, float scaleX, float scaleY, bool isSilhouette, float opacity);
};
