#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/test/renderTriangle.hpp"
#include "engine/log.hpp"
#include "engine/window.hpp"

struct printHelloOnKey : public engine::component
{
    engine::input::key key;

  private:
    void update_() override
    {
        if (engine::input::isKeyJustDown(key))
            engine::log::logInfo("Hello World!");
    }
};

int main()
{
    // auto* a = new std::vector<size_t>()
    engine::log::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false, engine::graphics::renderer::opengl);
    initializeDebugShortcuts();
    engine::entity::create("print test")->addComponent<printHelloOnKey>()->key = engine::input::key::p;
    engine::entity::create("triangle")->addComponent<engine::test::renderTriangle>();
    engine::entity::create("camera")->addComponent<engine::camera>();
    engine::time::setTargetFps(120);
    engine::application::run();
}