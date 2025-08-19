#pragma once

#include "componentUtility.hpp"
#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/components/componentUtility.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace engine
{
struct transform : public component
{
    static inline auto &s_postUpdateHooks = *new std::vector<std::function<void(entity &)>>();
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
            if (auto ptr = ent.getComponent<transform>())
            {
                bench("inner");
                auto &ref = *ptr.get();
                if (ref._dirty)
                {
                    bench("update local and global matrices");
                    // ref.translateMatrix = glm::translate(glm::mat4(1), ref.position);
                    // ref.rotationMatrix = glm::mat4_cast(ref.rotation);
                    // ref.scaleMatrix = glm::scale(glm::mat4(1), ref.scale);
                    // ref.modelGlobalMatrix = parentModelGlobalMatrix = parentModelGlobalMatrix * ref.modelMatrix;
                    ref._dirty = false;
                    parentDirty = true;
                }
                else if (parentDirty)
                {
                    bench("update global matrix");
                    // ref.modelGlobalMatrix = parentModelGlobalMatrix * ref.modelMatrix;
                }
            }
        }
        {
            bench("postUpdateHooks");
            for (auto &hook : s_postUpdateHooks)
                hook(ent);
        }
        for (size_t i = 0; i < ent.getChildrenCount(); i++)
            updateModelMatricesRecursively_(*ent.getChildAt(i).get(), parentModelGlobalMatrix, parentDirty);
    }

    void created_() override
    {
        initialize_();
        if (getEntity()->getComponent<transform>())
            log::logWarning("Cannot have more than one engine::transform in one entity. Only the first one will be used.");
    }
};
} // namespace engine