#include "core/app.hpp"
#include "core/input.hpp"
#include "core/log.hpp"
#include <iostream>
#include <algorithm>

class printFps : public engine::component
{
public:
    void update(const float deltaTime) override
    {
        auto a = (1.f / engine::time::getTargetFps() * 100.f);
        auto b = (engine::time::getLastFrameSleepTime() * 100);
        engine::log("frame: {} fps: {} dt: {} slept: {} targetFps: {} frame-use: %{}", engine::time::getTotalFrames(), 1 / deltaTime, deltaTime, engine::time::getLastFrameSleepTime(), engine::time::getTargetFps(), 100 - ((engine::time::getLastFrameSleepTime()) / (1.f / engine::time::getTargetFps()) * 100));
    }
};

class closeOnKey : public engine::component
{
public:
    std::vector<engine::input::key> closeKeys;

    void update(const float deltaTime) override
    {
        if (std::any_of(closeKeys.begin(), closeKeys.end(), [](engine::input::key key)
            {
                return engine::input::isKeyJustDown(key);
            }))
        {
            std::cout << "closing app...\n";
            engine::application::close();
        }
    }
};

static auto getEntities()
{
    std::vector<engine::entity*> r;
    auto* a = new engine::entity();
    a->name = "debugger";
    a->addComponent<printFps>();
    a->addComponent<closeOnKey>().closeKeys = { engine::input::key::escape, engine::input::key::q };
    r.push_back(a);
    return r;
}

int main()
{
    engine::input::initialize();

    engine::application::run(getEntities());
}