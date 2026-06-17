#pragma once
#include "math3d.hpp"

class Camera {
public:
    Vec3 position;
    Vec3 front;
    Vec3 up;
    Vec3 right;
    Vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;

    Camera(Vec3 startPos = Vec3(0.0f, 2.0f, 0.0f));

    Mat4 getViewMatrix() const;
    void updateCameraVectors();

    void processKeyboard(int direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
};
