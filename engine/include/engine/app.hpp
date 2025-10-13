#pragma once

#include "benchmark.hpp"
#include "common/typeInfo.hpp"
#include "engine/ref.hpp"
#include "ittnotify.h"
#include "log.hpp"
#include "quickVector.hpp"
#include "ref.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <tracy/Tracy.hpp>
#include <type_traits>

namespace engine
{
struct entity;
struct application;

constexpr size_t notFound = (size_t)-1;

// a component belonging to an entity. it can be used to add functionality to the entity
struct component
{
    createBaseTypeInformation(component);
    friend entity;

    // marks this component for removal. it will be removed after the current frame is finished (before the application::postComponentHooks)
    void remove() noexcept;

    // get weak reference to the entity owning this component. it's valid as long as the component exists in the entity hierarchy
    weakRef<entity> &getEntity()
    {
        return _entity;
    }

    // get weak reference to self. it's valid as long as the component exists in the entity hierarchy
    weakRef<component> &getWeakRef()
    {
        return _selfRef;
    }
    template <typename T>
        requires std::is_base_of_v<component, T>
    weakRef<T> getWeakRefAs() const
    {
        return *_selfRef.as<T>();
    }

    void setEnabled(const bool enabled) noexcept
    {
        _state |= enabled ? state::enabling : state::disabling;
    }

    bool getEnabled() const noexcept
    {
        return _state & state::enabled;
    }

    // increases this component's lock (makes sure this component will not be removed).
    // always call popLock() after this component is no longer needed by the consumer
    void pushLock() noexcept
    {
        _removeLock++;
    }

    // decreases this component's lock (if reaches zero, this component will be removable)
    void popLock() noexcept
    {
        _removeLock--;
    }

  protected:
    // gets called after creation as soon as all the base local variables are initialized and after the constructor.
    // returns whether this component should stay (if false, it'll be removed immediately without calling any other APIs in this component)
    virtual bool created_()
    {
        return true;
    }

    // gets called every tick
    virtual void update_() {};

    // gets called when the component is removed from the entity and base local variables are valid, and before the destructor is called
    virtual void removed_() {};

    // gets called when the component is enabled. it's always called after created_
    virtual void enabled_() {};

    // gets called when the component is disabled. it's always called before removed_
    virtual void disabled_() {};

  private:
    enum state : uint8_t
    {
        enabled = 1 << 0,
        removing = 1 << 1,
        enabling = 1 << 2,
        disabling = 1 << 3,
        disabling_or_enabling = enabling | disabling
    };

    friend inline state operator|(const state a, const state b) noexcept
    {
        using underlying = std::underlying_type_t<state>;
        return static_cast<state>(static_cast<underlying>(a) | static_cast<underlying>(b));
    }

    friend inline state operator&(const state a, const state b) noexcept
    {
        using underlying = std::underlying_type_t<state>;
        return static_cast<state>(static_cast<underlying>(a) & static_cast<underlying>(b));
    }

    friend inline state operator~(const state a) noexcept
    {
        using underlying = std::underlying_type_t<state>;
        return static_cast<state>(~static_cast<underlying>(a));
    }

    friend inline state &operator|=(state &a, const state b) noexcept
    {
        using underlying = std::underlying_type_t<state>;
        a = static_cast<state>(static_cast<underlying>(a) | static_cast<underlying>(b));
        return a;
    }

    friend inline state &operator&=(state &a, const state b) noexcept
    {
        using underlying = std::underlying_type_t<state>;
        a = static_cast<state>(static_cast<underlying>(a) & static_cast<underlying>(b));
        return a;
    }

    weakRef<entity> _entity;
    state _state = state::enabled;
    weakRef<component> _selfRef{};

    // lock for removing. if value is beyond 0, cannot remove this component
    unsigned char _removeLock;
};

// an entity in the engine. it can have components and children entities
struct entity
{
    friend application;
    std::string name;

    // creates a new entity with the given name
    static inline weakRef<entity> create(std::string &&name = "New Entity")
    {
        s_newEntities.emplace_back(true);
        ownRef<entity> &newEntity = s_newEntities.back();
        s_rootEntities.emplace_back(newEntity);
        newEntity->name = std::move(name);
        newEntity->_selfRef = newEntity;
        return newEntity;
    }

    template <typename Func>
    static inline void forEachRootEntity(Func &&func)
    {
        s_rootEntities.forEach(std::forward<Func>(func));
    }

    static inline size_t getRootEntitiesCount() noexcept
    {
        return s_rootEntities.size();
    }

    // creates a copy of the root entities vector and returns it
    static inline quickVector<weakRef<entity>> getRootEntities() noexcept
    {
        return s_rootEntities;
    }

    static inline weakRef<entity> &getRootEntityAt(const size_t index) noexcept
    {
        return s_rootEntities[index];
    }

    static inline size_t getEntitiesCount() noexcept
    {
        return s_entities.size() + s_newEntities.size();
    }

    // creates a copy of the entities reference vector
    static inline quickVector<weakRef<entity>> getEntities() noexcept
    {
        return s_entities;
    }

    static inline weakRef<entity> getEntityAt(const size_t index) noexcept
    {
        return index < s_entities.size() ? s_entities[index] : s_newEntities[index - s_entities.size()];
    }

    // creates a copy of the children vector
    quickVector<weakRef<entity>> getChildren() const noexcept
    {
        return _children;
    }

    // executes function on all children (cannot modify the children during this)
    template <typename Func>
    void forEachChild(Func &&func)
    {
        _children.forEach(std::forward<Func>(func));
    }

    // executes function on all children (cannot modify the children during this)
    template <typename Func>
    void forEachChild(Func &&func) const
    {
        _children.forEach(func);
    }

    size_t getChildrenCount() const noexcept
    {
        return _children.size();
    }

    weakRef<entity> getChildAt(const size_t index) noexcept
    {
        return _children[index];
    }

    weakRef<entity> getParent() const noexcept
    {
        return _parent;
    }

    void remove() noexcept
    {
        _removing = true;
        _children.forEach([](const weakRef<entity> &child) {
            child->remove();
        });
    }

    void setParent(const weakRef<entity> &parent)
    {
        if (parent == _selfRef)
        {
            log::logError("cannot set entity as parent of itself: \"{}\"", name);
            return;
        }
        if (_parent && !parent)
        {
            s_rootEntities.emplace_back(_selfRef);
            _parent->removeFromChildren_(_selfRef);
        }
        else if (!_parent && parent)
        {
            // remove from root entities
            s_rootEntities.erase(_selfRef);
            parent->_children.emplace_back(_selfRef);
        }
        _parent = parent;
    }

    // gets or adds the requested component. may return nullptr if can't add the component
    template <typename T, typename... Args>
        requires std::is_base_of_v<component, T> && std::is_constructible_v<T, Args...>
    weakRef<T> ensureComponentExists(Args &&...args)
    {
        assertComponentTypeValidity_<T>();
        if (auto r = getComponent<T>())
            return r;
        return addComponent<T>(args...);
    }

    // tries to add a new component. returns nullptr if failed
    template <typename T, typename... Args>
        requires std::is_base_of_v<component, T> && std::is_constructible_v<T, Args...>
    weakRef<T> addComponent(Args &&...args)
    {
        assertComponentTypeValidity_<T>();
        ownRef<T> newComponentAsT{true, std::forward<Args>(args)...};

        ownRef<component> newComponent = newComponentAsT;
        newComponent->_entity = _selfRef;
        newComponent->_selfRef = newComponent;
        _newComponents.emplace_back(newComponent);
        if (!newComponent->created_())
        {
            _newComponents.pop_back();
            log::logError("could not add component \"{}\" to entity \"{}\"", typeid(T).name(), name);
            return {};
        }
        newComponent->enabled_();
        return newComponentAsT;
    }

    // returns found component or nullptr
    template <typename T>
        requires std::is_base_of_v<component, T>
    weakRef<T> getComponent() const
    {
        for (size_t i = 0; i < _components.size(); i++)
            if (isOfType<T>((component &)(_components[i])))
                return _components[i]->getWeakRefAs<T>();
        for (size_t i = 0; i < _newComponents.size(); i++)
            if (isOfType<T>((component &)(_newComponents[i])))
                return _newComponents[i]->getWeakRefAs<T>();
        return {};
    }

    template <typename T>
        requires std::is_base_of_v<component, T>
    void getComponents(quickVector<weakRef<T>> &result)
    {
        for (size_t i = 0; i < _components.size(); i++)
            if (isOfType<T>((component &)(_components[i])))
                result.push_back(_components[i]->getWeakRefAs<T>());
        for (size_t i = 0; i < _newComponents.size(); i++)
            if (isOfType<T>((component &)(_newComponents[i])))
                result.push_back(_newComponents[i]->getWeakRefAs<T>());
    }

    // returns found component in self or the first one found in parents, or nullptr
    template <typename T>
        requires std::is_base_of_v<component, T>
    weakRef<T> getComponentInParent() const
    {
        const entity *current = this;
        do
        {
            weakRef<T> result = current->getComponent<T>();
            if (result)
                return result;
            current = current->_parent;
        } while (current);
        return {};
    }

    // returns found components in self and in parents
    template <typename T>
        requires std::is_base_of_v<component, T>
    void getComponentsInParent(quickVector<weakRef<T>> &result)
    {
        const entity *current = this;
        do
        {
            getComponents(result);
            current = current->_parent;
        } while (current);
    }

    bool isSelfActive() const noexcept
    {
        return _active;
    }

    bool isHierarchyActive() const noexcept
    {
        return !_removing && _hierarchyActive;
    }

    void setActive(const bool active)
    {
        _active = active;
        setHierarchyActive_(active);
    }

    size_t getSiblindIndex() const
    {
        if (!_parent)
            return notFound;
        return _parent->_children.findIndex(_selfRef);
    }

    void setSiblingIndex(size_t index) const
    {
        if (!_parent)
        {
            engine::log::logError("trying to set the siblind of entity \"{}\" which does not have parent", name);
            return;
        }
        auto *self = _parent->_children.find(_selfRef);
        std::rotate(self, self + 1, _parent->_children.data() + _parent->_children.size());
    }

  private:
    static inline auto &s_entities = *new quickVector<ownRef<entity>>();
    static inline auto &s_newEntities = *new quickVector<ownRef<entity>>();
    static inline auto &s_rootEntities = *new quickVector<weakRef<entity>>();

    quickVector<ownRef<component>> _components{};
    quickVector<ownRef<component>> _newComponents{};
    quickVector<weakRef<entity>> _children{};
    weakRef<entity> _selfRef{};
    weakRef<entity> _parent{};
    bool _removing = false;
    bool _active = true;
    bool _hierarchyActive = true;

    // static_asserts for validity checks on component types
    template <typename T>
    static inline void assertComponentTypeValidity_()
    {
        static_assert(std::is_base_of_v<component, T>, "you can only add components that are derived from engine::component");
        static_assert(T::c_hasBase, "you should use createTypeInformation(T, component) where T is your component type, for components.");
    }

    void update_()
    {
        _components.forEach([](const ownRef<component> &comp) {
            comp->update_();
        });
    }

    void addNewComponents_()
    {
        _components.reserve(_components.size() + _newComponents.size());
        _newComponents.forEach([&](const ownRef<component> &comp) {
            _components.emplace_back(std::move(comp));
        });
        _newComponents.clear();
    }

    void updateComponentStates_()
    {
        _components.eraseIf([](const ownRef<component> &comp) {
            auto state = comp->_state;
            if (comp->_state & component::state::removing)
            {
                comp->disabled_();
                comp->removed_();
                return true;
            }
            if (comp->_state & component::state::enabling)
            {
                comp->_state &= ~component::state::disabling_or_enabling;
                comp->_state |= component::state::enabled;
                comp->enabled_();
            }
            else if (comp->_state & component::state::disabling)
            {
                comp->_state &= ~component::state::disabling_or_enabling;
                comp->_state &= ~component::state::enabled;
                comp->disabled_();
            }
            return false;
        });
    }

    void removed_()
    {
        if (_parent)
            _parent->removeFromChildren_(_selfRef);
        s_rootEntities.erase(_selfRef);
        _components.forEachAndClear([](const ownRef<component> &comp) {
            comp->removed_();
        });
    }

    void setHierarchyActive_(const bool active)
    {
        _hierarchyActive = active;
        _children.forEach([active](const weakRef<entity> &child) {
            child->setHierarchyActive_(active);
        });
    }

    void removeFromChildren_(const weakRef<entity> &entity)
    {
        _children.erase(entity);
    }
};

class time
{
    friend application;

  public:
    time() = delete;
    static inline int getTargetFps() noexcept
    {
        return _targetFps;
    }

    static inline void setTargetFps(const int targetFps) noexcept
    {
        _targetFps = targetFps;
        new (&_targetDelay) std::chrono::duration<float>(1.f / _targetFps);
    }

    static inline float getTotalTime() noexcept
    {
        return _totalTime;
    }

    static inline float getDeltaTime() noexcept
    {
        return _deltaTime;
    }

    static inline size_t getTotalFrames() noexcept
    {
        return _totalFrames;
    }

    static inline float getLastFrameSleepTime() noexcept
    {
        return _lastFrameSleep;
    }

  private:
    static inline int _targetFps = 120;
    static inline std::chrono::duration<float> _targetDelay{1 / 120.f};
    static inline float _totalTime;
    static inline size_t _totalFrames;
    static inline float _deltaTime;
    static inline float _lastFrameSleep;
};

struct application
{
    application() = delete;

    // hooks to execute before component updates in the loop
    static inline auto &preComponentHooks = *new quickVector<std::function<void()>>();

    // hooks to execute after component updates in the loop
    static inline auto &postComponentHooks = *new quickVector<std::function<void()>>();

    // hooks to execute after the application is closed
    static inline auto &onExitHooks = *new quickVector<std::function<void()>>();

// executes a copy of the hooks' vector
#define executeHooks(hooks)                             \
    {                                                   \
        bench(#hooks);                                  \
        const auto copy = hooks;                        \
        copy.forEach([](const auto &func) { func(); }); \
    }

    static inline void run()
    {
        TracyPlotConfig(s_tracyEntityCountName, tracy::PlotFormatType::Number, false, false, tracy::Color::AliceBlue);
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto currentFrameTime = std::chrono::high_resolution_clock::now();

        while (_isRunning)
        {
            __itt_frame_begin_v3(benchmark::s_vtuneDomain, 0);
            currentFrameTime = std::chrono::high_resolution_clock::now();
            time::_deltaTime = (currentFrameTime - lastFrameTime).count() / 1e9f; // convert to seconds
            time::_totalTime += time::_deltaTime;
            time::_totalFrames++;
            lastFrameTime = currentFrameTime;

            executeHooks(preComponentHooks);
            {
                bench("handling entities");
                {
                    bench("updating entities");
                    entity::s_entities.forEach([](const weakRef<entity> &entity) {
                        entity->update_();
                    });
                }
                {
                    bench("updating component states");
                    entity::s_entities.forEach([](const weakRef<entity> &entity) {
                        entity->updateComponentStates_();
                        entity->addNewComponents_();
                    });
                }

                {
                    bench("removing entities");
                    entity::s_entities.eraseIfUnordered([](const weakRef<entity> &entity) {
                        if (entity->_removing)
                        {
                            entity->removed_();
                            return true;
                        }
                        return false;
                    });
                }

                {
                    bench("adding new entities");
                    entity::s_entities.reserve(entity::s_entities.size() + entity::s_newEntities.size());
                    entity::s_newEntities.forEachAndClear([](const ownRef<entity> &entity) {
                        entity::s_entities.emplace_back(std::move(entity));
                    });
                }
            }
            TracyPlot(s_tracyEntityCountName, static_cast<long long>(entity::getEntitiesCount()));

            executeHooks(postComponentHooks);
            executeHooks(_postLoopExecutes);
            {
                bench("sleep");
                // frame-rate consistency
                auto now = std::chrono::high_resolution_clock::now();
                auto diff = now - currentFrameTime;
                auto diffFromTarget = time::_targetDelay - diff;
                time::_lastFrameSleep = std::chrono::duration<float>(diffFromTarget).count();
                if (diffFromTarget.count() > 0)
                    sleepUntilExact_(now + diffFromTarget);
            }
            __itt_frame_end_v3(benchmark::s_vtuneDomain, 0);
            FrameMark;
        }

        executeHooks(onExitHooks);
    }
#undef executeHooks

    // thread-safe. exists the application after the current frame is finished
    static inline void close()
    {
        _isRunning = false;
    }

  private:
    static inline bool _isRunning = true;

    // commands to execute after each loop. resets on every iteration. it's entirely internal to this class
    static inline auto &_postLoopExecutes = *new quickVector<std::function<void()>>();
    static inline const char *s_tracyEntityCountName = "entity count";

    static inline void sleepUntilExact_(const auto time)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<float>(now - time);
        auto toSleep = diff - std::chrono::duration<float>(0.001f); // leave 2 milliseconds
        std::this_thread::sleep_for(toSleep);
        while (std::chrono::high_resolution_clock::now() < time)
        {
            // spin-lock for accuracy
        }
    }
};

#ifndef _GAME_ENGINE_APP_H
#define _GAME_ENGINE_APP_H
inline void component::remove() noexcept
{
    if (_removeLock > 0)
    {
        log::logError("Cannot remove component \"{}\" from entity \"{}\", because other components depend on this.", getTypeName(), _entity->name);
        return;
    }
    _state |= state::removing;
}

#endif
} // namespace engine