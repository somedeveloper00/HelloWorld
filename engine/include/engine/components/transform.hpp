#pragma once

#include "../app.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "glm/gtc/quaternion.hpp"
#include <atomic>
#include <memory>
#include <mutex>

namespace engine
{
struct transform : public engine::component
{
    glm::vec3 position{};
    glm::vec3 scale{};
    glm::quat rotation{};

    void created_() override
    {
        static std::atomic_bool s_hooked = false;
        if (s_hooked)
            return;
        application::postComponentHooks.insert(application::postComponentHooks.begin(), []() {
            std::scoped_lock lock(s_dirtyTransformsMutex);
            // update model matrices of dirty transforms
            glm::mat4 t, r, s;
            for (auto &trans : s_dirtyTransforms)
            {
                if (trans.expired())
                    continue;
                transform &ref = *dynamic_cast<transform *>(trans.lock().get());
                t = glm::translate(glm::mat4(1), ref.position);
                r = glm::mat4_cast(ref.rotation);
                s = glm::scale(glm::mat4(1), ref.scale);
                ref._modelMatrix = t * r * s;
                ref._dirty = false;
            }
            s_dirtyTransforms.clear();
        });
        s_hooked = true;
    }

    glm::vec3 getForward() const
    {
        return rotation * glm::vec3(0.f, 0.f, -1.f);
    }

    glm::vec3 getUp() const
    {
        return rotation * glm::vec3(0.f, 1.f, 0.f);
    }

    glm::mat4 getModelMatrix() const
    {
        return _modelMatrix;
    }

    void markDirty()
    {
        if (!_dirty)
        {
            _dirty = true;
            s_dirtyTransformsMutex.lock();
            s_dirtyTransforms.push_back(getSelfRef());
        }
    }

    void recalcModelMatrixRecursive()
    {
        markDirty();
        std::vector<std::shared_ptr<transform>> buff{10};
        for (auto &child : getEntity()->getChildren())
        {
            child->getComponents(buff);
            for (auto &transform : buff)
                transform->recalcModelMatrixRecursive();
            buff.clear();
        }
    }

  private:
    static inline auto &s_dirtyTransforms = *new std::vector<std::weak_ptr<component>>();
    static inline std::mutex s_dirtyTransformsMutex;

    bool _dirty;
    glm::mat4 _modelMatrix;
};
} // namespace engine