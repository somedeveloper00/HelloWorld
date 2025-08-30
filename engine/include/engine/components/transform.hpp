#pragma once

#include "componentUtility.hpp"
#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/components/componentUtility.hpp"
#include "engine/quickVector.hpp"
#include "engine/ref.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace engine
{
struct transform : public component
{
    static inline auto &s_postUpdateHooks = *new quickVector<std::function<void(entity &)>>();
    glm::vec3 position{};
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};
    glm::mat4 translateMatrix;
    glm::mat4 rotationMatrix;
    glm::mat4 scaleMatrix;
    glm::mat4 modelGlobalMatrix;

    glm::vec3 getForward() const
    {
        return rotation * glm::vec3(0.f, 0.f, -1.f);
    }

    glm::vec3 getUp() const
    {
        return rotation * glm::vec3(0.f, 1.f, 0.f);
    }

    void markDirtyRecursively() noexcept
    {
        _dirty = true;
    }

  private:
    bool _dirty = true;

    static inline void initialize_()
    {
        executeOnce();
        application::hooksMutex.lock();
        // application::postComponentHooks.insert(application::postComponentHooks.begin(), []() {
        //     bench("update transforms");
        //     // update matrices
        //     for (size_t i = 0; i < entity::getRootEntitiesCount(); i++)
        //     {
        //         glm::mat4 globalModelMatrix{0.1f};
        //         updateModelMatricesRecursively_(*entity::getRootEntityAt(i).get(), globalModelMatrix, false);
        //     }
        // });
        application::hooksMutex.unlock();
    }

    static inline void updateModelMatricesRecursively_(entity &ent, glm::mat4 &parentModelGlobalMatrix, bool parentDirty)
    {
        {
            bench("ent.getComponent<transform>()");
            if (transform *ptr = ent.getComponent<transform>())
            {
                bench("inner");
                transform &transformRef = *ptr;
                if (transformRef._dirty)
                {
                    bench("update local and global matrices");
                    // transformRef.translateMatrix = glm::translate(glm::mat4(1), transformRef.position);
                    // transformRef.rotationMatrix = glm::mat4_cast(transformRef.rotation);
                    // transformRef.scaleMatrix = glm::scale(glm::mat4(1), transformRef.scale);
                    // transformRef.modelGlobalMatrix = parentModelGlobalMatrix = parentModelGlobalMatrix * transformRef.modelMatrix;
                    transformRef._dirty = false;
                    parentDirty = true;
                }
                else if (parentDirty)
                {
                    bench("update global matrix");
                    // transformRef.modelGlobalMatrix = parentModelGlobalMatrix * transformRef.modelMatrix;
                }
            }
        }
        {
            bench("transform's postUpdateHooks");
            s_postUpdateHooks.forEach([&](const auto &hook) { hook(ent); });
        }
        for (size_t i = 0; i < ent.getChildrenCount(); i++)
            updateModelMatricesRecursively_(*(entity *)ent.getChildAt(i), parentModelGlobalMatrix, parentDirty);
    }

    void created_() override
    {
        initialize_();
        auto trans = getEntity()->getComponent<transform>();
        if (trans && trans != getWeakRef())
            log::logWarning("Cannot have more than one engine::transform in one entity. Only the first one will be used.");
    }
};
} // namespace engine