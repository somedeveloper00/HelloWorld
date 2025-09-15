#pragma once

#include "common/typeInfo.hpp"
#include "engine/components/ui/uiSelectable.hpp"
#include "engine/log.hpp"

struct pointerDebug : public engine::ui::uiSelectable
{
    createTypeInformation(pointerDebug, engine::ui::uiSelectable);

  protected:
    void onPointerEnter() override
    {
        engine::log::logInfo("onPointerEnter");
    }

    void onPointerExit() override
    {
        engine::log::logInfo("onPointerExit");
    }

    void onDown() override
    {
        engine::log::logInfo("onDown");
    }

    void onUp() override
    {
        engine::log::logInfo("onUp");
    }
};