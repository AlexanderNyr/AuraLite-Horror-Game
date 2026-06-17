#pragma once
#include <string>
#include "renderer.hpp"

class UIRenderer {
private:
    Shader uiShader;
    GLuint fontTextureId = 0;
    GLuint rectVao = 0, rectVbo = 0;
    int width = 1280;
    int height = 720;

public:
    void init(int screenWidth, int screenHeight);
    void resize(int screenWidth, int screenHeight);
    void drawRect(float x, float y, float w, float h, const float color[4], GLuint textureId = 0);
    void drawText(const std::string& text, float x, float y, float scale, const float color[4]);
    void drawVirtualJoysticks(float leftX, float leftY, float leftRadius, float activeX, float activeY,
                              float rightX, float rightY, float rightRadius, float activeCamX, float activeCamY,
                              bool isAndroid);
    void cleanup();

private:
    void generateFontTexture();
};
