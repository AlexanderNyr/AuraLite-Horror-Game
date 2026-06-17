#include "procedural.hpp"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Helper to add face to a box
void addBoxFace(Mesh& mesh, const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4, const Vec3& normal, const float color[4], const Vec2& uvScale) {
    unsigned int startIdx = (unsigned int)mesh.vertices.size();

    Vertex v1, v2, v3, v4;
    v1.pos = p1; v1.normal = normal; v1.uv = Vec2(0.0f, 0.0f);
    v2.pos = p2; v2.normal = normal; v2.uv = Vec2(uvScale.x, 0.0f);
    v3.pos = p3; v3.normal = normal; v3.uv = Vec2(uvScale.x, uvScale.y);
    v4.pos = p4; v4.normal = normal; v4.uv = Vec2(0.0f, uvScale.y);

    for (int i = 0; i < 4; ++i) {
        v1.color[i] = color[i];
        v2.color[i] = color[i];
        v3.color[i] = color[i];
        v4.color[i] = color[i];
    }

    mesh.vertices.push_back(v1);
    mesh.vertices.push_back(v2);
    mesh.vertices.push_back(v3);
    mesh.vertices.push_back(v4);

    mesh.indices.push_back(startIdx + 0);
    mesh.indices.push_back(startIdx + 1);
    mesh.indices.push_back(startIdx + 2);

    mesh.indices.push_back(startIdx + 0);
    mesh.indices.push_back(startIdx + 2);
    mesh.indices.push_back(startIdx + 3);
}

void generateBox(Mesh& mesh, const Vec3& size, const Vec3& offset, const float color[4], const Vec2& uvScale) {
    float dx = size.x * 0.5f;
    float dy = size.y * 0.5f;
    float dz = size.z * 0.5f;

    // Corner vertices
    Vec3 c0 = offset + Vec3(-dx, -dy, -dz);
    Vec3 c1 = offset + Vec3( dx, -dy, -dz);
    Vec3 c2 = offset + Vec3( dx,  dy, -dz);
    Vec3 c3 = offset + Vec3(-dx,  dy, -dz);
    Vec3 c4 = offset + Vec3(-dx, -dy,  dz);
    Vec3 c5 = offset + Vec3( dx, -dy,  dz);
    Vec3 c6 = offset + Vec3( dx,  dy,  dz);
    Vec3 c7 = offset + Vec3(-dx,  dy,  dz);

    // Front (z+)
    addBoxFace(mesh, c4, c5, c6, c7, Vec3(0, 0, 1), color, uvScale);
    // Back (z-)
    addBoxFace(mesh, c1, c0, c3, c2, Vec3(0, 0, -1), color, uvScale);
    // Left (x-)
    addBoxFace(mesh, c0, c4, c7, c3, Vec3(-1, 0, 0), color, uvScale);
    // Right (x+)
    addBoxFace(mesh, c5, c1, c2, c6, Vec3(1, 0, 0), color, uvScale);
    // Top (y+)
    addBoxFace(mesh, c3, c2, c6, c7, Vec3(0, 1, 0), color, uvScale);
    // Bottom (y-)
    addBoxFace(mesh, c0, c1, c5, c4, Vec3(0, -1, 0), color, uvScale);
}

void generateCylinder(Mesh& mesh, float baseRadius, float topRadius, float height, int slices, const Vec3& offset, const float color[4]) {
    unsigned int baseVertIdx = (unsigned int)mesh.vertices.size();

    // Generate vertices for the lateral surface
    for (int j = 0; j <= 1; ++j) {
        float y = offset.y + (j == 0 ? 0.0f : height);
        float r = (j == 0 ? baseRadius : topRadius);

        for (int i = 0; i <= slices; ++i) {
            float angle = (float)i / slices * 2.0f * M_PI;
            float c = std::cos(angle);
            float s = std::sin(angle);

            Vertex v;
            v.pos = Vec3(offset.x + c * r, y, offset.z + s * r);
            // Cylinder lateral normals point outward
            v.normal = Vec3(c, 0.0f, s).normalized();
            v.uv = Vec2((float)i / slices, (float)j);
            std::memcpy(v.color, color, sizeof(float) * 4);

            mesh.vertices.push_back(v);
        }
    }

    // Connect side vertices with triangles
    for (int i = 0; i < slices; ++i) {
        unsigned int b0 = baseVertIdx + i;
        unsigned int b1 = baseVertIdx + i + 1;
        unsigned int t0 = baseVertIdx + slices + 1 + i;
        unsigned int t1 = baseVertIdx + slices + 1 + i + 1;

        mesh.indices.push_back(b0);
        mesh.indices.push_back(b1);
        mesh.indices.push_back(t1);

        mesh.indices.push_back(b0);
        mesh.indices.push_back(t1);
        mesh.indices.push_back(t0);
    }
}

void generateCone(Mesh& mesh, float baseRadius, float height, int slices, const Vec3& offset, const float color[4]) {
    unsigned int baseVertIdx = (unsigned int)mesh.vertices.size();

    // Apex vertex (top)
    Vertex apex;
    apex.pos = offset + Vec3(0.0f, height, 0.0f);
    apex.normal = Vec3(0.0f, 1.0f, 0.0f);
    apex.uv = Vec2(0.5f, 1.0f);
    std::memcpy(apex.color, color, sizeof(float) * 4);
    mesh.vertices.push_back(apex);

    unsigned int apexIdx = baseVertIdx;

    // Base circle vertices
    for (int i = 0; i <= slices; ++i) {
        float angle = (float)i / slices * 2.0f * M_PI;
        float c = std::cos(angle);
        float s = std::sin(angle);

        Vertex v;
        v.pos = offset + Vec3(c * baseRadius, 0.0f, s * baseRadius);
        // Slanted normal
        v.normal = Vec3(c, 0.5f, s).normalized();
        v.uv = Vec2((float)i / slices, 0.0f);
        std::memcpy(v.color, color, sizeof(float) * 4);

        mesh.vertices.push_back(v);
    }

    // Connect triangles
    for (int i = 0; i < slices; ++i) {
        unsigned int b0 = baseVertIdx + 1 + i;
        unsigned int b1 = baseVertIdx + 1 + i + 1;

        mesh.indices.push_back(b0);
        mesh.indices.push_back(b1);
        mesh.indices.push_back(apexIdx);
    }
}

float getTerrainHeight(float x, float z) {
    // Spooky valley math: center is flat, sides rise steeply
    float baseNoise = std::sin(x * 0.06f) * std::cos(z * 0.06f) * 3.5f + std::sin(x * 0.15f) * 0.5f;
    
    // Smooth valley flat path in the middle (from x = -20 to x = 20)
    float edgeDist = std::abs(x);
    float valleyWalls = 0.0f;
    if (edgeDist > 30.0f) {
        float factor = (edgeDist - 30.0f);
        valleyWalls = factor * factor * 0.015f + factor * 0.35f;
    }
    
    return baseNoise + valleyWalls;
}

void generateTerrain(Mesh& mesh, float width, float depth, int subdivsX, int subdivsZ) {
    float startX = -width / 2.0f;
    float startZ = -depth / 2.0f;
    float stepX = width / subdivsX;
    float stepZ = depth / subdivsZ;

    // Generate vertices
    for (int z = 0; z <= subdivsZ; ++z) {
        float posZ = startZ + z * stepZ;
        for (int x = 0; x <= subdivsX; ++x) {
            float posX = startX + x * stepX;
            float posY = getTerrainHeight(posX, posZ);

            Vertex v;
            v.pos = Vec3(posX, posY, posZ);

            // Calculate finite differences for smooth normals
            float hL = getTerrainHeight(posX - 1.0f, posZ);
            float hR = getTerrainHeight(posX + 1.0f, posZ);
            float hD = getTerrainHeight(posX, posZ - 1.0f);
            float hU = getTerrainHeight(posX, posZ + 1.0f);
            v.normal = Vec3(hL - hR, 2.0f, hD - hU).normalized();

            // UV mapping
            v.uv = Vec2(posX * 0.25f, posZ * 0.25f);

            // Ground colors: dark gloomy moss-green blended with soil-brown
            float slope = 1.0f - v.normal.y; // 0 = flat, 1 = vertical
            float r = 0.05f + slope * 0.12f;
            float g = 0.12f - slope * 0.05f;
            float b = 0.03f + slope * 0.02f;

            v.color[0] = r;
            v.color[1] = g;
            v.color[2] = b;
            v.color[3] = 1.0f;

            mesh.vertices.push_back(v);
        }
    }

    // Generate indices
    for (int z = 0; z < subdivsZ; ++z) {
        for (int x = 0; x < subdivsX; ++x) {
            unsigned int row1 = z * (subdivsX + 1);
            unsigned int row2 = (z + 1) * (subdivsX + 1);

            mesh.indices.push_back(row1 + x);
            mesh.indices.push_back(row2 + x);
            mesh.indices.push_back(row1 + x + 1);

            mesh.indices.push_back(row1 + x + 1);
            mesh.indices.push_back(row2 + x);
            mesh.indices.push_back(row2 + x + 1);
        }
    }
}

void generateCabin(Mesh& mesh) {
    // 1. Cozy wooden cabin base structure
    // Log wood color
    float woodColor[4] = {0.35f, 0.22f, 0.12f, 1.0f};
    float darkWoodColor[4] = {0.20f, 0.12f, 0.06f, 1.0f};
    float redRoofColor[4] = {0.30f, 0.10f, 0.10f, 1.0f};
    float bedColor[4] = {0.80f, 0.20f, 0.20f, 1.0f};
    float whiteSheetColor[4] = {0.90f, 0.90f, 0.90f, 1.0f};

    // Floor (dirt texture coordinates)
    generateBox(mesh, Vec3(12.0f, 0.2f, 10.0f), Vec3(0.0f, 0.1f, 0.0f), woodColor, {3.0f, 3.0f});

    // Back Wall (z = -5)
    generateBox(mesh, Vec3(12.0f, 6.0f, 0.4f), Vec3(0.0f, 3.0f, -5.0f), darkWoodColor, {3.0f, 2.0f});

    // Left Wall (x = -6)
    generateBox(mesh, Vec3(0.4f, 6.0f, 10.0f), Vec3(-6.0f, 3.0f, 0.0f), darkWoodColor, {3.0f, 2.0f});

    // Right Wall (x = 6)
    generateBox(mesh, Vec3(0.4f, 6.0f, 10.0f), Vec3(6.0f, 3.0f, 0.0f), darkWoodColor, {3.0f, 2.0f});

    // Front Wall with Doorway
    // Left side of front wall
    generateBox(mesh, Vec3(4.5f, 6.0f, 0.4f), Vec3(-3.75f, 3.0f, 5.0f), darkWoodColor, {1.5f, 2.0f});
    // Right side of front wall
    generateBox(mesh, Vec3(4.5f, 6.0f, 0.4f), Vec3(3.75f, 3.0f, 5.0f), darkWoodColor, {1.5f, 2.0f});
    // Over the door
    generateBox(mesh, Vec3(3.0f, 1.8f, 0.4f), Vec3(0.0f, 5.1f, 5.0f), darkWoodColor, {1.0f, 1.0f});

    // Slanted Roof
    // Left slant
    generateBox(mesh, Vec3(8.0f, 0.3f, 11.2f), Vec3(-3.5f, 7.3f, 0.0f), redRoofColor, {4.0f, 2.0f});
    // We would rotate it, but for our simple procedural needs, a flat high-pitched cap or slightly overlapping boards is perfect!
    // Right slant
    generateBox(mesh, Vec3(8.0f, 0.3f, 11.2f), Vec3(3.5f, 7.3f, 0.0f), redRoofColor, {4.0f, 2.0f});
    // Ridge cap
    generateBox(mesh, Vec3(1.0f, 0.5f, 11.6f), Vec3(0.0f, 8.2f, 0.0f), darkWoodColor, {1.0f, 1.0f});

    // 2. FURNITURE INSIDE
    // Bed frame
    generateBox(mesh, Vec3(4.5f, 1.0f, 3.0f), Vec3(-3.5f, 0.7f, -3.0f), darkWoodColor);
    // Bed mattress
    generateBox(mesh, Vec3(4.2f, 0.6f, 2.8f), Vec3(-3.5f, 1.3f, -3.0f), bedColor);
    // Pillow
    generateBox(mesh, Vec3(1.0f, 0.3f, 2.0f), Vec3(-5.0f, 1.6f, -3.0f), whiteSheetColor);

    // Diary Table
    generateBox(mesh, Vec3(2.5f, 1.6f, 2.5f), Vec3(4.0f, 1.0f, -3.0f), darkWoodColor);
    // Glowing Journal
    float bookColor[4] = {0.8f, 0.8f, 0.9f, 1.0f};
    generateBox(mesh, Vec3(0.6f, 0.1f, 0.8f), Vec3(4.0f, 1.85f, -3.0f), bookColor);
}

void generateTree(Mesh& mesh) {
    float trunkColor[4] = {0.20f, 0.14f, 0.08f, 1.0f};
    float leavesColor[4] = {0.05f, 0.12f, 0.06f, 1.0f};

    // Pine Tree trunk: tall skinny cylinder
    generateCylinder(mesh, 0.4f, 0.3f, 5.0f, 6, Vec3(0, 0, 0), trunkColor);

    // Pine Tree Foliage: Stacked cones
    generateCone(mesh, 2.5f, 3.0f, 6, Vec3(0, 3.5f, 0), leavesColor);
    generateCone(mesh, 2.0f, 2.5f, 6, Vec3(0, 5.5f, 0), leavesColor);
    generateCone(mesh, 1.4f, 2.0f, 6, Vec3(0, 7.3f, 0), leavesColor);
}

void generateWell(Mesh& mesh) {
    float stoneColor[4] = {0.35f, 0.35f, 0.35f, 1.0f};
    float woodColor[4] = {0.25f, 0.16f, 0.08f, 1.0f};

    // Circular stone well walls using boxes
    int stoneSlices = 10;
    float radius = 2.5f;
    for (int i = 0; i < stoneSlices; ++i) {
        float angle = (float)i / stoneSlices * 2.0f * M_PI;
        float c = std::cos(angle);
        float s = std::sin(angle);
        
        // Position on a circle
        Vec3 pos(c * radius, 1.0f, s * radius);
        // Box oriented towards circle center
        generateBox(mesh, Vec3(1.2f, 2.0f, 0.4f), pos, stoneColor);
    }

    // Wooden roof posts
    generateBox(mesh, Vec3(0.2f, 4.0f, 0.2f), Vec3(-2.2f, 3.0f, 0.0f), woodColor);
    generateBox(mesh, Vec3(0.2f, 4.0f, 0.2f), Vec3(2.2f, 3.0f, 0.0f), woodColor);

    // Cross bar
    generateBox(mesh, Vec3(4.6f, 0.2f, 0.2f), Vec3(0.0f, 4.8f, 0.0f), woodColor);

    // Spooky small wooden cover
    generateBox(mesh, Vec3(5.2f, 0.2f, 3.5f), Vec3(0.0f, 5.0f, 0.0f), woodColor);
}

void generateCar(Mesh& mesh) {
    float metalColor[4] = {0.22f, 0.22f, 0.25f, 1.0f}; // spooky rusted navy grey
    float darkMetalColor[4] = {0.12f, 0.12f, 0.14f, 1.0f};
    float wheelColor[4] = {0.05f, 0.05f, 0.05f, 1.0f};
    float yellowLightColor[4] = {1.0f, 1.0f, 0.8f, 1.0f};

    // Car Body Base
    generateBox(mesh, Vec3(5.0f, 1.2f, 10.0f), Vec3(0.0f, 1.2f, 0.0f), metalColor);

    // Car Cabin
    generateBox(mesh, Vec3(4.5f, 1.4f, 5.0f), Vec3(0.0f, 2.5f, -1.5f), darkMetalColor);

    // Front glowing Headlights
    generateBox(mesh, Vec3(0.8f, 0.4f, 0.1f), Vec3(-1.8f, 1.3f, 5.05f), yellowLightColor);
    generateBox(mesh, Vec3(0.8f, 0.4f, 0.1f), Vec3(1.8f, 1.3f, 5.05f), yellowLightColor);

    // Wheels (4 cylinders/boxes)
    generateBox(mesh, Vec3(0.8f, 1.5f, 1.5f), Vec3(-2.4f, 0.75f, 3.0f), wheelColor);
    generateBox(mesh, Vec3(0.8f, 1.5f, 1.5f), Vec3(2.4f, 0.75f, 3.0f), wheelColor);
    generateBox(mesh, Vec3(0.8f, 1.5f, 1.5f), Vec3(-2.4f, 0.75f, -3.0f), wheelColor);
    generateBox(mesh, Vec3(0.8f, 1.5f, 1.5f), Vec3(2.4f, 0.75f, -3.0f), wheelColor);
}
