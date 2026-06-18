#include "renderer.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "AnxietyHorror", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "AnxietyHorror", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#define LOGE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#endif

bool Shader::compile(const std::string& vertexSource, const std::string& fragmentSource) {
    std::string versionHeader;
#ifdef __ANDROID__
    versionHeader = "#version 320 es\nprecision mediump float;\n";
#else
    versionHeader = "#version 450 core\n";
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

// ---- High-detail procedural materials (multi-octave fbm) ----
static float th_hash(int x, int y) {
    int n = x * 374761393 + y * 668265263;
    n = (n ^ (n >> 13)) * 1274126177;
    return (float)((n ^ (n >> 16)) & 0x7fffffff) / (float)0x7fffffff;
}
static float th_smoothNoise(float x, float y) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float a = th_hash(ix, iy),     b = th_hash(ix + 1, iy);
    float c = th_hash(ix, iy + 1), d = th_hash(ix + 1, iy + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
static float th_fbm(float x, float y, int octaves) {
    float sum = 0.0f, amp = 0.5f, freq = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * th_smoothNoise(x * freq, y * freq);
        freq *= 2.0f; amp *= 0.5f;
    }
    return sum;
}
static inline uint8_t th_clamp8(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    return (uint8_t)v;
}

void Texture::generateMaterial(int width, int height, int kind) {
    if (id == 0) glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    std::vector<uint8_t> pixels(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            float u = (float)x / (float)width;
            float v = (float)y / (float)height;
            float r, g, b;

            if (kind == 0) {
                // Mossy damp ground: layered greens/browns + scattered pebbles + dirt patches.
                float base   = th_fbm(u * 9.0f,  v * 9.0f,  5);
                float detail = th_fbm(u * 34.0f, v * 34.0f, 4);
                float patch  = th_fbm(u * 4.0f,  v * 4.0f,  3);
                float moss   = 0.5f + 0.5f * sinf((base + detail * 0.3f) * 6.28318f);
                float dirt   = patch * patch;
                float pebble = (th_smoothNoise(u * 90.0f, v * 90.0f) > 0.82f) ? 1.0f : 0.0f;
                float shade  = 0.55f + 0.45f * detail;

                r = (28.0f + 34.0f * dirt + 18.0f * (1.0f - moss)) * shade + pebble * 95.0f;
                g = (40.0f + 52.0f * moss - 14.0f * dirt) * shade + pebble * 90.0f;
                b = (18.0f + 16.0f * moss - 6.0f  * dirt) * shade + pebble * 78.0f;
            } else if (kind == 1) {
                // Weathered planks: long grain, plank seams, damp dark stains.
                float plank = floorf(v * 7.0f);
                float seam  = ((v * 7.0f - plank) < 0.04f) ? 0.35f : 1.0f;
                float grain = th_fbm(u * 6.0f + plank * 3.1f, v * 90.0f, 5);
                float fine  = th_fbm(u * 3.0f, v * 240.0f, 3) * 0.4f;
                float stain = th_fbm(u * 5.0f, v * 5.0f, 4);
                float t = (0.45f + grain * 0.45f + fine) * seam;
                t -= stain * stain * 0.35f;
                if (t < 0.05f) t = 0.05f;

                r = (78.0f * t + 26.0f);
                g = (52.0f * t + 16.0f);
                b = (30.0f * t + 9.0f);
            } else if (kind == 2) {
                // Stone/rock: granular speckle, cracks and lichen tint.
                float rock  = th_fbm(u * 14.0f, v * 14.0f, 6);
                float speck = th_fbm(u * 70.0f, v * 70.0f, 3);
                float crack = th_fbm(u * 8.0f,  v * 8.0f,  4);
                crack = 1.0f - fabsf(crack - 0.5f) * 2.0f;
                crack = powf(crack, 8.0f);
                float lichen = (th_smoothNoise(u * 20.0f + 5.0f, v * 20.0f) > 0.7f) ? 0.5f : 0.0f;
                float t = 0.4f + rock * 0.4f + speck * 0.2f;
                t *= (1.0f - crack * 0.6f);

                r = 96.0f * t + 22.0f;
                g = (96.0f * t + 24.0f) * (1.0f - lichen * 0.2f) + lichen * 30.0f;
                b = (92.0f * t + 24.0f) * (1.0f - lichen * 0.35f) + lichen * 18.0f;
            } else {
                // Bark: vertical fibrous ridges + deep cracks.
                float ridge = sinf((u * 26.0f + th_fbm(u * 4.0f, v * 9.0f, 4) * 3.0f) * 3.14159f);
                ridge = ridge * 0.5f + 0.5f;
                float deep  = th_fbm(u * 3.0f, v * 22.0f, 5);
                float t = 0.35f + ridge * 0.4f + deep * 0.25f;
                r = 60.0f * t + 18.0f;
                g = 42.0f * t + 12.0f;
                b = 26.0f * t + 8.0f;
            }

            pixels[idx]     = th_clamp8(r);
            pixels[idx + 1] = th_clamp8(g);
            pixels[idx + 2] = th_clamp8(b);
            pixels[idx + 3] = 255;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Anisotropic filtering if available (desktop) for crisp grazing-angle detail.
#ifndef __ANDROID__
    GLfloat maxAniso = 0.0f;
    glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY*/, &maxAniso);
    if (maxAniso > 1.0f) {
        GLfloat want = maxAniso < 8.0f ? maxAniso : 8.0f;
        glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY*/, want);
    }
#endif

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::cleanup() {
    if (id != 0) {
        glDeleteTextures(1, &id);
        id = 0;
    }
}

// ==================== Shadow map ====================
bool ShadowMap::init(int resolution) {
    size = resolution;
    glGenFramebuffers(1, &fbo);

    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
#ifndef __ANDROID__
    GLfloat border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
#endif
    // Enable hardware depth comparison for free PCF via sampler2DShadow.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
#ifndef __ANDROID__
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
#endif
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!ok) {
        LOGE("ShadowMap framebuffer incomplete");
    }
    return ok;
}

void ShadowMap::beginRender() {
    glViewport(0, 0, size, size);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::endRender(int screenW, int screenH) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
}

void ShadowMap::bindForReading(int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthTex);
}

void ShadowMap::cleanup() {
    if (depthTex != 0) { glDeleteTextures(1, &depthTex); depthTex = 0; }
    if (fbo != 0) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
}
