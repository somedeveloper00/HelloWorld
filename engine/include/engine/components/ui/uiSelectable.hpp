#pragma once

#include "common/componentUtils.hpp"
#include "engine/app.hpp"
#include "engine/components/pointerRead.hpp"
#include "engine/ref.hpp"
#include "engine/window.hpp"

namespace engine::ui
{
// an object that's selectable. usually inherited
struct uiSelectable : public component
{
    createTypeInformation(uiSelectable, component);

    // get currently selected item. could be nullptr
    static inline weakRef<uiSelectable> getSelected() noexcept
    {
        return s_selected;
    }

    // set currently seleccted item
    static inline void setSelected(const weakRef<uiSelectable> &instance) noexcept
    {
        if (s_selected == instance)
            return;
        s_selected = instance;
        instance->onUnselected();
        if (s_selected)
            s_selected->onSelected();
    }

  protected:
    static inline weakRef<uiSelectable> s_selected;
    pointerRead *_pointerRead;
    std::function<void()> _onPointerEnterLambda;
    std::function<void()> _onPointerExitLambda;
    bool _isHovered;

    void created_() override
    {
        disallowMultipleComponents(uiSelectable);
        _pointerRead = getEntity()->ensureComponentExists<pointerRead>();
        _pointerRead->pushLock();
        _onPointerEnterLambda = [this]() {
            _isHovered = true;
            onPointerEnter();
        };
        _onPointerExitLambda = [this]() {
            _isHovered = false;
            onPointerExit();
        };
        _pointerRead->onPointerEnter.push_back(_onPointerEnterLambda);
        _pointerRead->onPointerExit.push_back(_onPointerExitLambda);
        initialize_();
    }

    void removed_() override
    {
        _pointerRead->popLock();
        {
            const auto target = _onPointerEnterLambda.target<void (*)()>();
            _pointerRead->onPointerEnter.eraseIf([target](const auto &func) {
                return func.template target<void (*)()>() == target;
            });
        }
        {
            const auto target = _onPointerExitLambda.target<void (*)()>();
            _pointerRead->onPointerExit.eraseIf([target](const auto &func) {
                return func.template target<void (*)()>() == target;
            });
        }
    }

    // called when pointer enters it (doesn't get called when using keyboard navigation)
    virtual void onPointerEnter()
    {
    }

    // called when pointer exists it (doesn't get called when using keyboard navigation)
    virtual void onPointerExit()
    {
    }

    // called when pointer is down on this
    virtual void onDown()
    {
    }

    // called when pointer is up, after it was down on this
    virtual void onUp()
    {
    }

    // called when this is selected. this is called after onUnselected()
    virtual void onSelected()
    {
    }

    // called when this is unselected (happens after being selected). this is called before onSelected()
    virtual void onUnselected()
    {
    }

  private:
    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::hooksMutex.lock();
        application::preComponentHooks.push_back([]() {
            static bool mouseLeftUp = false;
            if (input::isKeyJustDown(input::key::mouseLeft))
            {
                mouseLeftUp = false;
                if (s_selected)
                    s_selected->onDown();
            }
            else if (!mouseLeftUp && input::isKeyUp(input::key::mouseLeft))
            {
                mouseLeftUp = true;
                if (s_selected)
                    s_selected->onUp();
            }
        });
        application::hooksMutex.unlock();
    }
};
} // namespace engine::ui