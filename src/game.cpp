#include "game.hpp"
#include "procedural.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "AnxietyHorror", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#endif

static float clamp(float val, float minVal, float maxVal) {
    return val < minVal ? minVal : (val > maxVal ? maxVal : val);
}

static uint32_t lcg_seed = 987654321;
static float randFloat() {
    lcg_seed = (lcg_seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)lcg_seed / (float)0x7fffffff;
}

void Game::syncCollectiblesWithSave() {
    for (size_t i = 0; i < woodLogs.size(); ++i) {
        woodLogs[i].collected = ((int)i < logsCollected);
    }
    for (size_t i = 0; i < flowers.size(); ++i) {
        flowers[i].collected = ((int)i < flowersCollected);
    }
    for (size_t i = 0; i < stones.size(); ++i) {
        stones[i].collected = ((int)i < stonesCollected);
    }
    oldTool.collected = toolFound;
    rareHerb.collected = herbFound;
}

void Game::saveProgress() {
    SaveData data;
    data.currentDay = currentDay;
    data.wellRepaired = wellRepaired;
    data.logsCollected = logsCollected;
    data.flowersCollected = flowersCollected;
    data.stonesCollected = stonesCollected;
    data.toolFound = toolFound;
    data.herbFound = herbFound;
    SaveSystem::saveGame(data);
}

void Game::loadProgress() {
    SaveData data;
    if (SaveSystem::loadGame(data)) {
        if (data.currentDay < 1 || data.currentDay > 9) data.currentDay = 1;
        currentDay = data.currentDay;
        wellRepaired = data.wellRepaired;
        logsCollected = data.logsCollected;
        flowersCollected = data.flowersCollected;
        stonesCollected = data.stonesCollected;
        toolFound = data.toolFound;
        herbFound = data.herbFound;
        syncCollectiblesWithSave();
    }
}

void Game::loadSettings() {
    if (SettingsSystem::loadSettings(settings)) {
        applySettings();
    } else {
        // Default settings match current defaults
        settings.languageIndex = 0;
        settings.mouseSensitivity = 0.005f;
        settings.viewDistance = 1.0f;
        applySettings();
    }
}

void Game::saveSettings() {
    SettingsSystem::saveSettings(settings);
}

void Game::applySettings() {
    settings.sanitize();

    // Language
    if (settings.languageIndex == 0) loc.loadLanguage("en");
    else if (settings.languageIndex == 1) loc.loadLanguage("ru");
    else if (settings.languageIndex == 2) loc.loadLanguage("es");

    // Mouse sensitivity
    camera.mouseSensitivity = settings.mouseSensitivity;

    // View distance multiplier affects fogEnd base used in setupDaySettings
    // We recompute current day fog if we are already in a day
    if (currentDay >= 1 && currentDay <= 9) {
        float dayProgress = (currentDay - 1) / 8.0f;
        fogStart = (280.0f - dayProgress * 160.0f) * settings.viewDistance;
        fogEnd   = (780.0f - dayProgress * 320.0f) * settings.viewDistance;
    }
}

void Game::init(int width, int height, bool mobileMode) {
    screenWidth = width;
    screenHeight = height;
    isAndroid = mobileMode;
    loc.init("lang");
    loadSettings();

    // === Improved Shaders with better lighting ===
    std::string vert3D = R"glsl(
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;
        layout(location = 3) in vec4 aColor;

        out vec3 FragPos; out vec3 Normal; out vec2 TexCoord; out vec4 Color;
        uniform mat4 model, view, projection;
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord; Color = aColor;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )glsl";

    std::string frag3D = R"glsl(
        in vec3 FragPos; in vec3 Normal; in vec2 TexCoord; in vec4 Color;
        out vec4 FragColor;

        uniform vec3 viewPos, fogColor, ambientColor, dirLightColor, dirLightDir;
        uniform float fogStart, fogEnd, time, fogDensity;
        uniform vec3 flashPos, flashDir; uniform float flashIntensity;
        uniform sampler2D texture_diffuse; uniform int useTexture;

        float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
        float valueNoise(vec2 p){
            vec2 i=floor(p),f=fract(p); vec2 u=f*f*(3.-2.*f);
            float a=hash(i),b=hash(i+vec2(1,0)),c=hash(i+vec2(0,1)),d=hash(i+vec2(1,1));
            return mix(mix(a,b,u.x),mix(c,d,u.x),u.y);
        }

        void main() {
            vec4 base = (useTexture!=0)? texture(texture_diffuse,TexCoord)*Color : Color;
            if(base.a<0.1) discard;

            vec3 norm=normalize(Normal), viewDir=normalize(viewPos-FragPos);
            vec3 albedo=pow(max(base.rgb,vec3(0.)),vec3(2.2));

            float hemi=norm.y*0.5+0.5;
            vec3 sky=ambientColor*mix(vec3(0.68,0.78,0.88),vec3(1.),hemi);
            vec3 ground=vec3(0.065,0.06,0.045)*(1.-hemi);
            vec3 ambient=(sky+ground)*albedo;

            vec3 ldir=normalize(-dirLightDir);
            float diff=max(dot(norm,ldir)+0.08,0.);
            vec3 diffuse=diff*dirLightColor*albedo;

            vec3 hdir=normalize(ldir+viewDir);
            float spec=pow(max(dot(norm,hdir),0.),32.)*0.14;
            vec3 specular=spec*dirLightColor;

            vec3 spotlight=vec3(0.);
            if(flashIntensity>0.){
                vec3 toFrag=normalize(FragPos-flashPos);
                if(dot(toFrag,normalize(flashDir))>0.6){
                    float dist=length(FragPos-flashPos);
                    float att=1./(1.+0.015*dist);
                    spotlight=vec3(1.,0.95,0.85)*att*flashIntensity*0.6;
                }
            }

            vec3 finalCol=ambient+diffuse+specular+spotlight;

            float dist=length(viewPos-FragPos);
            float baseFog=clamp((dist-fogStart)/max(0.001,(fogEnd-fogStart)),0.,1.);
            float noise=valueNoise(FragPos.xz*0.016+vec2(time*0.01))*0.15;
            float fogAmount=clamp(baseFog*fogDensity + noise*fogDensity*0.35,0.,1.);

            vec3 fogged=mix(finalCol,fogColor,fogAmount*0.8);
            fogged=pow(fogged,vec3(1./2.2));
            FragColor=vec4(fogged,base.a);
        }
    )glsl";

    mainShader.compile(vert3D, frag3D);

    std::string vertBill = R"glsl(
        layout(location=0)in vec3 aPos; layout(location=2)in vec2 aTexCoord;
        out vec2 TexCoord; uniform mat4 model,view,projection;
        void main(){TexCoord=aTexCoord; gl_Position=projection*view*model*vec4(aPos,1.);}
    )glsl";

    std::string fragBill = R"glsl(
        in vec2 TexCoord; out vec4 FragColor;
        uniform vec3 fogColor; uniform float fogStart,fogEnd,fogDensity;
        uniform vec3 itemPos; uniform int itemType; uniform float opacity;
        void main(){
            vec2 uv=TexCoord; vec4 col=vec4(0.);
            if(itemType==0){float d=length(uv-vec2(0.5));
                if(d<0.4) col=vec4(0.9,0.52,0.7,1.);
                if(length(uv-vec2(0.5))<0.12) col=vec4(1.,0.9,0.3,1.);
            }else if(itemType==1) col=vec4(0.55,0.5,0.4,1.);
            else if(itemType==2) col=vec4(0.45,0.6,0.4,1.);
            float dist=length(itemPos); 
            float f=clamp((fogEnd-dist)/(fogEnd-fogStart),0.2,1.);
            FragColor=vec4(mix(fogColor,col.rgb,f*fogDensity),col.a*opacity);
        }
    )glsl";
    billboardShader.compile(vertBill, fragBill);

    grassTexture.generateNoise(128,128,false);
    woodTexture.generateNoise(128,128,true);

    // === Larger open world terrain ===
    generateTerrain(terrainMesh, 520.0f, 520.0f, 180, 180);
    terrainMesh.upload();

    generateCabin(cabinMesh); cabinMesh.upload();
    generateTree(treeMesh); treeMesh.upload();

    // Precompute forest
    lcg_seed = 12345;
    treePositions.clear(); treeScales.clear();
    for(int i = 0; i < 420; ++i) {
        float x = randFloat()*260 - 130;
        float z = randFloat()*260 - 130;
        if (sqrt(x*x + z*z) < 18) continue;
        treePositions.push_back({x, getTerrainHeight(x,z), z});
        treeScales.push_back(0.65f + randFloat()*0.7f);
    }

    generateWell(wellMesh); wellMesh.upload();
    generateCar(carMesh); carMesh.upload();

    // === Generate Village ===
    generateHouse(houseMesh);
    houseMesh.upload();
    generateRock(rockMesh);
    rockMesh.upload();
    generateFence(fenceMesh, 12.0f);
    fenceMesh.upload();
    generatePath(pathMesh, 3.5f, 18.0f);
    pathMesh.upload();

    generateVillage();
    buildColliders();

    // Billboard quad
    billboardQuad.vertices = {
        {Vec3(-1,-1,0),Vec3(0,0,1),Vec2(0,0),{1,1,1,1}},
        {Vec3(1,-1,0),Vec3(0,0,1),Vec2(1,0),{1,1,1,1}},
        {Vec3(1,1,0),Vec3(0,0,1),Vec2(1,1),{1,1,1,1}},
        {Vec3(-1,1,0),Vec3(0,0,1),Vec2(0,1),{1,1,1,1}}
    };
    billboardQuad.indices={0,1,2,0,2,3};
    billboardQuad.upload();

    float logC[4]={0.7,0.55,0.3,1}; generateBox(logMesh,{0.3,0.3,2.0},{0,0,0},logC); logMesh.upload();
    float flC[4]={0.85,0.45,0.65,1}; generateBox(flowerMesh,{0.5,0.7,0.5},{0,0,0},flC); flowerMesh.upload();
    float stC[4]={0.5,0.48,0.45,1}; generateBox(stoneMesh,{0.6,0.42,0.6},{0,0,0},stC); stoneMesh.upload();
    float tlC[4]={0.6,0.55,0.45,1}; generateBox(toolMesh,{0.32,1.6,0.32},{0,0,0},tlC); toolMesh.upload();

    float enC[4]={0.05,0.05,0.06,1}; generateBox(enemyMesh,{1.0,2.2,0.5},{0,1.1f,0},enC); enemyMesh.upload();
    float ghC[4]={0.85,0.85,0.9,0.45}; generateCone(ghostMesh,0.7f,2.0f,8,{0,0,0},ghC); ghostMesh.upload();

    // Weather particle systems
    rainParticles.resize(RAIN_PARTICLE_COUNT);
    snowParticles.resize(SNOW_PARTICLE_COUNT);
    for (int i = 0; i < RAIN_PARTICLE_COUNT; ++i) {
        rainParticles[i] = {randFloat()*80 - 40, randFloat()*40 + 10, randFloat()*80 - 40};
    }
    for (int i = 0; i < SNOW_PARTICLE_COUNT; ++i) {
        snowParticles[i] = {randFloat()*80 - 40, randFloat()*40 + 10, randFloat()*80 - 40};
    }
    buildWeatherMeshes();

    ui.init(screenWidth, screenHeight);
    updateTouchLayout();
    spawnCollectibles();
    setupDaySettings();

    // Try loading existing progress if available
    loadProgress();

    state = STATE_MENU;
    resetPlayer();
}

void Game::resize(int w, int h) {
    screenWidth = std::max(1,w);
    screenHeight = std::max(1,h);
    ui.resize(screenWidth, screenHeight);
    updateTouchLayout();
}

void Game::updateTouchLayout() {
    leftRadius = std::max(55.f, screenHeight*0.1f);
    rightRadius = std::max(78.f, screenHeight*0.155f);
    leftJoyX = screenWidth*0.12f; leftJoyY = screenHeight*0.78f;
    rightJoyX = screenWidth*0.87f; rightJoyY = screenHeight*0.78f;
    if(walkTouchId==-1) activeJoyX=leftJoyX, activeJoyY=leftJoyY;
    if(lookTouchId==-1) activeCamX=rightJoyX, activeCamY=rightJoyY;
}

void Game::resetPlayer() {
    camera.position = {2.0f, 2.2f, 22.0f};
    camera.yaw = -M_PI/2.3f;
    camera.pitch = 0.0f;
    camera.updateCameraVectors();
    stamina = 1.0f; isSprinting = false;
    fear = 0.0f;
}

void Game::generateVillage() {
    villageObjects.clear();

    // Main village houses
    villageObjects.push_back({{-48, getTerrainHeight(-48,-62), -62}, 0, 1.0f});
    villageObjects.push_back({{52, getTerrainHeight(52,-58), -58}, 0, 0.95f});
    villageObjects.push_back({{-35, getTerrainHeight(-35,45), 45}, 0, 0.85f});

    // Rocks
    for(int i=0; i<9; i++) {
        float x = randFloat()*180-90;
        float z = randFloat()*180-90;
        if (fabs(x) < 25 && fabs(z) < 25) continue;
        villageObjects.push_back({{x, getTerrainHeight(x,z), z}, 1, 0.7f + randFloat()*0.6f});
    }

    // Fences around village
    villageObjects.push_back({{-55, getTerrainHeight(-55,-45), -45}, 2, 1.0f});
    villageObjects.push_back({{38, getTerrainHeight(38,-42), -42}, 2, 1.0f});
}

void Game::buildColliders() {
    aabbColliders.clear();
    circleColliders.clear();

    // Cabin: 12x10 footprint, 6.2 high
    aabbColliders.push_back({ Vec3(-5.8f, 0.0f, -4.8f), Vec3(5.8f, 6.2f, 4.8f) });

    // Well
    Vec3 wellPos = {-20.0f, getTerrainHeight(-20.0f, -50.0f), -50.0f};
    circleColliders.push_back({ wellPos, 3.0f });

    // Car
    Vec3 carPos = {12.0f, getTerrainHeight(12.0f, 78.0f), 78.0f};
    aabbColliders.push_back({ carPos + Vec3(-2.6f, 0.0f, -5.2f), carPos + Vec3(2.6f, 2.6f, 5.2f) });

    // Village houses, fences and rocks
    for (const auto& obj : villageObjects) {
        if (obj.type == 0) {
            // House body: 7.0 x 5.5 x 8.5, add a little margin
            aabbColliders.push_back({ obj.pos + Vec3(-3.6f, 0.0f, -4.4f), obj.pos + Vec3(3.6f, 5.7f, 4.4f) });
        } else if (obj.type == 1) {
            // Rocks
            circleColliders.push_back({ obj.pos, 1.5f * obj.scale });
        } else if (obj.type == 2) {
            // Fence segment: ~12.0 x 0.2 x 1.6
            aabbColliders.push_back({ obj.pos + Vec3(-6.2f, 0.0f, -0.2f), obj.pos + Vec3(6.2f, 1.7f, 0.2f) });
        }
    }

    // Trees
    for (const auto& pos : treePositions) {
        circleColliders.push_back({ pos, 1.2f });
    }
}

bool Game::collidesAt(const Vec3& pos, float radius) const {
    float yBottom = pos.y - PLAYER_HEIGHT * 0.5f;
    float yTop    = pos.y + PLAYER_HEIGHT * 0.5f;

    for (const auto& box : aabbColliders) {
        if (box.min.y > yTop || box.max.y < yBottom) continue;
        if (box.intersectsSphere(pos, radius)) return true;
    }

    for (const auto& c : circleColliders) {
        if (std::fabs(pos.y - c.center.y) > 3.0f) continue;
        float dx = pos.x - c.center.x;
        float dz = pos.z - c.center.z;
        if (std::sqrt(dx*dx + dz*dz) < c.radius + radius) return true;
    }

    return false;
}

Vec3 Game::resolveCollisions(const Vec3& oldPos, const Vec3& newPos) const {
    Vec3 result = newPos;
    result.x = clamp(result.x, -160.0f, 160.0f);
    result.z = clamp(result.z, -160.0f, 160.0f);

    // Keep the same height for horizontal collision tests; terrain height is applied afterwards
    Vec3 testPos = result;
    testPos.y = oldPos.y;

    // Try X movement alone
    Vec3 onlyX = testPos;
    onlyX.z = oldPos.z;
    if (collidesAt(onlyX, PLAYER_RADIUS)) {
        result.x = oldPos.x;
    }

    // Try Z movement from the (possibly adjusted) X position
    Vec3 onlyZ = result;
    onlyZ.y = oldPos.y;
    onlyZ.z = newPos.z;
    if (collidesAt(onlyZ, PLAYER_RADIUS)) {
        result.z = oldPos.z;
    }

    return result;
}

void Game::spawnEnemies() {
    enemies.clear();

    // More enemies and more ghosts as days progress
    int shadowCount = std::max(0, currentDay - 2);
    int ghostCount  = std::max(0, currentDay - 4);

    lcg_seed = (uint32_t)(currentDay * 1234567 + 42);

    for (int i = 0; i < shadowCount; ++i) {
        float x = randFloat() * 240.0f - 120.0f;
        float z = randFloat() * 240.0f - 120.0f;
        if (std::sqrt(x*x + z*z) < 30.0f) continue; // not too close to cabin
        Enemy e;
        e.pos = {x, getTerrainHeight(x,z), z};
        e.target = e.pos;
        e.type = 0;
        e.speed = 1.2f + randFloat() * 0.6f;
        e.active = true;
        enemies.push_back(e);
    }

    for (int i = 0; i < ghostCount; ++i) {
        float x = randFloat() * 260.0f - 130.0f;
        float z = randFloat() * 260.0f - 130.0f;
        if (std::sqrt(x*x + z*z) < 40.0f) continue;
        Enemy e;
        e.pos = {x, getTerrainHeight(x,z) + 0.5f, z};
        e.target = e.pos;
        e.type = 1;
        e.speed = 1.6f + randFloat() * 0.8f;
        e.active = true;
        e.opacity = 0.0f;
        enemies.push_back(e);
    }
}

void Game::updateEnemies(float dt) {
    // Fear regenerates slowly when no enemy is close
    bool fearSource = false;

    for (auto& e : enemies) {
        if (!e.active) continue;
        e.stateTimer += dt;

        float distToPlayer = (e.pos - camera.position).length();
        bool canSeePlayer = distToPlayer < (e.type == 0 ? 80.0f : 120.0f);

        if (canSeePlayer) {
            e.target = camera.position;
        } else if ((e.pos - e.target).length() < 2.0f || e.stateTimer > 6.0f) {
            // Pick a new random wander target
            e.target.x = e.pos.x + (randFloat() - 0.5f) * 60.0f;
            e.target.z = e.pos.z + (randFloat() - 0.5f) * 60.0f;
            e.target.y = getTerrainHeight(e.target.x, e.target.z);
            e.stateTimer = 0.0f;
        }

        Vec3 dir = e.target - e.pos;
        dir.y = 0.0f;
        float d = dir.length();
        if (d > 0.1f) {
            dir = dir / d;
            Vec3 newPos = e.pos + dir * e.speed * dt;
            newPos.y = getTerrainHeight(newPos.x, newPos.z) + (e.type == 0 ? 0.0f : 0.5f);
            e.pos = newPos;
        }

        // Ghost fade in/out based on distance
        if (e.type == 1) {
            float targetOpacity = 0.0f;
            if (distToPlayer < 60.0f) targetOpacity = 0.5f;
            if (distToPlayer < 25.0f) targetOpacity = 0.85f;
            e.opacity += (targetOpacity - e.opacity) * dt * 2.0f;
        } else {
            e.opacity = 1.0f;
        }

        // Fear accumulation
        if (distToPlayer < 4.0f) {
            fear += dt * 0.35f;
            fearSource = true;
        } else if (distToPlayer < 8.0f) {
            fear += dt * 0.08f;
            fearSource = true;
        }
    }

    if (!fearSource) {
        fear -= dt * 0.06f;
    }
    fear = clamp(fear, 0.0f, 1.0f);

    if (fear >= 1.0f) {
        state = STATE_ENDING; // reuse ending state with game over text
        stateTimer = 0.0f;
    }
}

void Game::renderEnemies() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& e : enemies) {
        if (!e.active) continue;
        if (!frustum.isSphereInside(e.pos, 3.0f)) continue;

        float dist = (e.pos - camera.position).length();
        if (dist > fogEnd * 1.2f) continue; // hidden by fog

        if (e.type == 0) {
            // Shadow: dark opaque box
            Mat4 m = Mat4::translation(e.pos);
            mainShader.setMat4("model", m);
            mainShader.setInt("useTexture", 0);
            enemyMesh.draw();
        } else {
            // Ghost: translucent cone, billboard-ish alpha
            Mat4 m = Mat4::translation(e.pos) * Mat4::scaling({0.9f});
            mainShader.setMat4("model", m);
            mainShader.setInt("useTexture", 0);
            ghostMesh.draw();
        }
    }

    glDisable(GL_BLEND);
}

void Game::renderGameOver() {
    float dk[4] = {0.01f, 0.01f, 0.015f, 1.0f};
    ui.drawRect(0, 0, screenWidth, screenHeight, dk);
    float t[4] = {0.85f, 0.25f, 0.25f, 1.0f};
    ui.drawText(loc.tr("gameover.title", "The Fog Took You"), screenWidth*0.5f - 220, screenHeight*0.35f, 3.2f, t);
    float txt[4] = {0.75f, 0.72f, 0.68f, 1.0f};
    ui.drawText(loc.tr("gameover.text", "Something in the mist found you.\nTry to stay farther from the shadows."),
                screenWidth*0.18f, screenHeight*0.55f, 1.5f, txt);
    float pr[4] = {0.6f, 0.62f, 0.58f, 0.85f + sin(stateTimer*2.5f)*0.15f};
    ui.drawText(loc.tr("gameover.restart", "Press ACTION to return to menu"), screenWidth*0.18f, screenHeight*0.82f, 1.35f, pr);
}

void Game::spawnCollectibles() {
    lcg_seed = 987654321;
    woodLogs.clear(); flowers.clear(); stones.clear();

    woodLogs = {
        {{14,0.4f,-9},false,"log"},
        {{-16,0.4f,14},false,"log"},
        {{11,0.4f,18},false,"log"}
    };

    for(int i=0; i<16; i++) {
        float x = randFloat()*200-100;
        float z = randFloat()*200-100;
        flowers.push_back({{x,getTerrainHeight(x,z)+0.2f,z},false,"flower"});
    }
    for(int i=0; i<8; i++) {
        float x = randFloat()*170-85;
        float z = randFloat()*170-85;
        stones.push_back({{x,getTerrainHeight(x,z)+0.1f,z},false,"stone"});
    }

    oldTool = {{-58, getTerrainHeight(-58,-28)+0.3f, -28}, false, "tool"};
    rareHerb = {{42, getTerrainHeight(42,38)+0.25f, 38}, false, "herb"};
}

void Game::setupDaySettings() {
    state = STATE_INTRO; stateTimer = 0; fadeAlpha = 1.0f;
    wellRepaired = false; logsCollected = 0; flowersCollected = 0;
    stonesCollected = 0; toolFound = false; herbFound = false;
    flashlightOn = false; flashlightIntensity = 0;

    float dayProgress = (currentDay - 1) / 8.0f;

    fogDensity = 0.6f + dayProgress * 1.45f;
    fogStart = (280.0f - dayProgress * 160.0f) * settings.viewDistance;
    fogEnd = (780.0f - dayProgress * 320.0f) * settings.viewDistance;
    fatigue = dayProgress * 0.58f;

    if (currentDay == 1) {
        baseAmbientColor = {0.38f,0.41f,0.35f}; baseDirLightColor = {1.0f,0.97f,0.88f};
        dirLightDir = {0.5f,-0.7f,-0.4f}; baseFogColor = {0.84f,0.9f,0.94f};
        objectiveText = loc.tr("d1.obj", "Explore the open valley and the nearby village.");
        diaryText = loc.tr("d1.diary", "DAY 1\n\nThe valley opens up before me.\nThere seems to be an old village nearby.\nI feel like staying here for a while.");
    }
    else if (currentDay == 2) {
        baseAmbientColor = {0.3f,0.34f,0.37f}; baseDirLightColor = {0.87f,0.9f,0.97f};
        dirLightDir = {0.33f,-0.85f,-0.24f}; baseFogColor = {0.77f,0.84f,0.9f};
        objectiveText = loc.tr("d2.obj", "Visit the old well near the village.");
        diaryText = loc.tr("d2.diary", "DAY 2\n\nThe village feels abandoned but peaceful.\nThe well might still be usable.");
    }
    else if (currentDay == 3) {
        baseAmbientColor = {0.24f,0.28f,0.26f}; baseDirLightColor = {0.94f,0.87f,0.78f};
        dirLightDir = {0.57f,-0.67f,-0.37f}; baseFogColor = {0.8f,0.84f,0.77f};
        objectiveText = loc.tr("d3.obj", "Gather firewood from the edges of the forest.");
        diaryText = loc.tr("d3.diary", "DAY 3\n\nThe evenings are getting colder.\nI should collect some wood.");
    }
    else if (currentDay == 4) {
        baseAmbientColor = {0.2f,0.24f,0.27f}; baseDirLightColor = {0.79f,0.83f,0.92f};
        dirLightDir = {0.24f,-0.89f,-0.14f}; baseFogColor = {0.67f,0.74f,0.8f};
        objectiveText = loc.tr("d4.obj", "Collect wildflowers growing near the village.");
        diaryText = loc.tr("d4.diary", "DAY 4\n\nThe fog is thicker today.\nThe flowers still bring some color to this place.");
    }
    else if (currentDay == 5) {
        baseAmbientColor = {0.17f,0.21f,0.24f}; baseDirLightColor = {0.73f,0.77f,0.86f};
        dirLightDir = {0.36f,-0.81f,-0.29f}; baseFogColor = {0.59f,0.65f,0.72f};
        objectiveText = loc.tr("d5.obj", "Search the area west of the village for useful tools.");
        diaryText = loc.tr("d5.diary", "DAY 5\n\nI should look around the old houses.\nThere might be something left behind.");
    }
    else if (currentDay == 6) {
        baseAmbientColor = {0.14f,0.18f,0.21f}; baseDirLightColor = {0.63f,0.68f,0.77f};
        dirLightDir = {0.29f,-0.84f,-0.22f}; baseFogColor = {0.53f,0.58f,0.64f};
        objectiveText = loc.tr("d6.obj", "Collect stones to reinforce paths around the cabin.");
        diaryText = loc.tr("d6.diary", "DAY 6\n\nThe fog stays longer every day.\nMaking the paths clearer would help.");
    }
    else if (currentDay == 7) {
        baseAmbientColor = {0.12f,0.15f,0.18f}; baseDirLightColor = {0.56f,0.61f,0.71f};
        dirLightDir = {0.38f,-0.82f,-0.26f}; baseFogColor = {0.46f,0.51f,0.57f};
        objectiveText = loc.tr("d7.obj", "Find rare herbs in the deeper parts of the woods.");
        diaryText = loc.tr("d7.diary", "DAY 7\n\nI feel more tired with each passing day.\nSome herbs might help.");
    }
    else if (currentDay == 8) {
        baseAmbientColor = {0.1f,0.13f,0.16f}; baseDirLightColor = {0.49f,0.54f,0.63f};
        dirLightDir = {0.26f,-0.87f,-0.19f}; baseFogColor = {0.41f,0.45f,0.51f};
        objectiveText = loc.tr("d8.obj", "Finish collecting everything needed.");
        diaryText = loc.tr("d8.diary", "DAY 8\n\nThe valley feels more distant.\nI need to complete my tasks.");
    }
    else if (currentDay == 9) {
        baseAmbientColor = {0.09f,0.11f,0.14f}; baseDirLightColor = {0.43f,0.48f,0.56f};
        dirLightDir = {0.31f,-0.84f,-0.23f}; baseFogColor = {0.36f,0.39f,0.45f};
        objectiveText = loc.tr("d9.obj", "Return to the cabin. Your journey is ending.");
        diaryText = loc.tr("d9.diary", "DAY 9\n\nAfter nine days the valley feels\nboth familiar and strangely quiet.\nIt is time to leave.");
    }

    timeOfDay = 8.0f; // each day starts at morning
    spawnEnemies();
    setupWeather();
    applyTimeOfDay();
}

void Game::handleEvent(void* e) {
    SDL_Event* ev = (SDL_Event*)e;
    if (ev->type == SDL_KEYDOWN) {
        if (ev->key.keysym.scancode < 512) keys[ev->key.keysym.scancode] = true;
        if ((ev->key.keysym.sym == SDLK_e || ev->key.keysym.sym == SDLK_RETURN) && ev->key.repeat == 0)
            actionPressed = true;
        if (ev->key.keysym.sym == SDLK_f && ev->key.repeat == 0)
            flashlightTogglePressed = true;
        if (ev->key.keysym.sym == SDLK_F2 && ev->key.repeat == 0)
            languageCyclePressed = true;
        if (ev->key.keysym.sym == SDLK_n && ev->key.repeat == 0)
            newGameRequested = true;
    } else if (ev->type == SDL_KEYUP) {
        if (ev->key.keysym.scancode < 512) keys[ev->key.keysym.scancode] = false;
    } else if (ev->type == SDL_MOUSEMOTION && state == STATE_GAMEPLAY && !isAndroid) {
        camera.processMouseMovement(ev->motion.xrel, -ev->motion.yrel);
    } else if (ev->type == SDL_FINGERDOWN) {
        float fx = ev->tfinger.x * screenWidth, fy = ev->tfinger.y * screenHeight;
        long long id = ev->tfinger.fingerId;
        // Flashlight button (top-right)
        if (fx > screenWidth-150 && fx < screenWidth-40 && fy > 95 && fy < 205)
            flashlightTogglePressed = true;
        // Action button (bottom-right)
        else if (fx > screenWidth-150 && fx < screenWidth-40 && fy > screenHeight-310 && fy < screenHeight-200)
            actionPressed = true;
        else if (fx < screenWidth * 0.5f) { walkTouchId = id; activeJoyX = fx; activeJoyY = fy; }
        else { lookTouchId = id; lastTouchCamX = fx; lastTouchCamY = fy; }
    } else if (ev->type == SDL_FINGERMOTION) {
        float fx = ev->tfinger.x * screenWidth, fy = ev->tfinger.y * screenHeight;
        long long id = ev->tfinger.fingerId;
        if (id == walkTouchId) { activeJoyX = fx; activeJoyY = fy; }
        else if (id == lookTouchId) {
            camera.processMouseMovement((fx-lastTouchCamX)*0.3f, -(fy-lastTouchCamY)*0.3f);
            lastTouchCamX = fx; lastTouchCamY = fy;
        }
    } else if (ev->type == SDL_FINGERUP) {
        long long id = ev->tfinger.fingerId;
        if (id == walkTouchId) walkTouchId = -1;
        else if (id == lookTouchId) lookTouchId = -1;
    }
}

void Game::triggerNextDay() {
    currentDay++;
    if (currentDay > 9) currentDay = 1;
    setupDaySettings();
    resetPlayer();
    syncCollectiblesWithSave();
    saveProgress();
    audio.playSound("intro");
}

void Game::update(float dt) {
    if (dt > 0.12f) dt = 0.12f;
    stateTimer += dt;

    if (languageSelectRequested == 0) loc.loadLanguage("en");
    if (languageSelectRequested == 1) loc.loadLanguage("ru");
    if (languageSelectRequested == 2) loc.loadLanguage("es");
    languageSelectRequested = -1;
    if (languageCyclePressed) { loc.cycleLanguage(); languageCyclePressed = false; }

    if (state == STATE_MENU) {
        updateMenu(dt);
        return;
    }
    if (state == STATE_MENU_SETTINGS) {
        updateSettings(dt);
        return;
    }
    if (state == STATE_INTRO) {
        if (fadeAlpha > 0) fadeAlpha -= dt * 0.55f;
        if (actionPressed) { actionPressed = false; state = STATE_GAMEPLAY; fadeAlpha = 0; }
        return;
    }
    if (state == STATE_SLEEP_FADE) {
        fadeAlpha += dt * 0.85f;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            if (stateTimer > 1.7f) triggerNextDay();
        }
        return;
    }
    if (state == STATE_ENDING) {
        if (actionPressed) {
            actionPressed = false;
            currentDay = 1;
            setupDaySettings();
            resetPlayer();
            state = STATE_INTRO;
        }
        return;
    }

    const bool interact = actionPressed || keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_RETURN];
    actionPressed = false;

    bool kMove = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D];
    float jx = activeJoyX - leftJoyX, jy = activeJoyY - leftJoyY, jd = sqrt(jx*jx + jy*jy);
    bool mMove = isAndroid && walkTouchId != -1 && jd > 12;

    isSprinting = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ||
                  (mMove && jd > leftRadius*0.78f)) && stamina > 0.12f;
    camera.movementSpeed = isSprinting ? 7.2f : 4.1f;

    // Build horizontal movement vector from keyboard and/or virtual joystick
    Vec3 moveDir(0.0f, 0.0f, 0.0f);
    Vec3 hf = {camera.front.x, 0.0f, camera.front.z}; hf = hf.normalized();
    Vec3 hr = {camera.right.x, 0.0f, camera.right.z}; hr = hr.normalized();

    if (keys[SDL_SCANCODE_W]) moveDir += hf;
    if (keys[SDL_SCANCODE_S]) moveDir -= hf;
    if (keys[SDL_SCANCODE_A]) moveDir -= hr;
    if (keys[SDL_SCANCODE_D]) moveDir += hr;

    if (mMove) {
        float vx = jx / jd, vy = jy / jd;
        moveDir -= hf * vy;
        moveDir += hr * vx;
    }

    if (moveDir.length() > 0.0001f) {
        moveDir = moveDir.normalized();
        Vec3 newPos = camera.position + moveDir * camera.movementSpeed * dt;
        newPos = resolveCollisions(camera.position, newPos);
        camera.position = newPos;
    }

    float fatiguePenalty = fatigue * 0.38f;
    if (isSprinting) stamina = clamp(stamina - dt*(0.2f + fatiguePenalty*0.45f), 0, 1);
    else stamina = clamp(stamina + dt*(0.23f - fatiguePenalty), 0, 1);

    camera.position.x = clamp(camera.position.x, -160.0f, 160.0f);
    camera.position.z = clamp(camera.position.z, -160.0f, 160.0f);
    camera.position.y = getTerrainHeight(camera.position.x, camera.position.z) + 1.8f;

    if (flashlightTogglePressed) {
        flashlightOn = !flashlightOn;
        flashlightTogglePressed = false;
    }
    flashlightIntensity = flashlightOn ? 0.85f : 0;

    // Time of day and weather progression
    timeOfDay += dt * timeScale;
    if (timeOfDay >= 24.0f) timeOfDay -= 24.0f;
    applyTimeOfDay();
    updateWeather(dt);

    updateEnemies(dt);

    float dCabin = sqrt(camera.position.x*camera.position.x + camera.position.z*camera.position.z);

    // === 9 Days Objectives ===
    if (currentDay == 1) {
        objectiveText = loc.tr("d1.explore", "Explore the valley and the old village.");
        if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 2) {
        float dw = sqrt((camera.position.x+20)*(camera.position.x+20)+(camera.position.z+50)*(camera.position.z+50));
        if (!wellRepaired) {
            objectiveText = loc.tr("d2.check", "Inspect the well near the village.");
            if (dw < 5.5f && interact) { wellRepaired = true; audio.playSound("repair"); saveProgress(); }
        } else if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 3) {
        for (auto& l : woodLogs) if (!l.collected && (camera.position - l.pos).length() < 3.3f && interact) {
            l.collected = true; logsCollected++; audio.playSound("pickup"); saveProgress();
        }
        if (logsCollected < 3) objectiveText = loc.tr("d3.progress", "Collect firewood") + " (" + std::to_string(logsCollected) + "/3)";
        else if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 4) {
        for (auto& f : flowers) if (!f.collected && (camera.position - f.pos).length() < 2.9f && interact) {
            f.collected = true; flowersCollected++; audio.playSound("pickup"); saveProgress();
        }
        if (flowersCollected < 8) objectiveText = loc.tr("d4.progress", "Gather flowers") + " (" + std::to_string(flowersCollected) + "/8)";
        else if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 5) {
        float dt = (camera.position - oldTool.pos).length();
        if (!toolFound) {
            objectiveText = loc.tr("d5.search", "Search west of the village for tools.");
            if (dt < 4.5f && interact) { toolFound = true; audio.playSound("pickup"); saveProgress(); }
        } else if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 6) {
        for (auto& s : stones) if (!s.collected && (camera.position - s.pos).length() < 3.1f && interact) {
            s.collected = true; stonesCollected++; audio.playSound("pickup"); saveProgress();
        }
        if (stonesCollected < 6) objectiveText = loc.tr("d6.progress", "Collect stones") + " (" + std::to_string(stonesCollected) + "/6)";
        else if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 7) {
        float dh = (camera.position - rareHerb.pos).length();
        if (!herbFound) {
            objectiveText = loc.tr("d7.herb", "Find rare herbs in the deeper forest.");
            if (dh < 4.2f && interact) { herbFound = true; audio.playSound("pickup"); saveProgress(); }
        } else if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 8) {
        objectiveText = loc.tr("d8.final", "Finish gathering resources.");
        if (dCabin < 4.2f && interact) { state = STATE_SLEEP_FADE; stateTimer = 0; audio.playSound("sleep"); }
    }
    else if (currentDay == 9) {
        objectiveText = loc.tr("d9.end", "Return to the cabin to end your journey.");
        if (dCabin < 4.2f && interact) { state = STATE_ENDING; stateTimer = 0; audio.playSound("sleep"); }
    }

    audio.update(fogDensity, currentDay, kMove || mMove, isSprinting, stamina, fatigue);
}

void Game::drawBillboard(const Shader& sh, const Vec3& pos, float sx, float sy, int type, float op) {
    sh.use();
    Vec3 dir = camera.position - pos;
    float ang = atan2(dir.x, dir.z);
    Mat4 m = Mat4::translation(pos) * Mat4::rotationY(ang) * Mat4::scaling({sx, sy, 1});
    sh.setMat4("model", m);
    sh.setMat4("view", camera.getViewMatrix());
    float r = float(screenWidth) / screenHeight;
    sh.setMat4("projection", Mat4::perspective(45*M_PI/180, r, 0.1f, 1100));
    sh.setVec3("itemPos", pos);
    sh.setInt("itemType", type);
    sh.setFloat("opacity", op);
    sh.setFloat("fogDensity", fogDensity);
    billboardQuad.draw();
}

void Game::render() {
    glClearColor(fogColor.x, fogColor.y, fogColor.z, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (state == STATE_MENU) {
        renderMenu();
        return;
    }
    if (state == STATE_MENU_SETTINGS) {
        renderSettings();
        return;
    }

    if (state == STATE_INTRO) {
        float blk[4]={0.03f,0.035f,0.045f,1};
        ui.drawRect(0,0,screenWidth,screenHeight,blk);
        float tc[4]={0.9f,0.92f,0.87f,1};
        ui.drawText(diaryText, screenWidth*0.08f, screenHeight*0.18f, 1.48f, tc);
        float pr[4]={0.7f,0.74f,0.68f,0.82f + sin(stateTimer*3.1f)*0.2f};
        ui.drawText(loc.tr("intro.prompt","Press ACTION to continue..."), screenWidth*0.08f, screenHeight*0.82f, 1.35f, pr);
        return;
    }

    if (state == STATE_ENDING) {
        if (fear >= 0.99f) {
            renderGameOver();
        } else {
            float dk[4]={0.02f,0.025f,0.03f,1};
            ui.drawRect(0,0,screenWidth,screenHeight,dk);
            float gd[4]={0.95f,0.92f,0.8f,1};
            ui.drawText(loc.tr("end.title","The Valley Remembers"), screenWidth*0.5f-160, screenHeight*0.28f, 3.2f, gd);
            float endTextColor[4] = {0.87f, 0.89f, 0.83f, 1.0f};
            ui.drawText(loc.tr("end.text","After nine days the valley feels\nboth familiar and strangely quiet.\nYou leave changed, but at peace."),
                        screenWidth*0.16f, screenHeight*0.45f, 1.55f, endTextColor);
        }
        return;
    }

    // === 3D RENDER ===
    mainShader.use();
    Mat4 viewMat = camera.getViewMatrix();
    mainShader.setMat4("view", viewMat);
    float ratio = float(screenWidth)/screenHeight;
    Mat4 projMat = Mat4::perspective(45*M_PI/180, ratio, 0.1f, 1100);
    mainShader.setMat4("projection", projMat);
    
    // Extract Frustum for culling
    Mat4 vpMat = projMat * viewMat;
    frustum.extract(vpMat);

    mainShader.setVec3("viewPos", camera.position);
    mainShader.setVec3("fogColor", fogColor);
    mainShader.setFloat("fogStart", fogStart);
    mainShader.setFloat("fogEnd", fogEnd);
    mainShader.setFloat("fogDensity", fogDensity);
    mainShader.setFloat("time", stateTimer);
    mainShader.setVec3("ambientColor", ambientColor);
    mainShader.setVec3("dirLightColor", dirLightColor);
    mainShader.setVec3("dirLightDir", dirLightDir);
    mainShader.setVec3("flashPos", camera.position);
    mainShader.setVec3("flashDir", camera.front);
    mainShader.setFloat("flashIntensity", flashlightIntensity);

    // Terrain
    mainShader.setMat4("model", Mat4::identity());
    mainShader.setInt("useTexture", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTexture.id);
    terrainMesh.draw();

    // Cabin
    mainShader.setMat4("model", Mat4::identity());
    mainShader.setInt("useTexture", 1);
    glBindTexture(GL_TEXTURE_2D, woodTexture.id);
    cabinMesh.draw();

    // Village Houses
    for (const auto& obj : villageObjects) {
        if (obj.type == 0) {
            if (!frustum.isSphereInside(obj.pos, 12.0f)) continue;
            Mat4 hm = Mat4::translation(obj.pos) * Mat4::scaling({obj.scale, obj.scale, obj.scale});
            mainShader.setMat4("model", hm);
            mainShader.setInt("useTexture", 0);
            houseMesh.draw();
        }
    }

    // Well
    Vec3 wellPos = {-20, getTerrainHeight(-20,-50), -50};
    if (frustum.isSphereInside(wellPos, 6.0f)) {
        Mat4 wm = Mat4::translation(wellPos);
        mainShader.setMat4("model", wm);
        mainShader.setInt("useTexture", 0);
        wellMesh.draw();
    }

    // Car
    Vec3 carPos = {12, getTerrainHeight(12,78), 78};
    if (frustum.isSphereInside(carPos, 8.0f)) {
        Mat4 cm = Mat4::translation(carPos) * Mat4::rotationY(M_PI/4);
        mainShader.setMat4("model", cm);
        carMesh.draw();
    }

    // Trees
    mainShader.setInt("useTexture", 0);
    for (size_t i = 0; i < treePositions.size(); ++i) {
        if (!frustum.isSphereInside(treePositions[i], 9.0f)) continue;
        Mat4 tm = Mat4::translation(treePositions[i]) * Mat4::scaling({treeScales[i]});
        mainShader.setMat4("model", tm);
        treeMesh.draw();
    }

    // Village Rocks & Fences
    for (const auto& obj : villageObjects) {
        if (obj.type == 1) {
            if (!frustum.isSphereInside(obj.pos, 5.0f)) continue;
            Mat4 rm = Mat4::translation(obj.pos) * Mat4::scaling({obj.scale});
            mainShader.setMat4("model", rm);
            rockMesh.draw();
        } else if (obj.type == 2) {
            if (!frustum.isSphereInside(obj.pos, 10.0f)) continue;
            Mat4 fm = Mat4::translation(obj.pos);
            mainShader.setMat4("model", fm);
            fenceMesh.draw();
        }
    }

    // Collectibles
    if (currentDay >= 3) for (const auto& l : woodLogs) if (!l.collected) {
        Mat4 lm = Mat4::translation(l.pos) * Mat4::rotationX(M_PI/2) * Mat4::scaling({0.5f});
        mainShader.setMat4("model", lm);
        logMesh.draw();
    }
    if (currentDay >= 4) for (const auto& f : flowers) if (!f.collected) {
        Mat4 fm = Mat4::translation(f.pos) * Mat4::scaling({0.85f});
        mainShader.setMat4("model", fm);
        flowerMesh.draw();
    }
    if (currentDay >= 6) for (const auto& s : stones) if (!s.collected) {
        Mat4 sm = Mat4::translation(s.pos) * Mat4::scaling({0.75f});
        mainShader.setMat4("model", sm);
        stoneMesh.draw();
    }
    if (currentDay >= 5 && !toolFound) {
        Mat4 tm = Mat4::translation(oldTool.pos) * Mat4::scaling({0.72f});
        mainShader.setMat4("model", tm);
        toolMesh.draw();
    }
    if (currentDay >= 7 && !herbFound) {
        Mat4 hm = Mat4::translation(rareHerb.pos) * Mat4::scaling({0.68f});
        mainShader.setMat4("model", hm);
        toolMesh.draw();
    }

    // Enemies
    renderEnemies();

    // Weather particles
    renderWeather();

    // Billboards
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    if (currentDay >= 4) for (const auto& f : flowers) if (!f.collected) {
        if (!frustum.isSphereInside(f.pos, 1.5f)) continue;
        float d = (camera.position - f.pos).length();
        if (d < 55) drawBillboard(billboardShader, f.pos, 1.0f, 1.2f, 0);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // === UI ===
    float tc[4] = {0.96f,0.97f,0.93f,1};
    std::stringstream ds; ds << "Day " << currentDay << " / 9";
    ui.drawText(ds.str(), 22, 22, 2.1f, tc);

    // Time and weather display
    int hours = (int)timeOfDay;
    int mins = (int)((timeOfDay - hours) * 60.0f);
    std::stringstream ts;
    ts << (hours < 10 ? "0" : "") << hours << ":" << (mins < 10 ? "0" : "") << mins;
    ui.drawText(ts.str(), screenWidth - 110, 22, 1.5f, tc);

    const char* weatherNames[4] = {"weather.clear", "weather.rain", "weather.snow", "weather.foggy"};
    const char* weatherFallbacks[4] = {"Clear", "Rain", "Snow", "Fog"};
    ui.drawText(loc.tr(weatherNames[weather], weatherFallbacks[weather]), screenWidth - 110, 44, 1.2f, tc);

    ui.drawText(objectiveText, 22, 58, 1.2f, tc);

    // Stamina + Fatigue
    float sbg[4] = {0.07f,0.07f,0.07f,0.55f};
    ui.drawRect(22, screenHeight-42, 195, 11, sbg);
    float sCol[4] = {0.38f + fatigue*0.28f, 0.82f - fatigue*0.38f, 0.42f, 0.82f};
    ui.drawRect(26, screenHeight-39, 187*stamina, 5.5f, sCol);

    if (fatigue > 0.12f) {
        float fatCol[4] = {0.52f, 0.32f, 0.22f, 0.6f};
        ui.drawRect(22, screenHeight-58, 195*fatigue, 4, fatCol);
    }

    // Fear bar (appears when enemies are active)
    if (currentDay >= 3 && fear > 0.01f) {
        float fbg[4] = {0.07f,0.07f,0.07f,0.55f};
        ui.drawRect(22, screenHeight-74, 195, 8, fbg);
        float fCol[4] = {0.85f, 0.18f, 0.18f, 0.85f};
        ui.drawRect(26, screenHeight-72, 187*fear, 4, fCol);
    }

    if (currentDay >= 5) {
        float lc[4];
        if (flashlightOn) {
            lc[0] = 0.9f; lc[1] = 0.86f; lc[2] = 0.52f; lc[3] = 0.82f;
        } else {
            lc[0] = 0.32f; lc[1] = 0.32f; lc[2] = 0.32f; lc[3] = 0.5f;
        }
        ui.drawRect(screenWidth-155, 88, 108, 36, lc);
    }

    ui.drawVirtualJoysticks(leftJoyX, leftJoyY, leftRadius, activeJoyX, activeJoyY,
                            rightJoyX, rightJoyY, rightRadius, activeCamX, activeCamY,
                            isAndroid, flashlightOn, "ACTION");

    if (state == STATE_SLEEP_FADE) {
        float blk[4] = {0,0,0,fadeAlpha};
        ui.drawRect(0,0,screenWidth,screenHeight,blk);
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
    houseMesh.cleanup();
    rockMesh.cleanup();
    fenceMesh.cleanup();
    pathMesh.cleanup();
    billboardQuad.cleanup();
    logMesh.cleanup();
    flowerMesh.cleanup();
    stoneMesh.cleanup();
    toolMesh.cleanup();
    enemyMesh.cleanup();
    ghostMesh.cleanup();
    rainMesh.cleanup();
    snowMesh.cleanup();
    ui.cleanup();
}

// ==================== WEATHER & TIME OF DAY ====================

void Game::buildWeatherMeshes() {
    // Rain: line segments
    rainMesh.vertices.clear();
    rainMesh.indices.clear();
    float rainColor[4] = {0.65f, 0.72f, 0.85f, 0.55f};
    for (int i = 0; i < RAIN_PARTICLE_COUNT; ++i) {
        Vertex v1, v2;
        v1.normal = {0,1,0}; v1.uv = {0,0};
        v2.normal = {0,1,0}; v2.uv = {0,0};
        for (int c = 0; c < 4; ++c) { v1.color[c] = rainColor[c]; v2.color[c] = rainColor[c]; }
        v1.pos = {0,0,0};
        v2.pos = {0,-0.8f,0};
        rainMesh.vertices.push_back(v1);
        rainMesh.vertices.push_back(v2);
        rainMesh.indices.push_back((unsigned int)(i*2));
        rainMesh.indices.push_back((unsigned int)(i*2+1));
    }
    rainMesh.upload();

    // Snow: small quads
    snowMesh.vertices.clear();
    snowMesh.indices.clear();
    float snowColor[4] = {0.95f, 0.95f, 1.0f, 0.8f};
    for (int i = 0; i < SNOW_PARTICLE_COUNT; ++i) {
        unsigned int base = (unsigned int)snowMesh.vertices.size();
        Vertex v;
        v.normal = {0,1,0}; v.uv = {0,0};
        for (int c = 0; c < 4; ++c) v.color[c] = snowColor[c];
        float s = 0.08f;
        v.pos = {-s,-s,0}; snowMesh.vertices.push_back(v);
        v.pos = { s,-s,0}; snowMesh.vertices.push_back(v);
        v.pos = { s, s,0}; snowMesh.vertices.push_back(v);
        v.pos = {-s, s,0}; snowMesh.vertices.push_back(v);
        snowMesh.indices.push_back(base+0); snowMesh.indices.push_back(base+1); snowMesh.indices.push_back(base+2);
        snowMesh.indices.push_back(base+0); snowMesh.indices.push_back(base+2); snowMesh.indices.push_back(base+3);
    }
    snowMesh.upload();
}

void Game::updateWeatherMeshes() {
    if (weather == WEATHER_RAIN) {
        for (int i = 0; i < RAIN_PARTICLE_COUNT; ++i) {
            Vec3 world = rainParticles[i] + camera.position;
            rainMesh.vertices[i*2].pos = world;
            rainMesh.vertices[i*2+1].pos = world + Vec3(0,-0.8f,0);
        }
        glBindBuffer(GL_ARRAY_BUFFER, rainMesh.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rainMesh.vertices.size() * sizeof(Vertex), rainMesh.vertices.data());
    }
    else if (weather == WEATHER_SNOW) {
        for (int i = 0; i < SNOW_PARTICLE_COUNT; ++i) {
            Vec3 world = snowParticles[i] + camera.position;
            float s = 0.08f;
            snowMesh.vertices[i*4+0].pos = world + Vec3(-s,-s,0);
            snowMesh.vertices[i*4+1].pos = world + Vec3( s,-s,0);
            snowMesh.vertices[i*4+2].pos = world + Vec3( s, s,0);
            snowMesh.vertices[i*4+3].pos = world + Vec3(-s, s,0);
        }
        glBindBuffer(GL_ARRAY_BUFFER, snowMesh.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, snowMesh.vertices.size() * sizeof(Vertex), snowMesh.vertices.data());
    }
}

void Game::setupWeather() {
    // Save the day colors as the base palette
    baseAmbientColor = ambientColor;
    baseDirLightColor = dirLightColor;
    baseFogColor = fogColor;

    // Choose weather based on day progression
    lcg_seed = (uint32_t)(currentDay * 7654321 + 123);
    float r = randFloat();
    if (currentDay <= 2) {
        weather = WEATHER_CLEAR;
    } else if (currentDay <= 4) {
        weather = (r < 0.6f) ? WEATHER_FOGGY : WEATHER_CLEAR;
    } else if (currentDay <= 6) {
        if (r < 0.5f) weather = WEATHER_RAIN;
        else if (r < 0.8f) weather = WEATHER_FOGGY;
        else weather = WEATHER_CLEAR;
    } else {
        if (r < 0.35f) weather = WEATHER_SNOW;
        else if (r < 0.7f) weather = WEATHER_RAIN;
        else weather = WEATHER_FOGGY;
    }
    windStrength = 1.0f + randFloat() * 2.0f;

    // Apply weather modifiers to fog
    if (weather == WEATHER_FOGGY) {
        fogDensity *= 1.4f;
        fogStart *= 0.7f;
    } else if (weather == WEATHER_RAIN) {
        fogDensity *= 1.15f;
        baseFogColor = Vec3::lerp(baseFogColor, Vec3(0.5f,0.55f,0.6f), 0.3f);
    } else if (weather == WEATHER_SNOW) {
        fogDensity *= 1.25f;
        baseFogColor = Vec3::lerp(baseFogColor, Vec3(0.85f,0.88f,0.92f), 0.4f);
    }
}

void Game::applyTimeOfDay() {
    float t = timeOfDay;
    float dayFactor = 0.0f;
    if (t < 5.0f) dayFactor = 0.0f;
    else if (t < 8.0f) dayFactor = (t - 5.0f) / 3.0f;
    else if (t < 17.0f) dayFactor = 1.0f;
    else if (t < 21.0f) dayFactor = 1.0f - (t - 17.0f) / 4.0f;
    else dayFactor = 0.0f;

    ambientColor = Vec3::lerp(baseAmbientColor * 0.25f, baseAmbientColor, dayFactor);
    dirLightColor = Vec3::lerp(baseDirLightColor * 0.15f, baseDirLightColor, dayFactor);
    fogColor = Vec3::lerp(baseFogColor * 0.35f, baseFogColor, dayFactor);
}

void Game::updateWeather(float dt) {
    if (weather == WEATHER_RAIN) {
        for (auto& p : rainParticles) {
            p.y -= (22.0f + windStrength * 2.0f) * dt;
            p.x += windStrength * 0.3f * dt;
            if (p.y < -8.0f) {
                p.y = 30.0f + randFloat() * 10.0f;
                p.x = randFloat() * 80.0f - 40.0f;
                p.z = randFloat() * 80.0f - 40.0f;
            }
        }
        updateWeatherMeshes();
    }
    else if (weather == WEATHER_SNOW) {
        for (auto& p : snowParticles) {
            p.y -= (2.0f + windStrength * 0.5f) * dt;
            p.x += (windStrength + sinf(p.y * 0.5f + stateTimer) * 1.5f) * dt;
            p.z += cosf(p.y * 0.3f + stateTimer * 0.7f) * dt * 0.5f;
            if (p.y < -8.0f) {
                p.y = 30.0f + randFloat() * 10.0f;
                p.x = randFloat() * 80.0f - 40.0f;
                p.z = randFloat() * 80.0f - 40.0f;
            }
        }
        updateWeatherMeshes();
    }
}

void Game::renderWeather() {
    if (weather == WEATHER_RAIN) {
        mainShader.use();
        mainShader.setMat4("model", Mat4::identity());
        mainShader.setMat4("view", camera.getViewMatrix());
        float r = float(screenWidth) / screenHeight;
        mainShader.setMat4("projection", Mat4::perspective(45*M_PI/180, r, 0.1f, 1100));
        mainShader.setInt("useTexture", 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        rainMesh.draw();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    else if (weather == WEATHER_SNOW) {
        mainShader.use();
        mainShader.setMat4("model", Mat4::identity());
        mainShader.setMat4("view", camera.getViewMatrix());
        float r = float(screenWidth) / screenHeight;
        mainShader.setMat4("projection", Mat4::perspective(45*M_PI/180, r, 0.1f, 1100));
        mainShader.setInt("useTexture", 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        snowMesh.draw();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

// ==================== MENU & SETTINGS ====================

void Game::updateMenu(float dt) {
    if (languageCyclePressed) {
        loc.cycleLanguage();
        languageCyclePressed = false;
    }

    // Navigation: W/S or Up/Down
    static float navCooldown = 0.0f;
    navCooldown -= dt;
    bool up = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    bool down = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];

    if (navCooldown <= 0.0f && (up || down)) {
        if (up) menuSelection = (menuSelection - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
        if (down) menuSelection = (menuSelection + 1) % MENU_ITEM_COUNT;
        // Wrap selection to skip Continue if there is no save
        if (menuSelection == 0 && !SaveSystem::hasSaveGame()) {
            if (down) menuSelection = 1;
            else menuSelection = MENU_ITEM_COUNT - 1;
        }
        navCooldown = 0.2f;
    }

    // Action
    if (actionPressed || newGameRequested) {
        actionPressed = false;
        newGameRequested = false;
        switch (menuSelection) {
            case 0: // Continue
                if (SaveSystem::hasSaveGame()) {
                    loadProgress();
                }
                setupDaySettings();
                resetPlayer();
                syncCollectiblesWithSave();
                state = STATE_INTRO;
                fadeAlpha = 1.0f;
                audio.playSound("intro");
                break;
            case 1: // New Game
                currentDay = 1;
                setupDaySettings();
                resetPlayer();
                syncCollectiblesWithSave();
                saveProgress();
                state = STATE_INTRO;
                fadeAlpha = 1.0f;
                audio.playSound("intro");
                break;
            case 2: // Settings
                state = STATE_MENU_SETTINGS;
                settingsSelection = 0;
                break;
            case 3: // Exit
                // We cannot directly exit from Game; main.cpp handles SDL_QUIT
                // Send a synthetic quit event
                SDL_Event quitEvent;
                quitEvent.type = SDL_QUIT;
                SDL_PushEvent(&quitEvent);
                break;
        }
    }
}

void Game::updateSettings(float dt) {
    static float navCooldown = 0.0f;
    static float adjustCooldown = 0.0f;
    navCooldown -= dt;
    adjustCooldown -= dt;

    bool up = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    bool down = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    bool left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    bool right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];

    if (navCooldown <= 0.0f && (up || down)) {
        if (up) settingsSelection = (settingsSelection - 1 + SETTINGS_ITEM_COUNT) % SETTINGS_ITEM_COUNT;
        if (down) settingsSelection = (settingsSelection + 1) % SETTINGS_ITEM_COUNT;
        navCooldown = 0.2f;
    }

    if (adjustCooldown <= 0.0f && (left || right)) {
        int dir = right ? 1 : -1;
        switch (settingsSelection) {
            case 0: // Language
                settings.languageIndex = (settings.languageIndex + dir + 3) % 3;
                applySettings();
                saveSettings();
                break;
            case 1: // Mouse sensitivity
                settings.mouseSensitivity += dir * 0.001f;
                applySettings();
                saveSettings();
                break;
            case 2: // View distance
                settings.viewDistance += dir * 0.1f;
                applySettings();
                saveSettings();
                break;
        }
        adjustCooldown = 0.15f;
    }

    if (actionPressed) {
        actionPressed = false;
        if (settingsSelection == 3) {
            // Back
            state = STATE_MENU;
            menuSelection = 2;
        }
    }
}

void Game::renderMenu() {
    float bg[4] = {0.05f,0.06f,0.08f,1};
    ui.drawRect(0,0,screenWidth,screenHeight,bg);
    float t[4]={0.94f,0.96f,0.9f,1}, s[4]={0.77f,0.8f,0.74f,1};
    ui.drawText(loc.tr("menu.title","AnxietyHorror"), screenWidth*0.1f, screenHeight*0.16f, 4.4f, t);
    ui.drawText(loc.tr("menu.subtitle","Nine days in an open valley"), screenWidth*0.105f, screenHeight*0.3f, 1.8f, s);

    const char* items[MENU_ITEM_COUNT] = {
        "menu.continue",
        "menu.newgame",
        "menu.settings",
        "menu.exit"
    };
    const char* fallbacks[MENU_ITEM_COUNT] = {
        "Continue",
        "New Game",
        "Settings",
        "Exit"
    };

    float startY = screenHeight * 0.5f;
    for (int i = 0; i < MENU_ITEM_COUNT; ++i) {
        if (i == 0 && !SaveSystem::hasSaveGame()) continue;
        bool selected = (i == menuSelection);
        float y = startY + i * 50.0f;
        float col[4] = {0.7f,0.74f,0.68f, selected ? 1.0f : 0.7f};
        if (selected) {
            float selBg[4] = {0.2f,0.25f,0.3f,0.6f};
            ui.drawRect(screenWidth*0.08f - 10, y - 5, 320, 40, selBg);
        }
        ui.drawText((selected ? "> " : "  ") + loc.tr(items[i], fallbacks[i]), screenWidth*0.08f, y, 1.8f, col);
    }

    // Controls hint
    float hint[4] = {0.5f,0.55f,0.5f,0.7f};
    ui.drawText(loc.tr("menu.controls","WASD / Up-Down to navigate | Enter to select"), screenWidth*0.08f, screenHeight*0.9f, 1.2f, hint);
}

void Game::renderSettings() {
    float bg[4] = {0.05f,0.06f,0.08f,1};
    ui.drawRect(0,0,screenWidth,screenHeight,bg);
    float t[4]={0.94f,0.96f,0.9f,1};
    ui.drawText(loc.tr("settings.title","Settings"), screenWidth*0.1f, screenHeight*0.12f, 3.8f, t);

    const char* labels[SETTINGS_ITEM_COUNT] = {
        "settings.language",
        "settings.sensitivity",
        "settings.viewdistance",
        "settings.back"
    };
    const char* fallbacks[SETTINGS_ITEM_COUNT] = {
        "Language",
        "Mouse Sensitivity",
        "View Distance",
        "Back"
    };

    static const char* langNames[3] = {"English", "Русский", "Espanol"};
    std::stringstream ss[SETTINGS_ITEM_COUNT];
    ss[0] << loc.tr(labels[0], fallbacks[0]) << ": " << langNames[settings.languageIndex];
    ss[1] << loc.tr(labels[1], fallbacks[1]) << ": " << std::fixed << std::setprecision(3) << settings.mouseSensitivity;
    ss[2] << loc.tr(labels[2], fallbacks[2]) << ": " << std::fixed << std::setprecision(1) << settings.viewDistance << "x";
    ss[3] << loc.tr(labels[3], fallbacks[3]);

    float startY = screenHeight * 0.35f;
    for (int i = 0; i < SETTINGS_ITEM_COUNT; ++i) {
        bool selected = (i == settingsSelection);
        float y = startY + i * 55.0f;
        float col[4] = {0.7f,0.74f,0.68f, selected ? 1.0f : 0.7f};
        if (selected) {
            float selBg[4] = {0.2f,0.25f,0.3f,0.6f};
            ui.drawRect(screenWidth*0.08f - 10, y - 5, 520, 40, selBg);
            ui.drawText("< " + ss[i].str() + " >", screenWidth*0.08f, y, 1.7f, col);
        } else {
            ui.drawText("  " + ss[i].str(), screenWidth*0.08f, y, 1.7f, col);
        }
    }

    float hint[4] = {0.5f,0.55f,0.5f,0.7f};
    ui.drawText(loc.tr("settings.controls","WASD / Arrows navigate | Left-Right adjust | Enter back"), screenWidth*0.08f, screenHeight*0.9f, 1.2f, hint);
}