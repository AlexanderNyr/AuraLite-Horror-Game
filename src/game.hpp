#pragma once
#include <vector>
#include <string>
#include "math3d.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "ui.hpp"
#include "localization.hpp"
#include "audio.hpp"
#include "save_system.hpp"
#include "settings_system.hpp"

enum GameState {
    STATE_MENU,
    STATE_MENU_SETTINGS,
    STATE_INTRO,
    STATE_GAMEPLAY,
    STATE_SLEEP_FADE,
    STATE_ENDING
};

enum WeatherType {
    WEATHER_CLEAR,
    WEATHER_RAIN,
    WEATHER_SNOW,
    WEATHER_FOGGY
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

struct CircleCollider {
    Vec3 center;
    float radius = 1.0f;
    bool active = true;
};

struct Enemy {
    Vec3 pos;
    Vec3 target;
    int type = 0; // 0=shadow, 1=ghost
    float speed = 1.5f;
    bool active = true;
    float animTimer = 0.0f;
    float opacity = 0.0f;
    float stateTimer = 0.0f;
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
    GameSettings settings;

    int menuSelection = 0;
    int settingsSelection = 0;
    static constexpr int MENU_ITEM_COUNT = 4;
    static constexpr int SETTINGS_ITEM_COUNT = 4;


    Shader mainShader;
    Shader billboardShader;
    Shader depthShader;       // renders scene depth from the sun for shadow mapping
    Shader skyShader;         // full-screen procedural sky gradient + sun
    Texture grassTexture;
    Texture woodTexture;
    Texture pathTexture;
    Texture stoneTexture;
    Texture barkTexture;

    ShadowMap shadowMap;
    ShadowMap flashShadowMap;
    Mat4 lightSpaceMatrix;
    Mat4 flashLightSpaceMatrix;
    Mesh skyQuad;             // full-screen triangle for the sky pass
    void renderDepthPass();   // first pass: scene depth from the sun/moon
    void renderFlashlightDepthPass(); // optional spot-shadow depth from flashlight
    void renderSceneGeometry(Shader& sh, bool depthOnly); // shared geometry submission

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

    Mesh enemyMesh;
    Mesh ghostMesh;

    // === NEW: Village meshes ===
    Mesh houseMesh;
    Mesh rockMesh;
    Mesh fenceMesh;
    Mesh pathMesh;

    std::vector<Vec3> treePositions;
    std::vector<float> treeScales;

    // Village objects
    std::vector<VillageObject> villageObjects;

    // Collision geometry
    std::vector<AABB> aabbColliders;
    std::vector<CircleCollider> circleColliders;
    static constexpr float PLAYER_RADIUS = 0.4f;
    static constexpr float PLAYER_HEIGHT = 1.8f;

    // Enemies
    std::vector<Enemy> enemies;
    float fear = 0.0f;


    // Environment
    Vec3 ambientColor;
    Vec3 dirLightColor;
    Vec3 dirLightDir;
    Vec3 fogColor;
    Vec3 baseAmbientColor;
    Vec3 baseDirLightColor;
    Vec3 baseFogColor;

    // Celestial bodies driven by timeOfDay (computed in applyTimeOfDay)
    Vec3 sunDir   = {0.0f,-1.0f,0.0f}; // direction the sunlight travels
    Vec3 moonDir  = {0.0f, 1.0f,0.0f};
    float sunHeight  = 0.0f;           // sin of elevation, [-1..1]
    float moonHeight = 0.0f;
    float nightFactor = 0.0f;          // 0 = day, 1 = full night
    Vec3 zenithColor;                  // top of the sky dome
    Vec3 horizonColor;                 // bottom of the sky dome
    float fogStart = 300.0f;
    float fogEnd = 800.0f;
    float fogDensity = 1.0f;

    // Time of day / weather
    float timeOfDay = 12.0f;
    float timeScale = 0.5f; // real seconds per game hour
    WeatherType weather = WEATHER_CLEAR;
    float windStrength = 0.0f;
    std::vector<Vec3> rainParticles;
    std::vector<Vec3> snowParticles;
    Mesh rainMesh;
    Mesh snowMesh;
    static constexpr int RAIN_PARTICLE_COUNT = 800;
    static constexpr int SNOW_PARTICLE_COUNT = 600;

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

    bool newGameRequested = false;
    Frustum frustum;

    void syncCollectiblesWithSave();
    void saveProgress();
    void loadProgress();

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
    void buildColliders();
    void spawnEnemies();
    void updateEnemies(float dt);
    void renderEnemies();
    void setupWeather();
    void updateWeather(float dt);
    void renderWeather();
    void applyTimeOfDay();
    void buildWeatherMeshes();
    void updateWeatherMeshes();
    void triggerNextDay();
    void resetPlayer();
    void applySettings();
    void loadSettings();
    void saveSettings();
    void drawBillboard(const Shader& shader, const Vec3& pos, float scaleX, float scaleY, int type, float opacity = 1.0f);

    Vec3 resolveCollisions(const Vec3& oldPos, const Vec3& newPos) const;
    bool collidesAt(const Vec3& pos, float radius) const;

    void renderMenu();
    void renderSettings();
    void renderGameOver();
    void updateMenu(float dt);
    void updateSettings(float dt);
};