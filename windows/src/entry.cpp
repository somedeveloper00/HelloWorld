#include "animateTransform.hpp"
#include "common/typeInfo.hpp"
#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/test/renderTriangle.hpp"
#include "engine/components/ui/canvasRendering.hpp"
#include "engine/components/ui/uiImage.hpp"
#include "engine/log.hpp"
#include "engine/ref.hpp"
#include "engine/window.hpp"

struct printHelloOnKey : public engine::component
{
    createTypeInformation(printHelloOnKey, engine::component);
    engine::input::key key;

  private:
    void update_() override
    {
        if (engine::input::isKeyJustDown(key))
            engine::log::logInfo("Hello World!");
    }
};

struct B
{
    createBaseTypeInformation(B);
    int a = 0;
    int b = 1;
};
struct A : public B
{
    createTypeInformation(A, B);
};
struct C : public B
{
    createTypeInformation(C, B);
};
int main()
{
    __itt_pause();
    engine::log::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false, engine::graphics::renderer::opengl);
    initializeDebugShortcuts();
    engine::entity::create("print test")->addComponent<printHelloOnKey>()->key = engine::input::key::p;
    engine::entity::create("triangle")->addComponent<engine::test::renderTriangle>();
    engine::entity::create("camera")->addComponent<engine::camera>();

    auto canvasEntity = engine::entity::create("main canvas");
    canvasEntity->addComponent<engine::ui::canvas>();
    auto imageEntity = engine::entity::create("image");
    imageEntity->setParent(canvasEntity);
    imageEntity->addComponent<engine::ui::uiImage>()->color = {1, 0, 0, 1};

    auto imageEntity2 = engine::entity::create("image-inner");
    imageEntity2->setParent(imageEntity);
    imageEntity2->addComponent<animateTransform>()->rotationValue = 1.f;
    imageEntity2->addComponent<engine::ui::uiImage>()->color = {0, 0, 0, 1};
    {
        auto ref = imageEntity2->getComponent<engine::ui::uiTransform>();
        ref->deltaSize = {-0.2f, -0.2f};
        ref->position.x += 0.1f;
        ref->position.y += 0.1f;
        ref->position.z -= 1;
        // ref->rotation = glm::rotate(ref->rotation, 45.f, glm::vec3(0, 0, 1.f));
    }

    engine::time::setTargetFps(120);
    engine::application::run();
}