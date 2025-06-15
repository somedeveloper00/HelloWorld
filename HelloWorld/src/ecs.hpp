#pragma once
#include "variadicUtils.hpp"
#include <functional>
#include <iostream>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

struct ComponentType
{
    std::vector<std::byte> components{};
    size_t size;
};

struct Archetype
{
    const size_t componentCount;

    Archetype(const std::vector<size_t>& componentHashes, const std::vector<size_t>& componentSizes) :
        componentCount(componentHashes.size()),
        _componentHashes(componentHashes),
        _componentTypes(componentCount)
    {
        std::cout << "new archetype created\n";
        for (size_t i = 0; i < componentCount; i++)
            _componentTypes[componentHashes[i]].size = componentSizes[i];
    }

    // add and return index of component (size_t: component hash)
    // it assumes components are correctly aligning with this archtype
    template<typename... Ts>
    size_t add(Ts... components)
    {
        (_addComponent(components), ...);
        return _count++;
    }

    size_t remove(size_t index, size_t count = 1)
    {
        for (auto& it : _componentTypes)
        {
            auto& componentType = it.second;
            void* ptr = componentType.components.data();
            void* target = (void*)((size_t)ptr + componentType.size * index);
            void* end = (void*)((size_t)ptr + componentType.size * _count);
            void* source = (void*)(std::max((size_t)end - componentType.size * count, (size_t)target + componentType.size * count));
            size_t length = (size_t)ptr + (_count * componentType.size) - (size_t)source;
            memcpy((void*)target, (void*)source, length);
            componentType.components.resize(componentType.size * (_count - count));
        }
        return _count -= count;
    }

    template<typename T>
    T& get(size_t index)
    {
        auto& componentType = _componentTypes[getTypeHash<T>()];
        auto* ptr = componentType.components.data();
        T* castedPtr = (T*)ptr;
        return castedPtr[index];
    }

    size_t getCount() const { return _count; }

    bool hasComponents(const std::vector<size_t>& componentHashes)
    {
        size_t ind = 0;
        size_t count = _componentHashes.size();
        for (size_t i = 0; i < componentHashes.size(); i++)
            while (true)
            {
                if (ind >= count)
                    return false;
                if (_componentHashes[ind] == componentHashes[i])
                    break;
                ind++;
            }
        return true;
    }

    bool hasExactComponents(const std::vector<size_t>& componentHashes)
    {
        if (_componentHashes.size() != componentHashes.size())
            return false;
        for (size_t i = 0; i < componentHashes.size(); i++)
            if (_componentHashes[i] != componentHashes[i])
                return false;
        return true;
    }

private:
    std::unordered_map<size_t, ComponentType> _componentTypes;
    const std::vector<size_t> _componentHashes;
    size_t _count = 0;

    template<typename T>
    void _addComponent(const T component)
    {
        const auto hash = getTypeHash<T>();
        constexpr auto size = sizeof(T);
        auto& componentType = _componentTypes[hash];
        componentType.components.resize(componentType.size * (_count + 1));
        auto dest = (T*)componentType.components.data();
        dest[_count] = component;
    }
};

struct World
{
    ~World()
    {
        for (auto& archetype : archetypes)
            delete archetype;
    }

    template<typename...Ts>
    void add(Ts... components)
    {
        auto sizes = createSortedSizes<Ts...>();
        auto hashes = createSortedHashes<Ts...>();
        auto& archetype = findOrCreateExactArchetype(hashes, sizes);
        archetype.add(components...);
    }

    template<typename... Ts>
    void remove(size_t rowId, size_t count = 1)
    {
        auto hashes = createSortedHashes<Ts...>();
        auto sizes = createSortedSizes<Ts...>();
        auto& archetype = findOrCreateExactArchetype(hashes, sizes);
        archetype.remove(rowId, count);
    }

    template<typename Func>
    void execute(Func&& func)
    {
        using traits = FunctionTraits<std::decay_t<Func>>;
        constexpr auto argsCount = traits::argsCount;
        _execute(std::forward<Func>(func), std::make_index_sequence<argsCount>{});
    }

    template<typename...Ts>
    Archetype& getArchetype()
    {
        auto hashes = createSortedHashes<Ts...>();
        auto sizes = createSortedSizes<Ts...>();
        auto& archetype = findOrCreateExactArchetype(hashes, sizes);
        return archetype;
    }

    int archetypesCount() const { return archetypes.size(); }

    size_t totalEntityCount() const
    {
        size_t r = 0;
        for (auto archetype : archetypes)
            r += archetype->getCount();
        return r;
    }

private:
    // key: createSortedHash<...>
    std::vector<Archetype*> archetypes{};

    template<typename Func, size_t... Indices>
    void _execute(Func&& func, std::index_sequence<Indices...>)
    {
        using traits = FunctionTraits<std::decay_t<Func>>;
        using args = typename traits::args;
        auto hashes = createSortedHashes<std::decay_t<typename traits::template arg<Indices>>...>();
        auto archetypes = findMatchingArchetypes(hashes);
        for (size_t i = 0; i < archetypes.size(); i++)
            for (size_t j = 0; j < archetypes[i]->getCount(); j++)
                std::invoke(std::forward<Func>(func), archetypes[i]->get<std::decay_t<typename traits::template arg<Indices>>>(j)...);
    }

    Archetype& findOrCreateExactArchetype(const std::vector<size_t>& typeHashes, const std::vector<size_t>& sizes)
    {
        for (size_t i = 0; i < archetypes.size(); i++)
            if (archetypes[i]->hasExactComponents(typeHashes))
                return *archetypes[i];
        // create new
        auto* archetype = new Archetype(typeHashes, sizes);
        archetypes.push_back(archetype);
        return *archetype;
    }

    std::vector<Archetype*> findMatchingArchetypes(const std::vector<size_t> typeHashes)
    {
        std::vector<Archetype*> results{};
        for (size_t i = 0; i < archetypes.size(); i++)
            if (archetypes[i]->hasComponents(typeHashes))
                results.push_back(archetypes[i]);
        return results;
    }
};