#pragma once
#include <vector>
#include <string>
#include "math3d.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "ui.hpp"
#include "localization.hpp"

enum GameState {
    STATE_MENU,
    STATE_INTRO,
    STATE_GAMEPLAY,
    STATE_SLEEP_FADE,
    STATE_ENDING
};

struct Collectible {
    Vec3 pos;
    bool collected = false;
    std::string type;
};

struct VillageObject {
    Vec3 pos;
    int type; // 0=house, 1=rock, 2=fence, 3=well
    float scale = 1.0f;
};

class Game {
public:
    GameState state = STATE_INTRO;
    int currentDay = 1;
    bool isAndroid = false;

    int screenWidth = 1280;
    int screenHeight = 720;

    Camera camera;
    UIRenderer ui;
    Localization loc;

    Shader mainShader;
    Shader billboardShader;
    Texture grassTexture;
    Texture woodTexture;
    Texture pathTexture;

    Mesh terrainMesh;
    Mesh cabinMesh;
    Mesh treeMesh;
    Mesh wellMesh;
    Mesh carMesh;
    Mesh billboardQuad;

    Mesh logMesh;
    Mesh flowerMesh;
    Mesh stoneMesh;
    Mesh toolMesh;

    // === NEW: Village meshes ===
    Mesh houseMesh;
    Mesh rockMesh;
    Mesh fenceMesh;
    Mesh pathMesh;

    std::vector<Vec3> treePositions;
    std::vector<float> treeScales;

    // Village objects
    std::vector<VillageObject> villageObjects;

    // Environment
    Vec3 ambientColor;
    Vec3 dirLightColor;
    Vec3 dirLightDir;
    Vec3 fogColor;
    float fogStart = 300.0f;
    float fogEnd = 800.0f;
    float fogDensity = 1.0f;

    bool flashlightOn = false;
    float flashlightIntensity = 0.0f;
    float flashlightBattery = 1.0f;

    float stamina = 1.0f;
    float fatigue = 0.0f;
    bool isSprinting = false;

    std::string objectiveText;
    std::string diaryText;
    float fadeAlpha = 1.0f;
    float stateTimer = 0.0f;

    bool wellRepaired = false;
    std::vector<Collectible> woodLogs;
    int logsCollected = 0;
    std::vector<Collectible> flowers;
    int flowersCollected = 0;
    Collectible oldTool;
    bool toolFound = false;
    std::vector<Collectible> stones;
    int stonesCollected = 0;
    Collectible rareHerb;
    bool herbFound = false;

    // Inputs
    bool keys[512] = {false};
    bool actionPressed = false;
    bool flashlightTogglePressed = false;
    bool languageCyclePressed = false;
    int languageSelectRequested = -1;

    bool touchActive = false;
    float leftJoyX = 150.0f, leftJoyY = 570.0f, leftRadius = 80.0f;
    float rightJoyX = 1130.0f, rightJoyY = 570.0f, rightRadius = 120.0f;
    float activeJoyX = 150.0f, activeJoyY = 570.0f;
    float activeCamX = 1130.0f, activeCamY = 570.0f;
    int walkTouchId = -1;
    int lookTouchId = -1;
    float lastTouchCamX = 0.0f;
    float lastTouchCamY = 0.0f;

    void init(int width, int height, bool mobileMode);
    void resize(int width, int height);
    void handleEvent(void* sdlEvent);
    void update(float deltaTime);
    void render();
    void cleanup();

private:
    void setupDaySettings();
    void updateTouchLayout();
    void spawnCollectibles();
    void generateVillage();
    void triggerNextDay();
    void resetPlayer();
    void drawBillboard(const Shader& shader, const Vec3& pos, float scaleX, float scaleY, int type, float opacity = 1.0f);
};