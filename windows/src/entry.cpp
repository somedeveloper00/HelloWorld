#include "animateTransform.hpp"
#include "debugShortcuts.hpp"
#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/pointerRead.hpp"
#include "engine/components/transform.hpp"
#include "engine/components/ui/canvasRendering.hpp"
#include "engine/components/ui/uiImage.hpp"
#include "engine/components/ui/uiImageButton.hpp"
#include "engine/window.hpp"
#include "fpsMoveAround.hpp"

int main()
{
    __itt_pause();
    engine::log::initialize();
    engine::graphics::initialize("Hello Enigne!", {100, 100}, {200, 200}, false, false, engine::graphics::renderer::opengl);
    engine::graphics::opengl::debugModeContext _;
    initializeDebugShortcuts();
    // engine::entity::create("triangle")->addComponent<engine::test::renderTriangle>();
    auto camera = engine::entity::create("camera");
    camera->addComponent<engine::camera>();
    camera->addComponent<fpsMoveAround>();

    auto canvasEntity = engine::entity::create("main canvas");
    canvasEntity->addComponent<engine::ui::canvas>();
    canvasEntity->getComponent<engine::transform>()->position.z = 1;
    // canvasEntity->addComponent<animateTransform>()->rotationValue = -0.5f;
    // canvasEntity->getComponent<animateTransform>()->rotationAxis = {0, 1, 0};

    auto imageEntity = engine::entity::create("test parent");
    imageEntity->setParent(canvasEntity);
    imageEntity->addComponent<engine::ui::uiImage>()->color = {1, 0, 0, 1.f};
    imageEntity->getComponent<engine::ui::uiTransform>()->position.z -= 0.01f;

    auto imageEntity2 = engine::entity::create("image-inner");
    imageEntity2->setParent(imageEntity);
    // imageEntity2->addComponent<animateTransform>()->rotationValue = 1.f;
    imageEntity2->addComponent<engine::ui::uiImage>()->color = {0, 0, 0, 1.f};
    {
        auto ref = imageEntity2->getComponent<engine::ui::uiTransform>();
        ref->deltaSize = {-100.f, -100.f};
        ref->position.z -= 0.1f;
    }

    auto imageEntity3 = engine::entity::create("image-child");
    imageEntity3->setParent(imageEntity2);
    auto btn = imageEntity3->addComponent<engine::ui::uiImageButton>();
    {
        auto ref = imageEntity3->getComponent<engine::ui::uiTransform>();
        ref->minAnchor.x = 0.5f;
        ref->position.z -= 0.1f;
    }

    auto list = engine::entity::create("list");
    list->setParent(imageEntity2);
    list->addComponent<engine::ui::uiImage>()->color = {0.7f, 0.7f, 1, 1.f};
    list->getComponent<engine::ui::uiTransform>()->position.z -= 0.1f;
    list->addComponent<animateTransform>()->rotationAxis = {0, 0, 1};
    list->addComponent<animateTransform>()->rotationValue = 10;
    list->getComponent<engine::ui::uiTransform>()->minAnchor.x = 0.1f;
    list->getComponent<engine::ui::uiTransform>()->maxAnchor.x = 0.4f;
    list->getComponent<engine::ui::uiTransform>()->layout = engine::ui::uiTransform::layout::vertical;
    for (size_t i = 0; i < 0; i++)
    {
        auto item = engine::entity::create("list-item-" + std::to_string(i));
        item->setParent(list);
        auto img = item->addComponent<engine::ui::uiImage>();
        if (i % 2 == 0)
            img->color = {0, 0, 0, 1};
        else
            img->color = {1, 1, 1, 1};
        item->getComponent<engine::ui::uiTransform>()->position.z -= 0.1f;
    }

    auto pointer = engine::entity::create("pointer");
    pointer->addComponent<engine::ui::uiImage>()->color = {1, 1, 1, 1};
    pointer->setParent(engine::entity::create("pointer-canvas")->addComponent<engine::ui::canvas>()->getEntity());
    pointer->getComponent<engine::pointerRead>()->setEnabled(false);
    auto trans = pointer->getComponent<engine::ui::uiTransform>(); //->scale = glm::vec3{0.1f};
    trans->minAnchor = {0.5f, 0.5f};
    trans->maxAnchor = {0.5f, 0.5f};
    trans->deltaSize = {10, 10};
    trans->position = {100, 100, -0.7f};

    engine::application::preComponentHooks.push_back([&]() {
        pointer->getComponent<engine::ui::uiTransform>()->position = {engine::input::getMousePositionCentered(), -.7f};
        pointer->getComponent<engine::ui::uiTransform>()->markDirty();
    });

    engine::time::setTargetFps(120);
    engine::application::run();
}