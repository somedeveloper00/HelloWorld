#pragma once

#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/transform.hpp"
#include "engine/quickVector.hpp"
#include "engine/window.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include <algorithm>

namespace engine::ui
{
struct canvas;

// 2D transform for UI elements
struct uiTransform final : public transform
{
    friend canvas;

    createTypeInformation(uiTransform, transform);

    uiTransform()
        : transform(true)
    {
    }

    enum layout
    {
        // no layouting. corresponds to layoutNoneProperties
        none,
        // horizontal list layout. corresponds to layoutHorizontalProperties
        horizontal,
        // vertical list layout. corresponds to layoutVerticalProperties
        vertical
    };

    // define properties shared by layout types
#define CommonProps()                                                      \
    /* preferred size in canvas units scale */                             \
    glm::vec2 preferedSize{100, 100};                                      \
    /* space between this and its children at the four sides*/             \
    glm::vec4 margin{5, 5, 5, 5};                                          \
    /* space between this and its parent at the four sides*/               \
    glm::vec4 padding{5, 5, 5, 5};                                         \
    /* used in relation to its siblings when filling the parent layout.    \
    value of 0 in any axis means not filling the parent along that axis */ \
    glm::vec2 weight{0, 0};

#define ArrayLayoutCommonProps()                            \
    /* spacing in canvas unit scale between the children */ \
    float elementsSpacing{5};                               \
    /* if true, layout starts from the end along the axis*/ \
    bool startFromEnd{false};

    // properties for the layout::none layout type
    struct layoutNoneProperties
    {
        CommonProps();
    };

    // properties for the layout::horizontal layout type
    struct layoutHorizontalProperties
    {
        CommonProps();
        ArrayLayoutCommonProps();
    };

    // properties for the layout::vertical layout type
    struct layoutVerticalProperties
    {
        CommonProps();
        ArrayLayoutCommonProps();
    };

#undef ArrayLayoutCommonProps
#undef CommonProps

    // layout type for this uiTransform
    layout layout = layout::none;

    // properties for the selected layout type
    union {
        layoutNoneProperties none{}; // <- default
        layoutHorizontalProperties horizontal;
        layoutVerticalProperties vertical;
    } layoutProperties;

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

  private:
    // used for optimization
    bool _uiTransformDirty = true;

    // used for layout calculations
    glm::vec2 _calculatedPreferedSize{0, 0};

    bool created_() override
    {
        initialize_();
        if (!transform::created_())
            return false;
        return true;
    }

    float getWeightX() const noexcept
    {
        switch (layout)
        {
        case none:
            return layoutProperties.none.weight.x;
        case horizontal:
            return layoutProperties.horizontal.weight.x;
        case vertical:
            return layoutProperties.vertical.weight.x;
            break;
        }
        return 0;
    }

    float getWeightY() const noexcept
    {
        switch (layout)
        {
        case none:
            return layoutProperties.none.weight.y;
        case horizontal:
            return layoutProperties.horizontal.weight.y;
        case vertical:
            return layoutProperties.vertical.weight.y;
            break;
        }
        return 0;
    }

    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::postComponentHooks.push_back([]() {
            updateUiTransforms_(false);
        });
        graphics::frameBufferSizeChanged.push_back([]() {
            updateUiTransforms_(true);
        });
    }

    static inline void updateUiTransforms_(const bool parentDirty)
    {
        bench("update uiTransforms");

        entity::forEachRootEntity([](entity *ent) {
            // update layout | first pass | update their preferred sizes recursively
            recursivelyUpdateLayoutPreferedSizes_(ent);
            // update layout | second pass | fill
            recursivelyUpdateLayoutsPreferedSizesByFill_(ent);
            // update layout | third pass | actually update transforms
            recursivelyUpdateTransforms_(ent, {0, 0}, false);

            // update matrices
            updateMatricesRecursively_({}, {1}, *ent, true, false);
        });
    }

    // update canvas space rectangles of this uiTransform component and all its children's
    static inline void updateMatricesRecursively_(const glm::vec2 &canvasUnit, const glm::mat4 &parentGlobalMatrix, entity &ent, bool parentDirty, bool skipCanvas);

    // traverses parents from children upwards and sets their _calculatedPreferedSize regardless of fill weights
    static inline glm::vec2 recursivelyUpdateLayoutPreferedSizes_(const entity *ent);

    // traverses children from parent downwards and sets their children's _calculatedPreferedSize by fill weights
    static inline void recursivelyUpdateLayoutsPreferedSizesByFill_(const entity *ent);

    // traverses children from parent downwards and sets the properties that affect their matrices
    static inline void recursivelyUpdateTransforms_(const entity *ent, const glm::vec2 sizeUnit, const bool skipSelf);
};

// renders UI
struct canvas : public component
{
    friend uiTransform;

    createTypeInformation(canvas, component);

    enum positionType
    {
        // fits screen at all times (positions itself according to the main camera)
        fullScreen,

        // free transform
        world
    };

    struct fullScreenProperties
    {
        // distance from the rendering camera's nearClip
        float distanceFromNearClip = 1.f;

        // multiplier applied to unit scale (if 1, it'll be equal to pixel size)
        glm::vec2 unitScaleMultiplier{1.f, 1.f};
    };

    struct worldProperties
    {
        // overrides the unit scale of its children. value is in world space
        glm::vec2 unitScale{0.01f, 0.01f};
    };

    // current position type. updates are affected in the next frame
    positionType positionType = positionType::fullScreen;

    // properties for the selected position type
    union {
        fullScreenProperties fullScreen{}; // <- default
        worldProperties world;
    } positionProperties;

    void markDirty() noexcept
    {
        _dirty = true;
    }

  protected:
    // unit scale for this canvas in world space
    glm::vec2 _unitScale{1, 1};
    transform *_transform;

    bool created_() override
    {
        initialize_();
        disallowMultipleComponents(canvas);
        if (!(_transform = getEntity()->ensureComponentExists<transform>()))
            return false;
        s_canvases.push_back(this);
        _transform->pushLock();
        if (positionType == positionType::fullScreen)
        {
            setUnitScaleToFullScreen();
            moveToFullScreenPostion();
        }
        else
            setUnitScaleToWorld();
        return true;
    }

    void removed_() override
    {
        s_canvases.erase(this);
        _transform->popLock();
    }

  private:
    static inline quickVector<canvas *> s_canvases{};
    bool _dirty = true;

    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::postComponentHooks.insert(0, []() {
            s_canvases.forEach([](canvas *instance) {
                if (instance->positionType == positionType::fullScreen)
                    instance->moveToFullScreenPostion();
                else
                    instance->setUnitScaleToWorld();
                instance->_dirty |= instance->_transform->isDirty();
            });
        });
        graphics::frameBufferSizeChanged.push_back([]() {
            s_canvases.forEach([](canvas *instance) {
                if (instance->positionType == positionType::fullScreen)
                    instance->setUnitScaleToFullScreen();
            });
        });
    }

    // updates _unitScale
    void setUnitScaleToFullScreen() noexcept
    {
        _unitScale = positionProperties.fullScreen.unitScaleMultiplier / static_cast<glm::vec2>(graphics::getFrameBufferSize());
        markDirty();
    }
    // updates _unitScale
    void setUnitScaleToWorld() noexcept
    {
        if (_unitScale != positionProperties.world.unitScale)
        {
            _unitScale = positionProperties.world.unitScale;
            markDirty();
        }
    }

    void moveToFullScreenPostion()
    {
        const auto *camera = camera::getMainCamera();
        if (!camera)
            return;
        camera->setTransformAcrossViewPort(_transform, positionProperties.fullScreen.distanceFromNearClip);
    }
};

#ifndef _CANVAS_RENDERING_H
#define _CANVAS_RENDERING_H

inline void uiTransform::updateMatricesRecursively_(const glm::vec2 &canvasUnit, const glm::mat4 &parentGlobalMatrix, entity &ent, bool parentDirty, bool skipCanvas)
{
    canvas *canvasPtr;
    if (!skipCanvas && (canvasPtr = ent.getComponent<canvas>()))
    {
        uiTransform::updateMatricesRecursively_(canvasPtr->_unitScale, canvasPtr->_transform->getGlobalMatrix(), ent, canvasPtr->_dirty | parentDirty, true);
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
                                       ref.position * glm::vec3(canvasUnit * 2.f, 1);                    // position delta
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
    ent.forEachChild([&canvasUnit, &parentGlobalMatrix, parentDirty](entity *child) {
        updateMatricesRecursively_(canvasUnit, parentGlobalMatrix, *child, parentDirty, false);
    });
}

inline glm::vec2 uiTransform::recursivelyUpdateLayoutPreferedSizes_(const entity *ent)
{
    if (uiTransform *instance = ent->getComponent<uiTransform>())
    {
        switch (instance->layout)
        {
        case none: {
#define props instance->layoutProperties.none
            // get the max of its children
            glm::vec2 childrenMax{0, 0};
            ent->forEachChild([&childrenMax](entity *child) {
                const auto childSize = recursivelyUpdateLayoutPreferedSizes_(child);
                childrenMax.x = std::max(childSize.x, childrenMax.x);
                childrenMax.y = std::max(childSize.y, childrenMax.y);
            });
            // calculate final size
            instance->_calculatedPreferedSize = glm::vec2(
                // width
                std::max(childrenMax.x, props.preferedSize.x) + // must at least fit children
                    props.padding.x + props.padding.z           // sum of padding along x
                ,
                // height
                std::max(childrenMax.y, props.preferedSize.y) + // must at least fit children
                    props.padding.y + props.padding.w           // sum of padding along y
            );
            return instance->_calculatedPreferedSize;
#undef props
        }
        case horizontal: {
#define props instance->layoutProperties.horizontal
            // get the sum of its children
            glm::vec2 childrenSum{0, 0};
            ent->forEachChild([&childrenSum, spacing = props.elementsSpacing](entity *child) {
                const auto childSize = recursivelyUpdateLayoutPreferedSizes_(child);
                childrenSum.x += childSize.x + spacing;
                childrenSum.y = std::max(childSize.y, childrenSum.y);
            });
            // remove extra spacing (5 children will need 4 spacing)
            childrenSum.x -= childrenSum.x <= 0 ? 0 : props.elementsSpacing;
            // find final size
            instance->_calculatedPreferedSize = glm::vec2(
                // width
                std::max(childrenSum.x, props.preferedSize.x) // must at least fit children
                    + props.padding.x + props.padding.z       // sum of padding along X
                ,
                // height
                std::max(childrenSum.y, props.preferedSize.y) // must at least fit children
                    + props.padding.y + props.padding.w       // sum of padding along Y
            );
            return instance->_calculatedPreferedSize;
#undef props
        }
        case vertical: {
#define props instance->layoutProperties.vertical
            // get the sum of its children
            glm::vec2 childrenSum{0, 0};
            ent->forEachChild([&childrenSum, spacing = props.elementsSpacing](entity *child) {
                const auto childSize = recursivelyUpdateLayoutPreferedSizes_(child);
                childrenSum.y += childSize.y + spacing;
                childrenSum.x = std::max(childSize.x, childrenSum.x);
            });
            // remove extra spacing (5 children will need 4 spacing)
            childrenSum.y -= childrenSum.y <= 0 ? 0 : props.elementsSpacing;
            // find final size
            instance->_calculatedPreferedSize = glm::vec2(
                // width
                std::max(childrenSum.x, props.preferedSize.x) // must at least fit children
                    + props.padding.x + props.padding.z       // sum of padding along X
                ,
                // height
                std::max(childrenSum.y, props.preferedSize.y) // must at least fit children
                    + props.padding.y + props.padding.w       // sum of padding along Y
            );
            return instance->_calculatedPreferedSize;
#undef props
        }
        }
    }
    return {0, 0};
}

inline void uiTransform::recursivelyUpdateLayoutsPreferedSizesByFill_(const entity *ent)
{
    if (uiTransform *instance = ent->getComponent<uiTransform>())
    {
        struct weightedChild final
        {
            uiTransform *instance;
            const float weight;
        };
        // pooled vectors for later use
        static quickVector<weightedChild> s_weightedChildren;
        s_weightedChildren.clear();
        static quickVector<uiTransform *> s_nonOverflownChildren;
        s_nonOverflownChildren.clear();

        switch (instance->layout)
        {
        case none:
            // do nothing
            break;
        case horizontal: {
            float freeSpace = instance->_calculatedPreferedSize.x;
            float totalWeight = 0;
            ent->forEachChild([&freeSpace](entity *child) {
                if (uiTransform *transformChild = child->getComponent<uiTransform>())
                {
                    const auto weight = transformChild->getWeightX();
                    if (weight > 0)
                        s_weightedChildren.push_back({transformChild, weight});
                    else
                        freeSpace -= transformChild->_calculatedPreferedSize.x;
                }
            });
            // divide free space to weighted childrn (first pass | add to their size even if overflown)
            float weightUnit = totalWeight > 0 ? freeSpace / totalWeight : 0;
            s_weightedChildren.forEach([&freeSpace, weightUnit](const weightedChild child) {
                const auto share = weightUnit * child.weight; // share of this item from the free space
                if (child.instance->_calculatedPreferedSize.x < share)
                {
                    child.instance->_calculatedPreferedSize.x = share;
                    freeSpace -= share;
                    s_nonOverflownChildren.push_back(child.instance);
                }
                else
                    // overflowed its share
                    freeSpace -= child.instance->_calculatedPreferedSize.x;
            });
            if (freeSpace < 0)
                // second pass | remove overflowed space from non-overflown items
                s_nonOverflownChildren.forEach([removing = -freeSpace / s_nonOverflownChildren.size()](uiTransform *child) {
                    child->_calculatedPreferedSize.x -= removing;
                });
            break;
        }
        case vertical: {
            float freeSpace = instance->_calculatedPreferedSize.y;
            float totalWeight = 0;
            ent->forEachChild([&freeSpace](entity *child) {
                if (uiTransform *transformChild = child->getComponent<uiTransform>())
                {
                    const auto weight = transformChild->getWeightY();
                    if (weight > 0)
                        s_weightedChildren.push_back({transformChild, weight});
                    else
                        freeSpace -= transformChild->_calculatedPreferedSize.y;
                }
            });
            // divide free space to weighted childrn (first pass | add to their size even if overflown)
            float weightUnit = totalWeight > 0 ? freeSpace / totalWeight : 0;
            s_weightedChildren.forEach([&freeSpace, weightUnit](const weightedChild child) {
                const auto share = weightUnit * child.weight; // share of this item from the free space
                if (child.instance->_calculatedPreferedSize.y < share)
                {
                    child.instance->_calculatedPreferedSize.y = share;
                    freeSpace -= share;
                    s_nonOverflownChildren.push_back(child.instance);
                }
                else
                    // overflowed its share
                    freeSpace -= child.instance->_calculatedPreferedSize.y;
            });
            if (freeSpace < 0)
                // second pass | remove overflowed space from non-overflown items
                s_nonOverflownChildren.forEach([removing = -freeSpace / s_nonOverflownChildren.size()](uiTransform *child) {
                    child->_calculatedPreferedSize.y -= removing;
                });
            break;
        }
        }
    }
    ent->forEachChild([](const entity *child) {
        recursivelyUpdateLayoutsPreferedSizesByFill_(child);
    });
}

inline void uiTransform::recursivelyUpdateTransforms_(const entity *ent, glm::vec2 sizeUnit, const bool skipSelf)
{
    canvas *canvasInstance = ent->getComponent<canvas>();
    if (canvasInstance)
        sizeUnit = canvasInstance->_unitScale;
    if (uiTransform *instance = ent->getComponent<uiTransform>())
    {
        switch (instance->layout)
        {
        case none:
            break;
        case horizontal: {
            if (!skipSelf)
                instance->deltaSize = instance->_calculatedPreferedSize;
            const auto totalSize = instance->_calculatedPreferedSize;
            auto point = instance->layoutProperties.horizontal.startFromEnd ? totalSize : glm::vec2{0, 0};
            ent->forEachChild([totalSize, &point](const entity *child) {
                if (uiTransform *childTransform = child->getComponent<uiTransform>())
                {
                    childTransform->minAnchor = {point.x / totalSize.x,
                                                 point.y / totalSize.y};
                    childTransform->maxAnchor = childTransform->minAnchor + childTransform->_calculatedPreferedSize / totalSize;
                }
            });
            break;
        }
        case vertical: {
            if (!skipSelf)
                instance->deltaSize = instance->_calculatedPreferedSize;
            const auto totalSize = instance->_calculatedPreferedSize;
            auto point = instance->layoutProperties.vertical.startFromEnd ? totalSize : glm::vec2{0, 0};
            ent->forEachChild([totalSize, &point](const entity *child) {
                if (uiTransform *childTransform = child->getComponent<uiTransform>())
                {
                    childTransform->minAnchor = {point.x / totalSize.x,
                                                 point.y / totalSize.y};
                    childTransform->maxAnchor = childTransform->minAnchor + childTransform->_calculatedPreferedSize / totalSize;
                }
            });
            if (!skipSelf)
                instance->deltaSize = instance->_calculatedPreferedSize;
            break;
        }
        }
    }
    ent->forEachChild([](const entity *child) {
        recursivelyUpdateLayoutsPreferedSizesByFill_(child);
    });
}

#endif

} // namespace engine::ui
