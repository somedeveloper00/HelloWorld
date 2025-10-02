#pragma once

#include "uiLayout.hpp"
#include <cstdint>

namespace engine::ui
{
// Horizontal layout components
struct uiHorizontalLayout : public uiLayout
{
    createTypeInformation(uiHorizontalLayout, uiLayout);

    enum direction : uint8_t
    {
        leftToRight = 0,
        rightToLeft = 1,
    };

    direction direction = direction::leftToRight;

    // spacing between children
    float spacing;

    // if true, the layout will expand to fit its children plus their paddings
    bool expandToFitChildren = false;

  protected:
    void updateHorizontallyNonDependantLayout_() override
    {
        
    }
};
} // namespace engine::ui