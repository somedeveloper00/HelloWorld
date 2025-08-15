#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/graphics.hpp"
#include "engine/graphics/renderTriangle.hpp"
#include "engine/input.hpp"
#include "engine/log.hpp"

struct printHelloOnKey : public engine::component
{
    engine::input::key key;
    void update() override
    {
        if (engine::input::isKeyJustDown(key))
            engine::log::logInfo("Hello World!");
    }
};

int main()
{
    engine::log::initialize();
    engine::input::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false, engine::graphics::renderer::opengl);
    initializeDebugShortcuts();
    engine::entity::create("print test")->addComponent<printHelloOnKey>()->key = engine::input::key::p;
    engine::entity::create("print test")->addComponent<engine::test::renderTriangle>();
    engine::entity::create("camera")->addComponent<engine::camera>();
    engine::application::run();
}