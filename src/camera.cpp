#include "camera.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Camera::Camera(Vec3 startPos)
    : position(startPos),
      front(Vec3(0.0f, 0.0f, -1.0f)),
      up(Vec3(0.0f, 1.0f, 0.0f)),
      worldUp(Vec3(0.0f, 1.0f, 0.0f)),
      yaw(-M_PI / 2.0f), // face towards -z by default
      pitch(0.0f),
      movementSpeed(15.0f),
      mouseSensitivity(0.005f) {
    updateCameraVectors();
}

Mat4 Camera::getViewMatrix() const {
    return Mat4::lookAt(position, position + front, up);
}

void Camera::updateCameraVectors() {
    Vec3 f;
    f.x = std::cos(yaw) * std::cos(pitch);
    f.y = std::sin(pitch);
    f.z = std::sin(yaw) * std::cos(pitch);
    front = f.normalized();

    right = Vec3::cross(front, worldUp).normalized();
    up = Vec3::cross(right, front).normalized();
}

void Camera::processKeyboard(int direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    // Walk only in the horizontal plane (FPS style)
    Vec3 horizontalFront = Vec3(front.x, 0.0f, front.z).normalized();
    Vec3 horizontalRight = Vec3(right.x, 0.0f, right.z).normalized();

    if (direction == 1) // FORWARD
        position += horizontalFront * velocity;
    if (direction == 2) // BACKWARD
        position -= horizontalFront * velocity;
    if (direction == 3) // LEFT
        position -= horizontalRight * velocity;
    if (direction == 4) // RIGHT
        position += horizontalRight * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (constrainPitch) {
        float limit = 89.0f * M_PI / 180.0f;
        if (pitch > limit) pitch = limit;
        if (pitch < -limit) pitch = -limit;
    }

    updateCameraVectors();
}
