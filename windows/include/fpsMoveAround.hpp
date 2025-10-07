#pragma once

#include "engine/app.hpp"
#include "engine/components/transform.hpp"
#include "engine/window.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

// moves transform in a FPS
struct fpsMoveAround : public engine::component
{
    createTypeInformation(fpsMoveAround, component);

    float speed = 7.f;
    float lookSpeed = 100.f;

  private:
    float yaw = 180;
    float pitch = 0;

    void update_() override
    {
        engine::graphics::setCursorVisibility(false);
        auto transform = getEntity()->getComponent<engine::transform>();
        if (!transform)
            return;
        transform->markDirty();
        float speed = engine::input::isKeyHeldDown(engine::input::key::leftShift) ? this->speed * 5.f : this->speed;
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
        // rotate
        const auto mouseDelta = static_cast<glm::vec2>(engine::input::getMousePositionCentered()) /
                                static_cast<glm::vec2>(engine::graphics::getFrameBufferSize());
        engine::graphics::setMousePositionCentered({0, 0});
        if (glm::length(mouseDelta) > 0.4f)
            return;
        transform->rotation = glm::quat(glm::vec3(
            glm::radians(pitch = getLockedPitch(pitch + lookSpeed * mouseDelta.y)),
            glm::radians(yaw -= lookSpeed * mouseDelta.x),
            0));
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