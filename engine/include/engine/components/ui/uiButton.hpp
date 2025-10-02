#pragma once

#include "uiSelectable.hpp"

namespace engine::ui
{
// base button implementation. should be implemented and expanded for visual representation of the button
struct uiButton : public uiSelectable
{
    createTypeInformation(uiButton, uiSelectable);

    quickVector<std::function<void()>> onClick;

  protected:
    bool _pressed = false;

    void onPointerDown() override
    {
        _pressed = true;
    }

    void onPointerUp() override
    {
        if (_pressed)
            onClick.forEach([](const auto &func) { func(); });
        _pressed = false;
    }
};
} // namespace engine::ui