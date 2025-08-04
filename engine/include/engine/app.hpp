#pragma once
#include "log.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace engine
{
class entity;
class application;

constexpr size_t notFound = (size_t)-1;

class component
{
    friend entity;

  public:
    virtual void update(const float deltaTime) {};

    void remove() noexcept
    {
        _state = static_cast<State>(_state | State::Removing);
    }

  private:
    enum State : uint8_t
    {
        Active = 0,
        Removing = 1 << 0
    };

    std::weak_ptr<entity> _entity;
    State _state = State::Active;

    bool awaitingRemoval_() const
    {
        return _state & State::Removing;
    }
};

class entity
{
    friend application;

  public:
    std::string name;

    static inline std::shared_ptr<entity> create(std::string &&name = "New Entity")
    {
        entity *newEntity = new entity(std::move(name));
        std::shared_ptr<entity> sharedPtr{newEntity};
        _rootEntities.push_back(sharedPtr);
        newEntity->_selfRef = sharedPtr;
        _newEntities.push_back(sharedPtr);
        return sharedPtr;
    }

    static inline size_t getRootEntitiesCount() noexcept
    {
        return _rootEntities.size();
    }

    static inline std::vector<std::shared_ptr<entity>> getRootEntities() noexcept
    {
        return _rootEntities;
    }

    static inline std::shared_ptr<entity> getRootEntityAt(const size_t index) noexcept
    {
        return _rootEntities[index];
    }

    static inline size_t getEntitiesCount() noexcept
    {
        return _entities.size() + _newEntities.size();
    }

    static inline std::vector<std::shared_ptr<entity>> getEntities() noexcept
    {
        auto copy = _entities;
        copy.insert(copy.end(), _newEntities.begin(), _newEntities.end());
        return copy;
    }

    static inline std::shared_ptr<entity> getEntityAt(const size_t index) noexcept
    {
        return index < _entities.size() ? _entities[index] : _newEntities[index - _entities.size()];
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
    }

    void setParent(const std::shared_ptr<entity> parent)
    {
        if (_parent.get() && !parent.get())
        {
            _rootEntities.push_back(_selfRef.lock());
            _parent->removeFromChildren_(this);
        }
        else if (!_parent.get() && parent.get())
        {
            for (size_t i = 0; i < _rootEntities.size(); i++)
                if (_rootEntities[i].get() == this)
                {
                    _rootEntities.erase(_rootEntities.begin() + i);
                    break;
                }
            parent->_children.push_back(_selfRef.lock());
        }
        _parent = parent;
    }

    template <typename T, typename... Args,
              std::enable_if_t<std::constructible_from<T, Args...> && std::is_base_of_v<component, T>, int> = 0>
    std::shared_ptr<T> addComponent(Args &&...args)
    {
        std::shared_ptr<T> component{new T(std::forward<Args>(args)...)};
        component->_entity = _selfRef;
        _newComponents.push_back(component);
        return component;
    }

    template <typename T, std::enable_if_t<std::is_base_of_v<component, T>, int> = 0>
    std::shared_ptr<T> getComponent() const
    {
        for (size_t i = 0; i < _components.size(); i++)
            if (dynamic_cast<T *>(_components[i].get()))
                return _components[i];
        return std::shared_ptr<T>();
    }

    template <typename T, std::enable_if_t<std::is_base_of_v<component, T>, int> = 0>
    void getComponents(std::vector<std::shared_ptr<T>> &vector)
    {
        for (size_t i = 0; i < _components.size(); i++)
            if (dynamic_cast<T *>(_components[i].get()))
                vector.push_back(_components[i]);
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
        if (!_parent.get())
            return notFound;
        for (size_t i = 0; i < _parent->_children.size(); i++)
            if (_parent->_children[i].get() == this)
                return i;
        return notFound;
    }

    void setSiblingIndex(size_t index) const
    {
        if (!_parent.get())
        {
            logWarning("trying to set the siblind of entity \"{}\" which does not have parent", name);
            return;
        }
        for (size_t i = 0; i < _parent->_children.size(); i++)
            if (_parent->_children[i].get() == this)
            {
                if (i == index)
                    break;
                std::shared_ptr<entity> item = std::move(_parent->_children[i]);
                _parent->_children.erase(_parent->_children.begin() + i);
                if (index > i)
                    index--;
                _parent->_children.insert(_parent->_children.begin() + index, std::move(item));
                break;
            }
    }

  private:
    static inline std::vector<std::shared_ptr<entity>> _entities;
    static inline std::vector<std::shared_ptr<entity>> _newEntities;
    static inline std::vector<std::shared_ptr<entity>> _rootEntities;

    std::vector<std::shared_ptr<component>> _components{};
    std::vector<std::shared_ptr<component>> _newComponents{};
    std::vector<std::shared_ptr<entity>> _children{};
    std::weak_ptr<entity> _selfRef;
    std::shared_ptr<entity> _parent{};
    bool _removing = false;
    bool _active = true;
    bool _hierarchyActive = true;

    entity(std::string &&name) : name(std::move(name))
    {
    }

    void update(const float deltaTime)
    {
        for (auto &component : _components)
            component->update(deltaTime);
    }

    void addNewComponents()
    {
        for (auto &component : _newComponents)
            _components.push_back(std::move(component));
        _newComponents.clear();
    }

    void removeComponents()
    {
        auto r = std::remove_if(_components.begin(), _components.end(),
                                [](std::shared_ptr<component> &comp) { return comp->awaitingRemoval_(); });
        _components.erase(r, _components.end());
    }

    void setHierarchyActive_(const bool active)
    {
        _hierarchyActive = active;
        for (auto &child : _children)
            child->setHierarchyActive_(active);
    }

    void removeFromChildren_(entity *entity)
    {
        for (size_t i = 0; i < _children.size(); i++)
            if (_children[i].get() == entity)
            {
                _children.erase(_children.begin() + i);
                return;
            }
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
        _targetDelay = std::chrono::duration<float>(1 / _targetFps);
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
        return _lastFrameSleep.count();
    }

  private:
    static inline int _targetFps = 30;
    static inline std::chrono::duration<float> _targetDelay{1 / 30.f};
    static inline float _totalTime;
    static inline size_t _totalFrames;
    static inline float _deltaTime;
    static inline std::chrono::duration<float> _lastFrameSleep;
};

class application
{
  public:
    application() = delete;

    static inline void run()
    {
        _isRunning = true;

        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto currentFrameTime = std::chrono::high_resolution_clock::now();

        while (_isRunning)
        {
            currentFrameTime = std::chrono::high_resolution_clock::now();
            time::_deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
            time::_totalTime += time::_deltaTime;
            time::_totalFrames++;
            lastFrameTime = currentFrameTime;

            for (auto &func : _preComponentHooks)
                std::invoke(func);

            for (auto &entities : entity::_entities)
                entities->update(time::_deltaTime);
            for (auto &entities : entity::_entities)
                entities->removeComponents();
            for (auto &entities : entity::_entities)
                entities->addNewComponents();

            { // remove entities
                auto newEnd = std::remove_if(entity::_entities.begin(), entity::_entities.end(),
                                             [](std::shared_ptr<entity> &entity) { return entity->_removing; });
                std::for_each(newEnd, entity::_entities.end(),
                              [](std::shared_ptr<entity> &ptr) { ptr->_parent->removeFromChildren_(ptr.get()); });
                entity::_entities.erase(newEnd, entity::_entities.end());
            }

            // add entities
            for (auto &entity : entity::_newEntities)
                entity::_entities.push_back(std::move(entity));
            entity::_newEntities.clear();

            for (auto &func : _postLoopExecutes)
                std::invoke(func);

            // frame-rate consistency
            auto now = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration<float>(now - currentFrameTime);
            time::_lastFrameSleep = time::_targetDelay - diff;
            if (time::_lastFrameSleep.count() > 0)
                sleepUntil_(now + time::_lastFrameSleep);
        }
    }

    static inline void close()
    {
        _isRunning = false;
    }

    static inline void addPreComponentHook(std::function<void()> &&func)
    {
        _preComponentHooks.push_back(func);
    }

    static inline void removePreComponentHook(std::function<void()> &&func)
    {
        auto target = func.target<void (*)()>();
        _postLoopExecutes.push_back([&target]() {
            for (size_t i = 0; i < _preComponentHooks.size(); i++)
                if (_preComponentHooks[i].target<void (*)()>() == target)
                {
                    _preComponentHooks.erase(_preComponentHooks.begin() + i);
                    return;
                }
        });
    }

  private:
    static inline bool _isRunning;

    // hooks to execute before component updates in the loop
    static inline std::vector<std::function<void()>> _preComponentHooks;

    // commands to execute after each loop. resets on every iteration
    static inline std::vector<std::function<void()>> _postLoopExecutes;

    static inline void sleepUntil_(const auto time)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<float>(now - time);
        auto toSleep = diff - std::chrono::duration<float>(0.001f); // leave 2 milliseconds
        std::this_thread::sleep_for(toSleep);
        // spin-lock for accuracy
        while (std::chrono::high_resolution_clock::now() < time)
        {
        }
    }
};
} // namespace engine