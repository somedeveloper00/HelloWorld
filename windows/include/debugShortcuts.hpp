#pragma once

#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/input.hpp"
#include "engine/log.hpp"


namespace
{
static inline void createRandomEntities_(const size_t count)
{
    static size_t id = 0;
    for (size_t i = 0; i < count; i++)
    {
        auto r = rand() % (engine::entity::getEntitiesCount() + 1);
        auto parent = r == engine::entity::getEntitiesCount() ? nullptr : engine::entity::getEntityAt(r);
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

static inline void debugShortcuts_()
{
    static auto close = engine::input::key::escape;
    static auto fps = engine::input::key::f;
    static auto hierarchy = engine::input::key::h;
    static auto createRandom = engine::input::key::a;
    static auto deleteRandom = engine::input::key::d;
    static auto createRandomCount = 100;
    static auto createRandomHugeCount = 1000;
    static auto deleteRandomCount = 5;

    if (engine::input::isKeyJustDown(close))
    {
        engine::application::close();
    }
    else if (engine::input::isKeyJustDown(fps))
    {
        engine::log::logInfo(
            "frame: {} fps: {} dt: {} slept: {} targetFps: {} frame-use: %{}", engine::time::getTotalFrames(),
            1 / engine::time::getDeltaTime(), engine::time::getDeltaTime(), engine::time::getLastFrameSleepTime(),
            engine::time::getTargetFps(),
            100 - ((engine::time::getLastFrameSleepTime()) / (1.f / engine::time::getTargetFps()) * 100));
    }
    else if (engine::input::isKeyJustDown(hierarchy))
    {
        auto rootEntities = engine::entity::getRootEntities();
        if (rootEntities.size() == 0)
            engine::log::logInfo("no entities.");
        else
        {
            size_t depth = 0;
            auto print = [&depth](engine::entity *entity) {
                std::string componentsStr;
                std::vector<std::shared_ptr<engine::component>> components;
                entity->getComponents(components);
                for (size_t i = 0; i < components.size(); i++)
                {
                    componentsStr += ((char *)typeid(*components[i].get()).name() + 6);
                    if (i != components.size() - 1)
                        componentsStr += ", ";
                }
                engine::log::logInfo(std::string(depth * 4, ' ') + "- \"{}\" {{{}}}", entity->name, componentsStr);
            };
            for (size_t i = 0; i < rootEntities.size(); i++)
            {
                engine::entity *current = rootEntities[i].get();
                while (true)
                {
                    print(current);
                    auto children = current->getChildren();
                    if (children.size() > 0)
                    {
                        current = children[0].get();
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
                            current = parent.get();
                            depth--;
                        }
                        else
                        {
                            current = siblings[siblingIndex + 1].get();
                            goto nextIter;
                        }
                    }
                nextIter:;
                }
            nextRoot:;
            }
        }
    }
    else if (engine::input::isKeyHeldDown(engine::input::key::shift) && engine::input::isKeyJustDown(createRandom))
    {
        {
            bench("add-huge");
            createRandomEntities_(createRandomHugeCount);
        }
        engine::log::logInfo("created {} entities", createRandomHugeCount);
    }
    else if (engine::input::isKeyJustDown(createRandom))
    {
        {
            bench("add");
            createRandomEntities_(createRandomCount);
        }
        engine::log::logInfo("created {} entities", createRandomCount);
    }
    else if (engine::input::isKeyJustDown(deleteRandom))
    {
        bench("remove");
        deleteRandomEntities_(deleteRandomCount);
    }
}
} // namespace

static inline void initializeDebugShortcuts()
{
    engine::application::addPreComponentHook(debugShortcuts_);
}
