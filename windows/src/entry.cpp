#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/input.hpp"
#include "engine/log.hpp"
#include "engine/os.hpp"
#include "engine/window.hpp"

class printHelloOnKey : public engine::component
{
  public:
    engine::input::key key;
    void update(const float deltaTime)
    {
        if (engine::input::isKeyJustDown(key))
            engine::log("Hello World!");
    }
};

static void memReport(std::string label)
{
    engine::logInfo("[{}] init mem: {}kb", label, engine::os::getTotalMemory() / 1024);
}

int main()
{
    memReport("init");
    engine::input::initialize();
    memReport("engine::input::initialize()");
    engine::window::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false);
    memReport("engine::window::initialize");
    initializeDebugShortcuts();
    memReport("initializeDebugShortcuts()");
    engine::application::run();
    memReport("end");
}