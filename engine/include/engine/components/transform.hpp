#pragma once

#include "common/componentUtils.hpp"
#include "engine/app.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace engine
{
struct transform : public component
{
    createTypeInformation(transform, component);

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

  protected:
    glm::mat4 _modelMatrix;
    glm::mat4 _modelGlobalMatrix;
    bool _dirty = true;

    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::hooksMutex.lock();
        application::postComponentHooks.insert(0, []() {
            bench("update transforms");
            // update matrices
            for (size_t i = 0; i < entity::getRootEntitiesCount(); i++)
            {
                glm::mat4 globalModelMatrix{0.1f};
                updateModelMatricesRecursively_(entity::getRootEntityAt(i), globalModelMatrix, false);
            }
        });
        application::hooksMutex.unlock();
    }

    static inline void updateModelMatricesRecursively_(entity *ent, glm::mat4 &parentModelGlobalMatrix, bool parentDirty)
    {
        if (transform *ptr = ent->getComponent<transform>())
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
                transformRef._modelGlobalMatrix = parentModelGlobalMatrix * transformRef._modelMatrix;
        }
        for (size_t i = 0; i < ent->getChildrenCount(); i++)
            updateModelMatricesRecursively_(ent->getChildAt(i), parentModelGlobalMatrix, parentDirty);
    }

    void created_() override
    {
        initialize_();
        if (checkForDuplicateTransforms_())
            remove();
    }

    // checks if this component's entity has one transform component or not
    bool checkForDuplicateTransforms_()
    {
        static quickVector<weakRef<transform>> buff;
        getEntity()->getComponents<transform>(buff);
        if (buff.size() > 1)
        {
            buff.clear();
            log::logWarning("Entity \"{}\" has more than one \"{}\" components, which is not allowed.", getEntity()->name, getTypeName());
            return true;
        }
        buff.clear();
        return false;
    }
};
} // namespace engine