#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/transform.hpp"
#include "engine/components/ui/canvasRendering.hpp"
#include "engine/components/ui/uiImage.hpp"
#include "engine/window.hpp"
#include "fpsMoveAround.hpp"
#include <cmath>
#include <string>

int main()
{
    __itt_pause();
    engine::log::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {400, 200}, false, false, engine::graphics::renderer::opengl);
    engine::graphics::opengl::debugModeContext _;
    initializeDebugShortcuts();
    // engine::entity::create("triangle")->addComponent<engine::test::renderTriangle>();
    auto camera = engine::entity::create("camera");
    camera->addComponent<engine::camera>();
    camera->addComponent<fpsMoveAround>();

    auto canvasEntity = engine::entity::create("main canvas");
    canvasEntity->addComponent<engine::ui::canvas>();
    canvasEntity->getComponent<engine::ui::canvas>()->positionType = engine::ui::canvas::positionType::world;
    canvasEntity->getComponent<engine::ui::canvas>()->positionProperties.world.unitScale = {0.01f, 0.01f};
    canvasEntity->getComponent<engine::transform>()->position.z = 1;
    auto child = engine::entity::create("fill");
    {
        child->addComponent<engine::ui::uiImage>()->color = {1, 1, 1, 1};
        child->setParent(canvasEntity);
        child->getComponent<engine::ui::uiTransform>()->minAnchor = {0, 0};
        child->getComponent<engine::ui::uiTransform>()->maxAnchor = {1, 1};
    }
    auto rightCorner = engine::entity::create("right corner");
    {
        rightCorner->setParent(canvasEntity);
        rightCorner->addComponent<engine::ui::uiImage>()->color = {1, 0, 0, 1};
        rightCorner->getComponent<engine::ui::uiTransform>()->minAnchor = {1, 1};
        rightCorner->getComponent<engine::ui::uiTransform>()->maxAnchor = {1, 1};
        rightCorner->getComponent<engine::ui::uiTransform>()->deltaSize = {10, 10};
        // rightCorner->getComponent<engine::ui::uiTransform>()->pivot = {1, 1};
        rightCorner->getComponent<engine::ui::uiTransform>()->position.z += 0.01f;
    }
    engine::application::postComponentHooks.push_back([&]() {
        return;
        canvasEntity->getComponent<engine::ui::canvas>()->positionProperties.world.unitScale.x = std::lerp(0.1f, 1, pow(sin(engine::time::getTotalTime()), 2));
        canvasEntity->getComponent<engine::ui::canvas>()->positionProperties.world.unitScale.y = std::lerp(0.5f, 0.1f, pow(sin(engine::time::getTotalTime()), 2));
        engine::log::logInfo("cam: {}", canvasEntity->getComponent<engine::ui::canvas>()->positionProperties.world.unitScale);
        canvasEntity->getComponent<engine::ui::canvas>()->markDirty();
    });

    engine::time::setTargetFps(120);
    engine::application::run();
}