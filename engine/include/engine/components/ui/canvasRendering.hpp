#pragma once

#include "engine/app.hpp"
#include "engine/components/transform.hpp"

namespace engine::ui
{
struct canvas;

// 2D transform for UI elements
struct uiTransform : public transform
{
    createTypeInformation(uiTransform, transform);

    friend canvas;

    // anchor in parent space (takes up bottom-left)
    glm::vec2 minAnchor{0.f, 0.f};

    // anchor in parent space (takes up top-right)
    glm::vec2 maxAnchor{1.f, 1.f};

    // size in canvas unit scale. pivot is considered center. takes effect after anchor
    glm::vec2 deltaSize{0.f, 0.f};

    void markDirty() noexcept override
    {
        transform::markDirty();
        _uiTransformDirty = true;
    }

  protected:
    // position of the normalized rectangle in canvas space (bottom-left point of the rect)
    glm::vec3 _canvasRectanglePosition;

    // size of the normalized rectangle in canvas space
    glm::vec3 _canvasRectangleSize;

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
        // order is 1 so it'll be after transform's hook
        application::postComponentHooks.insert(1, []() {
            bench("update uiTransforms");
            entity::forEachRootEntity([](const weakRef<entity> &ent) {
                glm::vec3 dummyVec3;
                glm::mat4 dummyMat4;
                updateCanvasRectRecursively_(dummyVec3, dummyMat4, *ent, dummyVec3, dummyVec3, false, false);
            });
        });
        application::hooksMutex.unlock();
    }

    // update canvas space rectangles of this uiTransform component and all its children's
    static inline void updateCanvasRectRecursively_(const glm::vec2 &canvasUnit, const glm::mat4 &canvasMatrix, entity &ent, const glm::vec3 &parentCanvasRectanglePosition, const glm::vec3 &parentCanvasRectangleSize, bool parentDirty, bool skipCanvas);
};

// renders UI
struct canvas : public component
{
    friend uiTransform;

    createTypeInformation(canvas, component);

    void markDirty() noexcept
    {
        _dirty = true;
    }

  protected:
    // scale unit for this canvas in canvas-space
    glm::vec2 _unit{1, 1};
    weakRef<transform> _transform;

    bool _dirty;

    // returns first parent
    static inline weakRef<canvas> findClosestCanvas_(const entity *startingEntity)
    {
        return startingEntity->getComponentInParent<canvas>();
    }

    void created_() override
    {
        _transform = getEntity()->ensureComponentExists<transform>();
    }
};

#ifndef _CANVAS_RENDERING_H
#define _CANVAS_RENDERING_H

inline void uiTransform::updateCanvasRectRecursively_(const glm::vec2 &canvasUnit, const glm::mat4 &canvasMatrix, entity &ent, const glm::vec3 &parentCanvasRectanglePosition, const glm::vec3 &parentCanvasRectangleSize, bool parentDirty, bool skipCanvas)
{
    canvas *canvasPtr;
    if (!skipCanvas && (canvasPtr = ent.getComponent<canvas>()))
    {
        if (!canvasPtr->_transform)
        {
            canvasPtr->_transform = canvasPtr->getEntity()->addComponent<transform>();
            log::logError("canvas \"{}\" did not have a transform component. Added one.", canvasPtr->getEntity()->name);
        }
        glm::vec3 pos{0, 0, 0}, scale{1, 1, 1};
        uiTransform::updateCanvasRectRecursively_(canvasPtr->_unit, canvasPtr->_transform->getGlobalMatrix(), ent, pos, scale, canvasPtr->_dirty, true);
        canvasPtr->_dirty = false;
        return;
    }
    if (uiTransform *ptr = ent.getComponent<uiTransform>())
    {
        uiTransform &ref = *ptr;
        if (ref._uiTransformDirty || parentDirty)
        {
            // update canvas-space rect
            ref._canvasRectanglePosition = parentCanvasRectanglePosition +
                                           glm::vec3(ref.minAnchor, 0) * parentCanvasRectangleSize +             // anchor size
                                           glm::vec3(ref.deltaSize / 2.f * canvasUnit, 0) +                      // delta size
                                           ref.position * glm::vec3(canvasUnit, 1);                              // position delta
            ref._canvasRectangleSize = parentCanvasRectangleSize * glm::vec3(ref.maxAnchor - ref.minAnchor, 0) + // anchor size
                                       glm::vec3(ref.deltaSize / 2.f * canvasUnit, 0) *                          // delta size
                                           ref.scale;                                                            // scale
            // update matrix
            ref._modelMatrix = glm::translate(glm::mat4(1), ref._canvasRectanglePosition) * // position
                               glm::mat4_cast(ref.rotation) *                               // rotation
                               glm::scale(glm::mat4(1), ref._canvasRectangleSize);          // scale
            ref._modelGlobalMatrix = canvasMatrix * ref._modelMatrix;
            ref._uiTransformDirty = false;
            parentDirty = true;
        }
        ent.forEachChild([&](const weakRef<entity> &child) {
            updateCanvasRectRecursively_(canvasUnit, canvasMatrix, *child, ref._canvasRectanglePosition, ref._canvasRectangleSize, parentDirty, false);
        });
        return;
    }
    ent.forEachChild([&](const weakRef<entity> &child) {
        updateCanvasRectRecursively_(canvasUnit, canvasMatrix, *child, parentCanvasRectanglePosition, parentCanvasRectangleSize, parentDirty, false);
    });
}

#endif

} // namespace engine::ui
