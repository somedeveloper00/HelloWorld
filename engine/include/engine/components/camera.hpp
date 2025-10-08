#pragma once

#include "engine/app.hpp"
#include "engine/window.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "transform.hpp"

namespace engine
{
// a simple camera component
struct camera : public component
{
    createTypeInformation(camera, component);

    // in radians
    float getFieldOfView() const noexcept
    {
        return _fieldOfView;
    }

    // in radians
    void setFieldOfView(const float fieldOfView) noexcept
    {
        _fieldOfView = fieldOfView;
        _projectionMatrixDirty = true;
    }

    bool getIsPerspective() const noexcept
    {
        return _isPerspective;
    }

    void setIsPerspective(const bool isPerspective) noexcept
    {
        _isPerspective = isPerspective;
        _projectionMatrixDirty = true;
    }

    float getNearPlane() const noexcept
    {
        return _nearPlane;
    }

    void setNearPlane(const float nearPlane) noexcept
    {
        _nearPlane = nearPlane;
        _projectionMatrixDirty = true;
    }

    float getFarPlane() const noexcept
    {
        return _farPlane;
    }

    void setFarPlane(const float farPlane) noexcept
    {
        _farPlane = farPlane;
        _projectionMatrixDirty = true;
    }

    void setAsMainCamera() const noexcept
    {
        s_main = getWeakRefAs<camera>();
    }

    const glm::mat4 &getProjectionMatrix() const noexcept
    {
        return _projectionMatrix;
    }

    const glm::mat4 &getViewMatrix() const noexcept
    {
        return _viewMatrix;
    }

    transform *getTransform() const noexcept
    {
        return _transform;
    }

    // updates the given transform such that it fills the viewport of this camera. also handles dirtying the given transform
    void setTransformAcrossViewPort(transform *transform, const float distanceFromNearClip) const
    {
        // position
        const auto pos = _transform->position + _transform->getForward() * (_nearPlane + distanceFromNearClip);
        if (pos != transform->position)
        {
            transform->position = pos;
            transform->markDirty();
        }
        // rotation
        const auto rot = glm::quatLookAt(-_transform->getForward(), _transform->getUp());
        if (rot != transform->rotation)
        {
            transform->rotation = rot;
            transform->markDirty();
        }
        // scale
        const float height = _isPerspective
                                 ? tan(getFieldOfView() * 0.5f) * (_nearPlane + distanceFromNearClip)
                                 : graphics::getFrameBufferSize().y;
        const float width = _isPerspective
                                ? height * graphics::getFrameBufferSize().x / graphics::getFrameBufferSize().y
                                : graphics::getFrameBufferSize().x;
        const glm::vec3 scale{width, height, _transform->scale.z};
        if (scale != transform->scale)
        {
            transform->scale = scale;
            transform->markDirty();
        }
    }

    // could be nullptr
    static inline camera *getMainCamera()
    {
        return s_main;
    }

  private:
    static inline camera *s_main = {};
    static inline quickVector<camera *> s_cameras{};
    bool _isPerspective = true;
    glm::mat4 _projectionMatrix;
    glm::mat4 _viewMatrix;
    // in radians
    float _fieldOfView = glm::radians(60.f);
    float _nearPlane = 0.01f;
    float _farPlane = 1000.f;
    transform *_transform;
    bool _projectionMatrixDirty = true;

    static inline void initialize_()
    {
        ensureExecutesOnce();
        static quickVector<camera *> dirties{};
        graphics::frameBufferSizeChanged.push_back([]() {
            s_cameras.forEach([](camera *instance) {
                instance->updateProjectionMatrix_();
                instance->_projectionMatrixDirty = false;
            });
        });
        // before transform
        application::postComponentHooks.insert(0, []() {
            bench("camera view matrix(pre)");
            dirties.clear();
            s_cameras.forEach([](camera *instance) {
                // mark for view matrix update
                if (instance->_transform->isDirty())
                    dirties.push_back(instance);

                // update projection matrices
                if (instance->_projectionMatrixDirty)
                {
                    instance->updateProjectionMatrix_();
                    instance->_projectionMatrixDirty = false;
                }
            });
        });
        // after transform
        application::postComponentHooks.push_back([]() {
            bench("camera view matrix(post)");
            dirties.forEach([](camera *instance) {
                instance->_viewMatrix = glm::inverse(instance->_transform->getGlobalMatrix());
            });
        });
    }

    bool created_() override
    {
        if (!(_transform = getEntity()->ensureComponentExists<transform>()))
            return false;
        _transform->pushLock();
        s_cameras.push_back(this);
        if (!s_main)
            s_main = this;
        initialize_();
        return true;
    }

    void removed_() override
    {
        _transform->popLock();
        s_cameras.erase(this);
        if (s_main == this)
            s_main = s_cameras.size() > 0 ? s_cameras.back() : nullptr;
    }

    void updateProjectionMatrix_()
    {
        const auto size = static_cast<glm::vec2>(graphics::getFrameBufferSize());
        _projectionMatrix =
            _isPerspective
                ? glm::perspectiveFov(_fieldOfView, size.x, size.y, _nearPlane, _farPlane)
                : glm::ortho(-size.x / 2.f, size.x / 2.f, -size.y / 2.f, size.y / 2.f, _nearPlane, _farPlane);
    }
};
} // namespace engine