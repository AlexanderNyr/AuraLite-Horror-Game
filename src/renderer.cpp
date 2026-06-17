#include "renderer.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "HorrorGame", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "HorrorGame", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#define LOGE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#endif

bool Shader::compile(const std::string& vertexSource, const std::string& fragmentSource) {
    std::string versionHeader;
#ifdef __ANDROID__
    versionHeader = "#version 300 es\nprecision mediump float;\n";
#else
    versionHeader = "#version 330 core\n";
#endif

    std::string fullVert = versionHeader + vertexSource;
    std::string fullFrag = versionHeader + fragmentSource;

    const char* vSource = fullVert.c_str();
    const char* fSource = fullFrag.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        LOGE("Vertex shader compile error: %s", infoLog);
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        LOGE("Fragment shader compile error: %s", infoLog);
        return false;
    }

    id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    glLinkProgram(id);

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(id, 512, nullptr, infoLog);
        LOGE("Shader program linking error: %s", infoLog);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

void Shader::use() const {
    if (id != 0) {
        glUseProgram(id);
    }
}

void Shader::cleanup() {
    if (id != 0) {
        glDeleteProgram(id);
        id = 0;
    }
}

void Shader::setMat4(const std::string& name, const Mat4& mat) const {
    GLint loc = glGetUniformLocation(id, name.c_str());
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, mat.m);
    }
}

void Shader::setVec3(const std::string& name, const Vec3& vec) const {
    GLint loc = glGetUniformLocation(id, name.c_str());
    if (loc != -1) {
        glUniform3f(loc, vec.x, vec.y, vec.z);
    }
}

void Shader::setFloat(const std::string& name, float val) const {
    GLint loc = glGetUniformLocation(id, name.c_str());
    if (loc != -1) {
        glUniform1f(loc, val);
    }
}

void Shader::setInt(const std::string& name, int val) const {
    GLint loc = glGetUniformLocation(id, name.c_str());
    if (loc != -1) {
        glUniform1i(loc, val);
    }
}

void Texture::generateSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (id == 0) {
        glGenTextures(1, &id);
    }
    glBindTexture(GL_TEXTURE_2D, id);

    uint8_t pixels[4] = {r, g, b, a};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::generateNoise(int width, int height, bool woodPattern) {
    if (id == 0) {
        glGenTextures(1, &id);
    }
    glBindTexture(GL_TEXTURE_2D, id);

    std::vector<uint8_t> pixels(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            if (woodPattern) {
                // Wood grain pattern: fine stripes along x with some sine waves
                float grain = sinf(y * 0.8f + sinf(x * 0.1f) * 0.5f) * 0.1f + 0.9f;
                // Horizonal joints
                if (y % 16 < 2) grain *= 0.4f;
                // Add tiny random grain noise
                grain += (rand() % 100) / 800.0f - 0.0625f;
                grain = fmaxf(0.0f, fminf(1.0f, grain));

                pixels[idx]     = (uint8_t)(55.0f * grain); // R
                pixels[idx + 1] = (uint8_t)(35.0f * grain); // G
                pixels[idx + 2] = (uint8_t)(18.0f * grain); // B
                pixels[idx + 3] = 255;                      // A
            } else {
                // Grass / Mud dark green organic moss pattern
                float noise = (sinf(x * 0.15f) * cosf(y * 0.15f) + sinf(x * 0.05f) * sinf(y * 0.09f)) * 0.25f + 0.5f;
                noise += (rand() % 100) / 1000.0f;
                noise = fmaxf(0.0f, fminf(1.0f, noise));

                pixels[idx]     = (uint8_t)(25.0f * noise); // R
                pixels[idx + 1] = (uint8_t)(38.0f * noise); // G
                pixels[idx + 2] = (uint8_t)(15.0f * noise); // B
                pixels[idx + 3] = 255;                      // A
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::cleanup() {
    if (id != 0) {
        glDeleteTextures(1, &id);
        id = 0;
    }
}
