#pragma once

#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/components/test/renderTriangle.hpp"
#include "engine/log.hpp"
#include "engine/quickVector.hpp"
#include "engine/ref.hpp"
#include "engine/window.hpp"
#include <cmath>

namespace
{
static inline void createRandomEntities_(const size_t count)
{
    static size_t id = 0;
    for (size_t i = 0; i < count; i++)
    {
        auto r = rand() % (engine::entity::getEntitiesCount() + 1);
        auto parent = r == engine::entity::getEntitiesCount() ? engine::weakRef<engine::entity>{} : engine::entity::getEntityAt(r);
        auto newEntity = engine::entity::create("random " + std::to_string(++id));
        newEntity->setParent(parent);
    }
}

static inline void deleteRandomEntities_(const size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (engine::entity::getEntitiesCount() == 0)
            return;
        auto r = rand() % engine::entity::getEntitiesCount();
        auto entity = engine::entity::getEntityAt(r);
        engine::log::logInfo("deleting entity \"{}\" at {}", entity->name, r);
        entity->remove();
    }
}

static inline void tick_()
{
    static auto close = engine::input::key::escape;
    static auto fps = engine::input::key::f;
    static auto hierarchy = engine::input::key::h;
    static auto addTriangle = engine::input::key::t;
    static auto createRandomCount = 100;
    static auto createRandomHugeCount = 1000000;
    static auto deleteRandomCount = 5;

    // engine::graphics::clearColor.r = sin(engine::time::getTotalTime()) * sin(engine::time::getTotalTime());
    // engine::graphics::clearColor.g = cos(2 * engine::time::getTotalTime()) * cos(2 * engine::time::getTotalTime());
    // engine::graphics::clearColor.b = engine::time::getTotalTime() - (float)(int)engine::time::getTotalTime();

    if (engine::input::isKeyJustDown(close))
    {
        engine::log::logInfo("closing. total frames: {} total execution time: {}", engine::time::getTotalFrames(), engine::time::getTotalTime());
        engine::application::close();
    }
    else if (engine::input::isKeyJustDown(fps))
    {
        engine::log::logInfo("frame: {} fps: {} dt: {} slept: {} targetFps: {} total-entities: {} frame-use: %{}", engine::time::getTotalFrames(), 1 / engine::time::getDeltaTime(), engine::time::getDeltaTime(), engine::time::getLastFrameSleepTime(), engine::time::getTargetFps(), engine::entity::getEntitiesCount(), 100 - ((engine::time::getLastFrameSleepTime()) / (1.f / engine::time::getTargetFps()) * 100));
    }
    else if (engine::input::isKeyJustDown(hierarchy))
    {
        auto rootEntities = engine::entity::getRootEntities();
        if (rootEntities.size() == 0)
            engine::log::logInfo("no entities.");
        else
        {
            engine::log::logInfo("entities hierarchy:");
            size_t depth = 0;
            auto print = [&depth](engine::entity *entity) {
                std::string componentsStr;
                engine::quickVector<engine::weakRef<engine::component>> components;
                entity->getComponents(components);
                for (size_t i = 0; i < components.size(); i++)
                {
                    engine::component &comp = *(engine::component *)components[i];
                    componentsStr += comp.getEnabled() ? comp.getTypeName() : (comp.getTypeName() + "(X)");
                    if (i != components.size() - 1)
                        componentsStr += ", ";
                }
                engine::log::logInfo(std::string(depth * 4, ' ') + "- \"{}\" {{{}}}", entity->name, componentsStr);
            };
            for (size_t i = 0; i < rootEntities.size(); i++)
            {
                engine::entity *current = rootEntities[i];
                while (true)
                {
                    print(current);
                    auto children = current->getChildren();
                    if (children.size() > 0)
                    {
                        current = children[0];
                        depth++;
                        continue;
                    }
                    while (true)
                    {
                        auto siblingIndex = current->getSiblindIndex();
                        if (siblingIndex == engine::notFound)
                            goto nextRoot;
                        auto parent = current->getParent();
                        auto siblings = parent->getChildren();
                        if (siblingIndex >= siblings.size() - 1)
                        {
                            current = parent;
                            depth--;
                        }
                        else
                        {
                            current = siblings[siblingIndex + 1];
                            goto nextIter;
                        }
                    }
                nextIter:;
                }
            nextRoot:;
            }
        }
    }
    else if (engine::input::isKeyJustDown(addTriangle))
    {
        for (size_t i = 0; i < 200000; i++)
            engine::entity::create("triangle")->addComponent<engine::test::renderTriangle>();
        __itt_resume();
    }
}
} // namespace

static inline void initializeDebugShortcuts()
{
    engine::graphics::clearColor = {0, 0, 0, 1};
    engine::application::preComponentHooks.push_back(tick_);
}
