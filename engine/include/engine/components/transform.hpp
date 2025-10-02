#pragma once

#include "common/componentUtils.hpp"
#include "engine/app.hpp"
#include "engine/window.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace engine
{
// 3D transform. children of this component must also calculate their own matrices
struct transform : public component
{
    createTypeInformation(transform, component);

    transform() = default;

    glm::vec3 position{};
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};

    glm::vec3 getForward() const
    {
        return rotation * glm::vec3(0.f, 0.f, -1.f);
    }

    glm::vec3 getUp() const
    {
        return rotation * glm::vec3(0.f, 1.f, 0.f);
    }

    glm::mat4 getGlobalMatrix() const noexcept
    {
        return _modelGlobalMatrix;
    }

    glm::mat4 getMatrix() const noexcept
    {
        return _modelMatrix;
    }

    virtual void markDirty() noexcept
    {
        _dirty = true;
    }

    const bool isDirty() const noexcept
    {
        return _dirty;
    }

  protected:
    glm::mat4 _modelMatrix;
    glm::mat4 _modelGlobalMatrix;
    bool _dirty = true;

    // set this to false to override matrix multiplication
    const bool _overrideMatrixCalculation = false;

    transform(bool overrideMatrixCalculation)
        : _overrideMatrixCalculation(overrideMatrixCalculation)
    {
    }

    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::postComponentHooks.insert(0, []() {
            bench("update transforms");
            // update matrices
            for (size_t i = 0; i < entity::getRootEntitiesCount(); i++)
            {
                glm::mat4 globalModelMatrix{1};
                updateModelMatricesRecursively_(entity::getRootEntityAt(i), globalModelMatrix, false);
            }
        });
    }

    static inline void updateModelMatricesRecursively_(entity *ent, glm::mat4 &parentModelGlobalMatrix, bool parentDirty)
    {
        transform *ptr;
        if ((ptr = ent->getComponent<transform>()) && !ptr->_overrideMatrixCalculation)
        {
            transform &transformRef = *ptr;
            if (transformRef._dirty)
            {
                const auto scaleMatrix = glm::scale(glm::mat4(1), transformRef.scale);
                const auto rotationMatrix = glm::mat4_cast(transformRef.rotation);
                const auto translateMatrix = glm::translate(glm::mat4(1), transformRef.position);
                transformRef._modelMatrix = translateMatrix * rotationMatrix * scaleMatrix;
                transformRef._modelGlobalMatrix = parentModelGlobalMatrix = parentModelGlobalMatrix * transformRef._modelMatrix;
                transformRef._dirty = false;
                parentDirty = true;
            }
            else if (parentDirty)
                transformRef._modelGlobalMatrix = parentModelGlobalMatrix = parentModelGlobalMatrix * transformRef._modelMatrix;
        }
        ent->forEachChild([&parentModelGlobalMatrix, &parentDirty](entity *child) {
            updateModelMatricesRecursively_(child, parentModelGlobalMatrix, parentDirty);
        });
    }

    void created_() override
    {
        initialize_();
        disallowMultipleComponents(transform);
    }
};
} // namespace engine