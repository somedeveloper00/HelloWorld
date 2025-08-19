#pragma once

#include "componentUtility.hpp"
#include "engine/app.hpp"
#include "transform.hpp"
#include <memory>

namespace engine
{
struct camera : public component
{
    glm::mat4 viewMatrix{};

  private:
    std::weak_ptr<transform> _transformPtr;

    static inline void initialize_()
    {
        executeOnce();
    }

    void created_() override
    {
        initialize_();
        std::shared_ptr<transform> ptr;
        if ((ptr = getEntity()->getComponent<transform>()))
            ptr = getEntity()->addComponent<transform>();
        _transformPtr = ptr;
    }
};
} // namespace engine