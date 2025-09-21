#pragma once

#include "common/componentUtils.hpp"
#include "engine/app.hpp"
#include "engine/components/transform.hpp"
#include "engine/quickVector.hpp"
#include "engine/window.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace engine::ui
{
struct canvas;

// 2D transform for UI elements
struct uiTransform : public transform
{
    createTypeInformation(uiTransform, transform);

    friend canvas;

    uiTransform()
        : transform(true)
    {
    }

    // anchor in parent space (takes up bottom-left)
    glm::vec2 minAnchor{0.f, 0.f};

    // anchor in parent space (takes up top-right)
    glm::vec2 maxAnchor{1.f, 1.f};

    // size in canvas unit scale. pivot is considered center. takes effect after anchor
    glm::vec2 deltaSize{0.f, 0.f};

    // normalized point to determine the pivot of this uiTransform (central point)
    glm::vec2 pivot{0.f, 0.f};

    void markDirty() noexcept override
    {
        transform::markDirty();
        _uiTransformDirty = true;
    }

  protected:
    // used for optimization
    bool _uiTransformDirty = true;

    void created_() override
    {
        transform::created_();
        initialize_();
    }

  private:
    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::hooksMutex.lock();
        application::postComponentHooks.push_back([]() {
            bench("update uiTransforms");
            entity::forEachRootEntity([](const weakRef<entity> &ent) {
                updateMatricesRecursively_({}, {1}, *ent, false, false);
            });
        });
        application::hooksMutex.unlock();
        graphics::frameBufferSizeChanged.push_back([]() {
            bench("update uiTransforms");
            entity::forEachRootEntity([](const weakRef<entity> &ent) {
                updateMatricesRecursively_({}, {1}, *ent, true, false);
            });
        });
    }

    // update canvas space rectangles of this uiTransform component and all its children's
    static inline void updateMatricesRecursively_(const glm::vec2 &canvasUnit, const glm::mat4 &parentGlobalMatrix, entity &ent, bool parentDirty, bool skipCanvas);
};

// renders UI
struct canvas : public component
{
    friend uiTransform;

    // increases unit scale to match screen pixels
    bool scaleToScreenPixels = 1;

    // multiplier for scale unit
    float scaleUnitMultiplier = 1.f;

    createTypeInformation(canvas, component);

  protected:
    // scale unit for this canvas in canvas-space
    glm::vec2 _unit{1, 1};
    weakRef<transform> _transform;

    // returns first parent
    static inline weakRef<canvas> findClosestCanvas_(const entity *startingEntity)
    {
        return startingEntity->getComponentInParent<canvas>();
    }

    void created_() override
    {
        s_canvases.push_back(this);
        _transform = getEntity()->ensureComponentExists<transform>();
        initialize_();
        _transform->pushLock();
        updateScaleUnit_();

        // add hook
        _frameBufferChangedHook = [this]() { this->updateScaleUnit_(); };
        graphics::frameBufferSizeChanged.push_back(_frameBufferChangedHook);
    }

    void removed_() override
    {
        s_canvases.erase(this);
        _transform->popLock();

        // remove hook
        auto target = _frameBufferChangedHook.target<void (*)()>();
        graphics::frameBufferSizeChanged.eraseIf([target](const auto &func) {
            return func.template target<void (*)()>() == target;
        });
    }

  private:
    static inline quickVector<canvas *> s_canvases;
    std::function<void()> _frameBufferChangedHook;
    bool _dirty;

    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::postComponentHooks.insert(0, []() {
            s_canvases.forEach([](canvas *instance) {
                instance->_dirty = instance->_transform->isDirty();
            });
        });
    }

    void updateScaleUnit_()
    {
        _unit = 1.f / (scaleToScreenPixels ? static_cast<glm::vec2>(graphics::getFrameBufferSize()) : glm::vec2(1.f)) * scaleUnitMultiplier;
        _transform->markDirty();
    }
};

#ifndef _CANVAS_RENDERING_H
#define _CANVAS_RENDERING_H

inline void uiTransform::updateMatricesRecursively_(const glm::vec2 &canvasUnit, const glm::mat4 &parentGlobalMatrix, entity &ent, bool parentDirty, bool skipCanvas)
{
    canvas *canvasPtr;
    if (!skipCanvas && (canvasPtr = ent.getComponent<canvas>()))
    {
        uiTransform::updateMatricesRecursively_(canvasPtr->_unit, canvasPtr->_transform->getGlobalMatrix(), ent, canvasPtr->_dirty | parentDirty, true);
        return;
    }
    if (uiTransform *ptr = ent.getComponent<uiTransform>())
    {
        uiTransform &ref = *ptr;
        if (ref._uiTransformDirty || parentDirty)
        {
            // update local
            const glm::vec3 scale = ref.scale *                                 // scale
                                    glm::vec3((ref.maxAnchor - ref.minAnchor) + // anchor size
                                                  ref.deltaSize * canvasUnit,   // delta size
                                              1);
            const glm::vec3 position = glm::vec3((ref.minAnchor + ref.maxAnchor) - 1.f - ref.pivot, 0) + // anchor size and pivot
                                       ref.position * glm::vec3(canvasUnit * 2.f, 1);                          // position delta
            const glm::quat rotation = ref.rotation;                                                     // no conversion
            // update matrix
            ref._modelMatrix = glm::translate(glm::mat4(1), position) * // position
                               glm::mat4_cast(ref.rotation) *           // rotation
                               glm::scale(glm::mat4(1), scale);         // scale
            ref._modelGlobalMatrix = parentGlobalMatrix * ref._modelMatrix;
            ref._uiTransformDirty = false;
            parentDirty = true;
        }
        ent.forEachChild([&](const weakRef<entity> &child) {
            updateMatricesRecursively_(canvasUnit, ref._modelGlobalMatrix, *child, parentDirty, false);
        });
        return;
    }
    ent.forEachChild([&](const weakRef<entity> &child) {
        updateMatricesRecursively_(canvasUnit, parentGlobalMatrix, *child, parentDirty, false);
    });
}

#endif

} // namespace engine::ui
