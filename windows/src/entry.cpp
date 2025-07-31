#include "engine/app.hpp"
#include "engine/log.hpp"
#include "engine/input.hpp"
#include "engine/os.hpp"
#include <iostream>
#include <algorithm>

class debugShortcuts : public engine::component
{
public:
    engine::input::key fps = engine::input::key::f;
    engine::input::key hierarchy = engine::input::key::h;
    void update(const float deltaTime) override
    {
        if (engine::input::isKeyJustDown(fps))
        {
            engine::log("frame: {} fps: {} dt: {} slept: {} targetFps: {} frame-use: %{}", engine::time::getTotalFrames(), 1 / deltaTime, deltaTime, engine::time::getLastFrameSleepTime(), engine::time::getTargetFps(), 100 - ((engine::time::getLastFrameSleepTime()) / (1.f / engine::time::getTargetFps()) * 100));
        }
        else if (engine::input::isKeyJustDown(hierarchy))
        {
            auto rootEntities = engine::entity::getRootEntities();
            size_t depth = 0;
            auto print = [&depth](engine::entity* entity)
                {
                    std::string componentsStr;
                    std::vector<std::shared_ptr<engine::component>> components;
                    entity->getComponents(components);
                    for (size_t i = 0; i < components.size(); i++)
                    {
                        componentsStr += ((char*)typeid(*components[i].get()).name() + 6);
                        if (i != components.size() - 1)
                            componentsStr += ", ";
                    }
                    engine::logInfo(std::string(depth * 4, ' ') + "- \"{}\" {{{}}}", entity->name, componentsStr);
                };
            for (size_t i = 0; i < rootEntities.size(); i++)
            {
                engine::entity* current = rootEntities[i].get();
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
};

class closeOnKey : public engine::component
{
public:
    std::vector<engine::input::key> closeKeys;

    void update(const float deltaTime) override
    {
        if (std::any_of(closeKeys.begin(), closeKeys.end(), [](engine::input::key key)
            {
                return engine::input::isKeyJustDown(key);
            }))
        {
            std::cout << "closing app...\n";
            engine::application::close();
        }
    }
};
class printHelloOnKey : public engine::component
{
public:
    engine::input::key key;
    void update(const float deltaTime)
    {
        if (engine::input::isKeyJustDown(key))
            engine::log("Hello World!");
    }
};

static void createEntities()
{
    auto a = engine::entity::create("A");
    a->addComponent<debugShortcuts>();
    a->addComponent<closeOnKey>()->closeKeys = { engine::input::key::escape, engine::input::key::q };
    auto b = engine::entity::create("B");
    b->setParent(a);
    engine::entity::create("C");
    engine::entity::create("D")->setParent(b);
}

int main()
{
    engine::logInfo("mem: {}kb", engine::os::getTotalMemory() / 1024);
    engine::input::initialize();
    engine::logInfo("mem: {}kb", engine::os::getTotalMemory() / 1024);
    createEntities();
    engine::logInfo("mem: {}kb", engine::os::getTotalMemory() / 1024);
    engine::application::run();
    engine::logInfo("mem: {}kb", engine::os::getTotalMemory() / 1024);
}