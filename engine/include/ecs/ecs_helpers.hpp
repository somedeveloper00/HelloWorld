#pragma once

#include <vector>
#include "ecs.hpp"

namespace ecs
{
    template<typename T>
    std::vector<T> findAllComponents(const World& world)
    {
        std::vector<T> r;
        world.execute([&r](T& t) { r.push_back(t); });
        return r;
    }
}