#pragma once

#include "common/componentUtils.hpp"
#include "engine/app.hpp"
#include "transform.hpp"

namespace engine
{
struct camera : public component
{
    createTypeInformation(camera, component);

    glm::mat4 viewMatrix{};

  private:
    weakRef<transform> _transformPtr;

    static inline void initialize_()
    {
        ensureExecutesOnce();
    }

    void created_() override
    {
        initialize_();
        _transformPtr = getEntity()->ensureComponentExists<transform>();
    }
};
} // namespace engine