#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <functional>
#include "refs.hpp"

namespace engine
{
    class entity;
    class application;

    class component
    {
        friend entity;
    public:
        virtual void update(const float deltaTime) {};

    private:
        enum State : uint8_t
        {
            Active = 0,
            Removing = 1 << 0
        };

        ref<entity> _entity;
        State _state = State::Active;

        bool awaitingRemoval_() const { return _state & State::Removing; }
    };

    class entity
    {
        friend application;
    public:
        std::string name = "New Entity";
        std::vector<ref<entity>> children{};
        ref<entity> parent{};

        entity() { _selfRef = this; }

        template<typename T, typename... Args, std::enable_if_t<std::constructible_from<T, Args...>, int> = 0>
        T& addComponent(Args&&... args)
        {
            T* component = new T(std::forward<Args>(args)...);
            component->_entity = _selfRef;
            _newComponents.push_back(component);
            return *component;
        }

    private:
        static inline std::vector<entity*> _entities;

        std::vector<refOwn<component>> _components{};
        std::vector<component*> _newComponents{};
        refOwn<entity> _selfRef;

        void update(const float deltaTime)
        {
            for (auto& component : _components)
                component.get()->update(deltaTime);
        }

        void addNewComponents()
        {
            for (auto* component : _newComponents)
                _components.push_back(component);
            _newComponents.clear();
        }

        void removeComponents()
        {
            auto r = std::remove_if(_components.begin(), _components.end(), [](refOwn<component>& comp)
                {
                    return comp.get()->awaitingRemoval_();
                });
            _components.erase(r, _components.end());
        }
    };

    class time
    {
        friend application;
    public:
        static inline int getTargetFps() noexcept { return _targetFps; }
        static inline void setTargetFps(const int targetFps) noexcept
        {
            _targetFps = targetFps;
            _targetDelay = std::chrono::duration < float>(1 / _targetFps);
        }
        static inline float getTotalTime() noexcept { return _totalTime; }
        static inline float getDeltaTime() noexcept { return _deltaTime; }
        static inline size_t getTotalFrames() noexcept { return _totalFrames; }
        static inline float getLastFrameSleepTime() noexcept { return _lastFrameSleep.count(); }

    private:
        static inline int _targetFps = 30;
        static inline std::chrono::duration<float> _targetDelay{ 1 / 30.f };
        static inline float _totalTime;
        static inline size_t _totalFrames;
        static inline float _deltaTime;
        static inline std::chrono::duration<float> _lastFrameSleep;
    };

    class application
    {
    public:
        static inline void run(std::vector<entity*>&& entities)
        {
            entity::_entities = std::move(entities);
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

                for (auto& func : _preComponentHooks)
                    std::invoke(func);

                for (auto* entities : entity::_entities)
                    entities->update(time::_deltaTime);
                for (auto* entities : entity::_entities)
                    entities->addNewComponents();
                for (auto* entities : entity::_entities)
                    entities->removeComponents();

                for (auto& func : _postLoopExecutes)
                    std::invoke(func);

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

        static inline void addPreComponentHook(std::function<void()>&& func)
        {
            _preComponentHooks.push_back(func);
        }

        static inline void removePreComponentHook(std::function<void()>&& func)
        {
            auto target = func.target<void(*)()>();
            _postLoopExecutes.push_back([&target]()
                {
                    for (size_t i = 0; i < _preComponentHooks.size(); i++)
                        if (_preComponentHooks[i].target<void(*)()>() == target)
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
            while (std::chrono::high_resolution_clock::now() < time) {}
        }
    };
}