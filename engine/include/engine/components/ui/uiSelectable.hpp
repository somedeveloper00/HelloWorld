#pragma once

#include "engine/app.hpp"
#include "engine/components/pointerRead.hpp"
#include "engine/components/ui/canvasRendering.hpp"
#include "engine/window.hpp"

namespace engine::ui
{
// an object that's selectable. usually inherited. disabling this will make this object not selectable without removing the component entirely
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
        if (s_selected)
            s_selected->onUnselected();
        s_selected = instance;
        if (s_selected)
            s_selected->onSelected();
    }

    // select self
    void select() const noexcept
    {
        setSelected(_uiSelectableWeakRef);
    }

    // deselect self
    void unselect() const noexcept
    {
        if (s_selected == this)
            setSelected({});
    }

  protected:
    pointerRead *_pointerRead;
    uiTransform *_uiTransform;

    bool created_() override
    {
        disallowMultipleComponents(uiSelectable);
        if (!(_uiTransform = getEntity()->ensureComponentExists<uiTransform>()))
            return false;
        if (!(_pointerRead = getEntity()->ensureComponentExists<pointerRead>()))
            return false;
        initialize_();
        _uiSelectableWeakRef = getWeakRefAs<uiSelectable>();
        _uiTransform->pushLock();
        _pointerRead->pushLock();
        _pointerRead->onPointerEnter.push_back(_onPointerEnterLambda = [this]() {
            _isHovered = true;
            onPointerEnter();
            s_hovered = getWeakRefAs<uiSelectable>();
        });
        _pointerRead->onPointerExit.push_back(_onPointerExitLambda = [this]() {
            _isHovered = false;
            onPointerExit();
            s_hovered = {};
        });
        return true;
    }

    void removed_() override
    {
        _uiTransform->popLock();
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

    void disabled_() override
    {
        unselect();
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
    virtual void onPointerDown()
    {
    }

    // called when pointer is up, after it was down on this
    virtual void onPointerUp()
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
    static inline weakRef<uiSelectable> s_selected, s_hovered;
    std::function<void()> _onPointerEnterLambda;
    std::function<void()> _onPointerExitLambda;
    weakRef<uiSelectable> _uiSelectableWeakRef;
    bool _isHovered;

    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::preComponentHooks.push_back([]() {
            static bool mouseLeftUp = false;
            if (input::isKeyJustDown(input::key::mouseLeft))
            {
                mouseLeftUp = false;
                if (s_hovered)
                    s_hovered->onPointerDown();
            }
            else if (!mouseLeftUp && input::isKeyUp(input::key::mouseLeft))
            {
                mouseLeftUp = true;
                if (s_hovered)
                    s_hovered->onPointerUp();
            }
        });
    }
};
} // namespace engine::ui