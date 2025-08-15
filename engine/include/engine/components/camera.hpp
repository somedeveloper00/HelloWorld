#pragma once

#include "../app.hpp"
#include "transform.hpp"

namespace engine
{
struct camera : public engine::component
{
  private:
    void created_() override
    {
        if (getEntity()->getComponent<transform>() == nullptr)
            getEntity()->addComponent<transform>();
    }
};
} // namespace engine