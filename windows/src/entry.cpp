#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/test/renderTriangle.hpp"
#include "engine/log.hpp"
#include "engine/ref.hpp"
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

struct A
{
    int a = 0, b = 0, c = 0;
    A() = default;
    A(int a, int b, int c)
        : a(a), b(b), c(c)
    {
    }
};

struct B : public A
{
    float d = 0;
    B() = default;
    B(int a, int b, int c, float d)
        : A(a, b, c), d(d)
    {
    }
};

engine::ownRef<B> getA()
{
    engine::ownRef<B> a{true, 1, 2, 3, 5.f};
    return a;
}

int main()
{
    __itt_pause();
    engine::log::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false, engine::graphics::renderer::opengl);
    initializeDebugShortcuts();
    engine::entity::create("print test")->addComponent<printHelloOnKey>()->key = engine::input::key::p;
    engine::entity::create("triangle")->addComponent<engine::test::renderTriangle>();
    engine::entity::create("camera")->addComponent<engine::camera>();
    engine::time::setTargetFps(120);
    engine::application::run();
}