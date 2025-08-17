#pragma once

#include "log.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace engine
{
class entity;
class application;

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

    // get shared_ptr to the entity owning this component. it's valid as long as the component exists in the entity hierarchy
    std::shared_ptr<entity> getEntity()
    {
        return _entity.lock();
    }

    // get weak_ptr to self. it's valid as long as the component exists in the entity hierarchy
    std::weak_ptr<component> getWeakPtr()
    {
        return _selfRef;
    }

  private:
    // gets called after creation as soon as all the base local variables are initialized and after the constructor
    virtual void created_() {};

    // gets called every tick
    virtual void update_() {};

    // gets called when the component is removed from the entity and base local variables are valid, and before the destructor is called
    virtual void removed_() {};

    enum State : uint8_t
    {
        Active = 0,
        Removing = 1 << 0
    };

    std::weak_ptr<entity> _entity;
    State _state = State::Active;
    std::weak_ptr<component> _selfRef;

    bool awaitingRemoval_() const
    {
        return _state & State::Removing;
    }
};

// an entity in the engine. it can have components and children entities
class entity
{
    friend application;

  public:
    std::string name;

    // creates a new entity with the given name
    static inline std::shared_ptr<entity> create(std::string &&name = "New Entity")
    {
        entity *newEntity = new entity(std::move(name));
        std::shared_ptr<entity> sharedPtr{newEntity};
        s_rootEntities.push_back(sharedPtr);
        newEntity->_selfRef = sharedPtr;
        s_newEntities.push_back(sharedPtr);
        return sharedPtr;
    }

    static inline size_t getRootEntitiesCount() noexcept
    {
        return s_rootEntities.size();
    }

    static inline std::vector<std::shared_ptr<entity>> getRootEntities() noexcept
    {
        return s_rootEntities;
    }

    static inline std::shared_ptr<entity> getRootEntityAt(const size_t index) noexcept
    {
        return s_rootEntities[index];
    }

    static inline size_t getEntitiesCount() noexcept
    {
        return s_entities.size() + s_newEntities.size();
    }

    static inline std::vector<std::shared_ptr<entity>> getEntities() noexcept
    {
        auto copy = s_entities;
        copy.insert(copy.end(), s_newEntities.begin(), s_newEntities.end());
        return copy;
    }

    static inline std::shared_ptr<entity> getEntityAt(const size_t index) noexcept
    {
        return index < s_entities.size() ? s_entities[index] : s_newEntities[index - s_entities.size()];
    }

    std::vector<std::shared_ptr<entity>> getChildren() const noexcept
    {
        return _children;
    }

    std::shared_ptr<entity> getParent() const noexcept
    {
        return _parent;
    }

    void remove() noexcept
    {
        _removing = true;
        for (auto &child : _children)
            child->remove();
    }

    void setParent(const std::shared_ptr<entity> &parent)
    {
        if (_parent && !parent)
        {
            auto ref = _selfRef.lock();
            s_rootEntities.push_back(ref);
            _parent->removeFromChildren_(ref);
        }
        else if (!_parent && parent)
        {
            auto ref = _selfRef.lock();
            // remove from root entities
            s_rootEntities.erase(std::remove(s_rootEntities.begin(), s_rootEntities.end(), ref), s_rootEntities.end());
            parent->_children.push_back(ref);
        }
        _parent = parent;
    }

    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...> && std::is_base_of_v<component, T>
    std::shared_ptr<T> addComponent(Args &&...args)
    {
        std::shared_ptr<component> component{new T(std::forward<Args>(args)...)};
        component->_entity = _selfRef;
        component->_selfRef = component;
        component->created_();
        _newComponents.push_back(component);
        return std::dynamic_pointer_cast<T>(component);
    }

    template <typename T>
        requires std::is_base_of_v<component, T>
    std::shared_ptr<T> getComponent() const
    {
        for (size_t i = 0; i < _components.size(); i++)
            if (dynamic_cast<T *>(_components[i].get()))
                return std::dynamic_pointer_cast<T>(_components[i]);
        return nullptr;
    }

    template <typename T>
        requires std::is_base_of_v<component, T>
    void getComponents(std::vector<std::shared_ptr<T>> &vector)
    {
        for (size_t i = 0; i < _components.size(); i++)
            if (dynamic_cast<T *>(_components[i].get()))
                vector.push_back(std::dynamic_pointer_cast<T>(_components[i]));
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
        auto ref = _selfRef.lock();
        return std::find(_parent->_children.begin(), _parent->_children.end(), ref) - _parent->_children.begin();
    }

    void setSiblingIndex(size_t index) const
    {
        if (!_parent)
        {
            engine::log::logError("trying to set the siblind of entity \"{}\" which does not have parent", name);
            return;
        }
        auto ref = _selfRef.lock();
        auto selfIt = std::find(_parent->_children.begin(), _parent->_children.end(), ref);
        std::rotate(selfIt, selfIt + 1, _parent->_children.end());
    }

  private:
    static inline auto &s_entities = *new std::vector<std::shared_ptr<entity>>();
    static inline auto &s_newEntities = *new std::vector<std::shared_ptr<entity>>();
    static inline auto &s_rootEntities = *new std::vector<std::shared_ptr<entity>>();

    std::vector<std::shared_ptr<component>> _components{};
    std::vector<std::shared_ptr<component>> _newComponents{};
    std::vector<std::shared_ptr<entity>> _children{};
    std::weak_ptr<entity> _selfRef;
    std::shared_ptr<entity> _parent{};
    bool _removing = false;
    bool _active = true;
    bool _hierarchyActive = true;

    entity(std::string &&name)
        : name(std::move(name))
    {
    }

    void update_()
    {
        for (auto &component : _components)
            component->update_();
    }

    void addNewComponents_()
    {
        for (auto &component : _newComponents)
            _components.push_back(std::move(component));
        _newComponents.clear();
    }

    void removeComponents_()
    {
        auto r = std::remove_if(
            _components.begin(), _components.end(),
            [](std::shared_ptr<component> &comp) {
                return comp->awaitingRemoval_();
            });
        std::for_each(
            r, _components.end(),
            [](const std::shared_ptr<component> &comp) {
                comp->removed_();
            });
        _components.erase(r, _components.end());
    }

    void removed_()
    {
        auto ref = _selfRef.lock();
        if (_parent)
            _parent->removeFromChildren_(ref);
        // remove from roots
        s_rootEntities.erase(std::remove(s_rootEntities.begin(), s_rootEntities.end(), ref), s_rootEntities.end());
        // remove all components
        for (auto &comp : _components)
            comp->removed_();
    }

    void setHierarchyActive_(const bool active)
    {
        _hierarchyActive = active;
        for (auto &child : _children)
            child->setHierarchyActive_(active);
    }

    void removeFromChildren_(std::shared_ptr<entity> entity)
    {
        _children.erase(std::remove(_children.begin(), _children.end(), entity), _children.end());
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
    static inline int _targetFps = 30;
    static inline std::chrono::duration<float> _targetDelay{1 / 30.f};
    static inline float _totalTime;
    static inline size_t _totalFrames;
    static inline float _deltaTime;
    static inline float _lastFrameSleep;
};

class application
{
  public:
    application() = delete;

    // hooks to execute before component updates in the loop
    static inline auto &preComponentHooks = *new std::vector<std::function<void()>>();

    // hooks to execute after component updates in the loop
    static inline auto &postComponentHooks = *new std::vector<std::function<void()>>();

    // hooks to execute after the application is closed
    static inline auto &onExitHooks = *new std::vector<std::function<void()>>();

    // for function pointer lists in this class
    static inline std::mutex hooksMutex;

    static inline void run()
    {
        _isRunning = true;

        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto currentFrameTime = std::chrono::high_resolution_clock::now();

        while (_isRunning)
        {
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

            for (auto &func : preComponentHooksCopy)
                func();

            for (auto &entities : entity::s_entities)
                entities->update_();
            for (auto &entities : entity::s_entities)
                entities->removeComponents_();
            for (auto &entities : entity::s_entities)
                entities->addNewComponents_();

            { // remove entities
                entity::s_entities.erase(
                    std::remove_if(
                        entity::s_entities.begin(), entity::s_entities.end(),
                        [](std::shared_ptr<entity> &testEntity) {
                            if (testEntity->_removing)
                            {
                                testEntity->removed_();
                                return true;
                            }
                            return false;
                        }),
                    entity::s_entities.end());
            }

            // add entities
            for (auto &entity : entity::s_newEntities)
                entity::s_entities.push_back(std::move(entity));
            entity::s_newEntities.clear();

            for (auto &func : postComponentHooksCopy)
                func();
            for (auto &func : _postLoopExecutes)
                func();

            // frame-rate consistency
            auto now = std::chrono::high_resolution_clock::now();
            auto diff = now - currentFrameTime;
            auto diffFromTarget = time::_targetDelay - diff;
            time::_lastFrameSleep = std::chrono::duration<float>(diffFromTarget).count();
            if (diffFromTarget.count() > 0)
                sleepUntilExact_(now + diffFromTarget);
        }
        hooksMutex.lock();
        auto onExitHooksCopy = onExitHooks;
        hooksMutex.unlock();
        for (auto &func : onExitHooksCopy)
            func();
    }

    // thread-safe. exists the application after the current frame is finished
    static inline void close()
    {
        _isRunning = false;
    }

  private:
    static inline bool _isRunning;

    // commands to execute after each loop. resets on every iteration. it's entirely internal to this class
    static inline auto &_postLoopExecutes = *new std::vector<std::function<void()>>();

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