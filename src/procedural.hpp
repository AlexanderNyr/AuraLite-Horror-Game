#pragma once
#include "renderer.hpp"

float getTerrainHeight(float x, float z);
void generateTerrain(Mesh& mesh, float width, float depth, int subdivsX, int subdivsZ);
void generateBox(Mesh& mesh, const Vec3& size, const Vec3& offset, const float color[4], const Vec2& uvScale = {1.0f, 1.0f});
void generateCylinder(Mesh& mesh, float baseRadius, float topRadius, float height, int slices, const Vec3& offset, const float color[4]);
void generateCone(Mesh& mesh, float baseRadius, float height, int slices, const Vec3& offset, const float color[4]);

void generateCabin(Mesh& mesh);
void generateTree(Mesh& mesh);
void generateWell(Mesh& mesh);
void generateCar(Mesh& mesh);

// === New village & open world meshes ===
void generateHouse(Mesh& mesh, bool hasChimney = true);
void generateRock(Mesh& mesh, float scale = 1.0f);
void generateFence(Mesh& mesh, float length = 4.0f);
void generatePath(Mesh& mesh, float width, float length);