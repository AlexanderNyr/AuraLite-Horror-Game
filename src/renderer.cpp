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
        glDeleteShader(vertexShader);
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
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
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
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(id);
        id = 0;
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
                // Layered procedural wood: long grain, dark plank seams and damp stains.
                float fx = (float)x / (float)width;
                float fy = (float)y / (float)height;
                float longGrain = sinf((fy * 36.0f + sinf(fx * 18.0f) * 1.7f) * 3.14159f) * 0.5f + 0.5f;
                float fineGrain = sinf((fy * 180.0f + fx * 11.0f) * 3.14159f) * 0.5f + 0.5f;
                float knots = sinf((fx * 9.0f + sinf(fy * 16.0f)) * 3.14159f) * sinf((fy * 7.0f) * 3.14159f);
                float seam = (y % 18 < 2) ? 0.45f : 1.0f;
                float grain = (0.55f + longGrain * 0.28f + fineGrain * 0.10f + knots * 0.08f) * seam;
                grain = fmaxf(0.0f, fminf(1.0f, grain));

                pixels[idx]     = (uint8_t)(72.0f * grain + 12.0f); // R
                pixels[idx + 1] = (uint8_t)(48.0f * grain + 8.0f);  // G
                pixels[idx + 2] = (uint8_t)(25.0f * grain + 5.0f);  // B
                pixels[idx + 3] = 255;                              // A
            } else {
                // Moss / mud ground: several frequencies plus deterministic pebbles.
                float fx = (float)x / (float)width;
                float fy = (float)y / (float)height;
                float broad = (sinf(fx * 18.0f) * cosf(fy * 15.0f)) * 0.5f + 0.5f;
                float moss = (sinf((fx + fy) * 57.0f) * sinf(fy * 43.0f)) * 0.5f + 0.5f;
                float pebble = ((x * 37 + y * 17) % 101 == 0) ? 0.35f : 0.0f;
                float wet = fmaxf(0.0f, fminf(1.0f, 0.42f + broad * 0.22f + moss * 0.18f - pebble));

                pixels[idx]     = (uint8_t)(18.0f + 28.0f * wet + pebble * 80.0f); // R
                pixels[idx + 1] = (uint8_t)(28.0f + 45.0f * wet + pebble * 70.0f); // G
                pixels[idx + 2] = (uint8_t)(14.0f + 18.0f * wet + pebble * 55.0f); // B
                pixels[idx + 3] = 255;                                             // A
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
