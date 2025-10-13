#pragma once

#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/transform.hpp"
#include "engine/window.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"

// moves transform in a FPS
struct fpsMoveAround : public engine::component
{
    createTypeInformation(fpsMoveAround, component);

    float speed = 7.f;
    float lookSpeed = .1f;
    float fovChangeRate = .1f;

  private:
    float yaw = 0;
    float pitch = 0;
    bool rotating = false;
    glm::ivec2 _mousePosWhenRotationStarted;
    bool _isRotating = false;

    void update_() override
    {
        auto transform = getEntity()->getComponent<engine::transform>();
        if (!transform)
            return;
        transform->markDirty();
        float speed = engine::input::isKeyHeldDown(engine::input::key::leftShift)
                          ? this->speed * 5.f
                      : engine::input::isKeyHeldDown(engine::input::key::leftControl)
                          ? this->speed / 5.f
                          : this->speed;
        if (engine::input::isKeyHeldDown(engine::input::key::w))
            transform->position += transform->getForward() * speed * engine::time::getDeltaTime();
        if (engine::input::isKeyHeldDown(engine::input::key::s))
            transform->position -= transform->getForward() * speed * engine::time::getDeltaTime();
        if (engine::input::isKeyHeldDown(engine::input::key::d))
            transform->position += transform->getRight() * speed * engine::time::getDeltaTime();
        if (engine::input::isKeyHeldDown(engine::input::key::a))
            transform->position -= transform->getRight() * speed * engine::time::getDeltaTime();
        if (engine::input::isKeyHeldDown(engine::input::key::e))
            transform->position += transform->getUp() * speed * engine::time::getDeltaTime();
        if (engine::input::isKeyHeldDown(engine::input::key::q))
            transform->position -= transform->getUp() * speed * engine::time::getDeltaTime();

        if (engine::input::isMouseInWindow() && engine::input::isKeyHeldDown(engine::input::key::mouseRight))
        {
            if (!_isRotating)
            {
                _isRotating = true;
                _mousePosWhenRotationStarted = engine::input::getMousePosition();
            }

            // rotate
            const auto mouseDelta = engine::input::getMouseDelta();
            pitch = getLockedPitch(pitch - lookSpeed * mouseDelta.y);
            yaw += lookSpeed * mouseDelta.x;
            engine::input::setMouseVisibility(false);
            engine::input::setMousePosition(_mousePosWhenRotationStarted);
        }
        else
        {
            engine::input::setMouseVisibility(true);
            _isRotating = false;
        }
        transform->rotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0));

        // field of view change
        if (engine::camera *camera = getEntity()->getComponent<engine::camera>())
        {
            const auto fov = camera->getFieldOfView();
            const auto newFov = fov + engine::input::getMouseWheelDelta().y * fovChangeRate;
            if (newFov != fov)
                camera->setFieldOfView(newFov);
        }
    }

    static float getLockedPitch(const float pitch) noexcept
    {
        if (pitch > 89.9f)
            return 89.9f;
        if (pitch < -89.9f)
            return -89.9f;
        return pitch;
    }
};