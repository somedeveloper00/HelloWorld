#pragma once

#include "engine/app.hpp"
#include "engine/components/transform.hpp"
#include <memory>

namespace engine::ui
{
struct uiTransform : public component
{
    glm::vec2 minAnchor{0.f, 0.f};
    glm::vec2 maxAnchor{1.f, 1.f};
    glm::vec2 minOffset{0.f, 0.f};
    glm::vec2 maxOffset{0.f, 0.f};

  private:
    static inline std::vector<uiTransform *> s_instances{};
    glm::mat4 _modelMatrix;

    static inline void initialize_()
    {
        executeOnce();
        application::hooksMutex.lock();
        application::postComponentHooks.push_back([]() {
            for (auto &instance : s_instances)
                if (auto transformPtr = instance->getEntity()->getComponent<transform>())
                {
                    auto &globalModelMatrix = transformPtr->modelGlobalMatrix;
                    // TODO: update the matrix
                }
        });
        application::hooksMutex.unlock();
    }

    void created_() override
    {
        initialize_();
        // ensure transform component exists
        if (!getEntity()->getComponent<transform>())
            getEntity()->addComponent<transform>();
        s_instances.push_back(this);
    }

    void removed_() override
    {
        s_instances.erase(std::remove(s_instances.begin(), s_instances.end(), this), s_instances.end());
    }
};
} // namespace engine::ui