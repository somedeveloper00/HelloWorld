#pragma once

#include "common/typeInfo.hpp"
#include "engine/components/ui/uiImage.hpp"
#include "engine/components/ui/uiSelectable.hpp"
#include "engine/log.hpp"
#include "engine/window.hpp"
#include <gl/gl.h>

struct pointerDebug : public engine::ui::uiSelectable
{
    createTypeInformation(pointerDebug, engine::ui::uiSelectable);

  protected:
    bool created_() override
    {
        if (!engine::ui::uiSelectable::created_())
            return false;
        const auto image = getEntity()->ensureComponentExists<engine::ui::uiImage>();
        if (!image)
            return false;
        image->pushLock();
        _pointerRead->setVertices(engine::graphics::opengl::getSquareVao(), 6);
        return true;
    }

    void removed_() override
    {
        getEntity()->getComponent<engine::ui::uiImage>()->popLock();
    }

    void onPointerEnter() override
    {
        getEntity()->getComponent<engine::ui::uiImage>()->color = {1, 0, 0, 1};
        engine::log::logInfo("\"{}\" onPointerEnter", getEntity()->name);
    }

    void onPointerExit() override
    {
        getEntity()->getComponent<engine::ui::uiImage>()->color = {0, 0, 0, 1};
        engine::log::logInfo("\"{}\" onPointerExit", getEntity()->name);
    }

    void onPointerDown() override
    {
        getEntity()->getComponent<engine::ui::uiImage>()->color = {0, 1, 0, 1};
        engine::log::logInfo("\"{}\" onDown", getEntity()->name);
    }

    void onPointerUp() override
    {
        getEntity()->getComponent<engine::ui::uiImage>()->color = {0, 0, 1, 1};
        engine::log::logInfo("\"{}\" onUp", getEntity()->name);
    }
};