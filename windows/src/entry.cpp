#include "animateTransform.hpp"
#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/transform.hpp"
#include "engine/components/ui/canvasRendering.hpp"
#include "engine/components/ui/uiImage.hpp"
#include "engine/window.hpp"

int main()
{
    __itt_pause();
    engine::log::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false, engine::graphics::renderer::opengl);
    initializeDebugShortcuts();
    // engine::entity::create("triangle")->addComponent<engine::test::renderTriangle>();
    engine::entity::create("camera")->addComponent<engine::camera>();

    auto canvasEntity = engine::entity::create("main canvas");
    canvasEntity->addComponent<engine::ui::canvas>();
    // canvasEntity->addComponent<animateTransform>()->rotationValue = -0.5f;
    // canvasEntity->getComponent<animateTransform>()->rotationAxis = {0, 1, 0};

    auto imageEntity = engine::entity::create("image");
    imageEntity->setParent(canvasEntity);
    imageEntity->addComponent<engine::ui::uiImage>()->color = {1, 0, 0, 0.5f};

    auto imageEntity2 = engine::entity::create("image-inner");
    imageEntity2->setParent(imageEntity);
    // imageEntity2->addComponent<animateTransform>()->rotationValue = 0.01f;
    imageEntity2->addComponent<engine::ui::uiImage>()->color = {0, 0, 0, 1};
    {
        auto ref = imageEntity2->getComponent<engine::ui::uiTransform>();
        ref->maxAnchor.y = 0.5f;
        ref->position.z -= 0.1f;
    }

    auto imageEntity3 = engine::entity::create("image-child");
    imageEntity3->setParent(imageEntity2);
    imageEntity3->addComponent<engine::ui::uiImage>()->color = {1, 1, 1, 1};
    {
        auto ref = imageEntity3->getComponent<engine::ui::uiTransform>();
        ref->minAnchor.x = 0.5f;
        ref->position.z -= 0.2f;
    }

    engine::application::preComponentHooks.push_back([&]() {
        // imageEntity3->getComponent<engine::ui::uiTransform>()->maxAnchor.x = pow(sin(engine::time::getTotalTime()), 2);
        // imageEntity3->getComponent<engine::ui::uiTransform>()->markDirty();
        if (engine::input::isKeyJustDown(engine::input::key::p))
        {
            canvasEntity->getComponent<engine::transform>()->markDirty();
        }
    });

    engine::time::setTargetFps(120);
    engine::application::run();
}