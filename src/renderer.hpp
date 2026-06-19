#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "math3d.hpp"

#ifdef __ANDROID__
    #include <GLES3/gl32.h>
#else
    #include <glad/glad.h>
#endif

struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
    float color[4];
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    void upload() {
        if (vao == 0) {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);
        }

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Position: Location 0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

        // Normal: Location 1
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        // UV: Location 2
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

        // Color: Location 3
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        glBindVertexArray(0);
    }

    void draw() const {
        if (vao != 0 && !indices.empty()) {
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
    }

    void cleanup() {
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
            vao = vbo = ebo = 0;
        }
        vertices.clear();
        indices.clear();
    }
};

class Shader {
public:
    GLuint id = 0;

private:
    mutable std::unordered_map<std::string, GLint> uniformCache;
    GLint getUniformLocationCached(const std::string& name) const;

public:
    bool compile(const std::string& vertexSource, const std::string& fragmentSource);
    void use() const;
    void cleanup();

    void setMat4(const std::string& name, const Mat4& mat) const;
    void setVec3(const std::string& name, const Vec3& vec) const;
    void setFloat(const std::string& name, float val) const;
    void setInt(const std::string& name, int val) const;
};

// Procedural texture ID helper
struct Texture {
    GLuint id = 0;
    void generateSolid(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void generateNoise(int width, int height, bool woodPattern = false);
    // High-detail multi-octave procedural materials.
    // kind: 0 = mossy ground, 1 = weathered wood, 2 = stone/rock, 3 = bark
    void generateMaterial(int width, int height, int kind);
    void cleanup();
};

// Directional-light shadow map (depth-only FBO) with PCF support in shaders.
struct ShadowMap {
    GLuint fbo = 0;
    GLuint depthTex = 0;
    int size = 2048;

    bool init(int resolution);
    void beginRender();   // bind FBO + set viewport + clear depth
    void endRender(int screenW, int screenH); // restore default framebuffer/viewport
    void bindForReading(int textureUnit) const;
    void cleanup();
};
