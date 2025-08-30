#pragma once

#include "componentUtility.hpp"
#include "engine/app.hpp"
#include "transform.hpp"

namespace engine
{
struct camera : public component
{
    glm::mat4 viewMatrix{};

  private:
    weakRef<transform> _transformPtr;

    static inline void initialize_()
    {
        executeOnce();
    }

    void created_() override
    {
        initialize_();
        _transformPtr = getEntity()->ensureComponentExists<transform>();
    }
};
} // namespace engine