#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/graphics.hpp"
#include "engine/input.hpp"
#include "engine/log.hpp"

class printHelloOnKey : public engine::component
{
  public:
    engine::input::key key;
    void update(const float deltaTime)
    {
        if (engine::input::isKeyJustDown(key))
            engine::log::logInfo("Hello World!");
    }
};

int main()
{
    engine::log::initialize();
    engine::input::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false);
    initializeDebugShortcuts();
    engine::application::run();
}