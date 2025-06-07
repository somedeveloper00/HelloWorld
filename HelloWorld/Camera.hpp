#pragma once
#include "Transform.hpp"

struct Camera {
    Transform transform;

    /// <summary>
    /// y rotation in degrees
    /// </summary>
    float yaw;

    /// <summary>
    /// x rotation in degrees
    /// </summary>
    float pitch;

    /// <summary>
    /// field of view in degrees
    /// </summary>
    float fov;

    float nearPlane;
    float farPlane;

    Camera(Transform transform = Transform(glm::vec3(0)), float yaw = 0, float pitch = 0, float fov = 1.f, float nearPlane = 0.1, float farPlane = 100.f) :
        transform(transform),
        yaw(yaw),
        pitch(pitch),
        fov(fov),
        nearPlane(nearPlane),
        farPlane(farPlane)
    {
        updateTransform();
    }

    glm::mat4 getProjectionMatrix(int screenWidth, int screenHeight) const
    {
        return glm::perspective(fov, (float)screenWidth / (float)std::max(1, screenHeight), nearPlane, farPlane);
    }

    glm::mat4 getViewMatrix() const
    {
        return transform.getViewMatrix();
    }

    void updateTransform()
    {
        transform.rotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.f));
    }
};
