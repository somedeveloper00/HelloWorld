#pragma once

#include "benchmark.hpp"
#include "common/typeHash.hpp"
#include "ittnotify.h"
#include "log.hpp"
#include "quickVector.hpp"
#include "ref.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
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
    friend entity;

    // marks this component for removal. it will be removed after the current frame is finished (before the application::postComponentHooks)
    void remove() noexcept
    {
        _state = static_cast<State>(_state | State::Removing);
    }

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
    weakRef<T> getWeakRefAs()
    {
        return *_selfRef.castTo<T>();
    }

  protected:
    // gets called after creation as soon as all the base local variables are initialized and after the constructor
    virtual void created_() {};

    // gets called every tick
    virtual void update_() {};

    // gets called when the component is removed from the entity and base local variables are valid, and before the destructor is called
    virtual void removed_() {};

  private:
    enum State : uint8_t
    {
        Active = 0,
        Removing = 1 << 0
    };

    weakRef<entity> _entity;
    State _state = State::Active;
    weakRef<component> _selfRef{};
    size_t _typeHash;

    bool awaitingRemoval_() const
    {
        return _state & State::Removing;
    }
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

    static inline size_t getRootEntitiesCount() noexcept
    {
        return s_rootEntities.size();
    }

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

    // gets or adds the requested component
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...> && std::is_base_of_v<component, T>
    weakRef<T> ensureComponentExists(Args &&...args)
    {
        if (auto r = getComponent<T>(args...))
            return r;
        return addComponent<T>(args...);
    }

    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...> && std::is_base_of_v<component, T>
    weakRef<T> addComponent(Args &&...args)
    {
        ownRef<T> component{true, std::forward<Args>(args)...};
        component->_entity = _selfRef;
        component->_selfRef = component;
        component->_typeHash = getTypeHash_<T>();
        _newComponents.emplace_back(component);
        _newComponents.back()->created_();
        return component;
    }

    // returns found component or nullptr
    template <typename T>
        requires std::is_base_of_v<component, T>
    weakRef<T> getComponent() const
    {
        bench("getComponent");
        constexpr auto hash = getTypeHash_<T>();
        ownRef<component> *result = _components.findIf([](const ownRef<component> &c) {
            return c->_typeHash == hash;
        });
        if (result)
            return *result->castTo<T>();
        result = _newComponents.findIf([](const ownRef<component> &c) {
            return c->_typeHash == hash;
        });
        if (result)
            return *result->castTo<T>();
        return {};
    }

    template <typename T>
        requires std::is_base_of_v<component, T>
    void getComponents(quickVector<weakRef<T>> &result)
    {
        bench("getComponents");
        constexpr size_t hash = getTypeHash_<T>();
        static auto callback = [&](const ownRef<component> &comp) {
            if (comp->_typeHash == hash)
            {
                ownRef<T> converted = comp.castTo<T>();
                result.emplace_back(converted);
            }
        };
        _components.forEach(callback);
        _newComponents.forEach(callback);
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

    void removeComponents_()
    {
        _components.eraseIf([](const ownRef<component> &comp) {
            return false;
            if (comp->awaitingRemoval_())
            {
                comp->removed_();
                return true;
            }
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

    // for function pointer lists in this class
    static inline std::mutex hooksMutex;

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

            hooksMutex.lock();
            auto preComponentHooksCopy = preComponentHooks;
            auto postComponentHooksCopy = postComponentHooks;
            auto onExitHooksCopy = onExitHooks;
            auto postLoopExecutesCopy = _postLoopExecutes;
            _postLoopExecutes.clear();
            hooksMutex.unlock();

            preComponentHooksCopy.forEach([](const auto &func) { func(); });

            {
                bench("handling entities");
                {
                    bench("updating entities");
                    entity::s_entities.forEachParallel([](const weakRef<entity> &entity) {
                        entity->update_();
                    });
                }
                {
                    bench("adding/removing components");
                    entity::s_entities.forEachParallel([](const weakRef<entity> &entity) {
                        entity->removeComponents_();
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

            {
                bench("postComponentHooks");
                postComponentHooksCopy.forEach([](const auto &func) { func(); });
            }
            {
                bench("_postLoopExecutes");
                postLoopExecutesCopy.forEach([](const auto &func) { func(); });
            }
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

        {
            bench("onExitHooks");
            hooksMutex.lock();
            auto onExitHooksCopy = onExitHooks;
            hooksMutex.unlock();
            onExitHooksCopy.forEach([](const auto &func) { func(); });
        }
    }

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
} // namespace engine