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
    std::vector<std::byte> componentsBuffer{};
    size_t size;
};

struct Archetype
{
    const size_t componentCount;
    const size_t id = s_lastId++;

    template<typename... Ts>
    inline static Archetype* create()
    {
        constexpr size_t count = getVariadicCount<Ts...>();
        constexpr std::array<size_t, count> hashes = createSortedHashes<Ts...>();
        constexpr std::array<size_t, count> sizes = createSortedSizes<Ts...>();
        Archetype* archetype = new Archetype(
            count,
            std::vector<size_t>(hashes.begin(), hashes.end()));
        archetype->_componentTypes.reserve(count);
        for (size_t i = 0; i < count; i++)
            archetype->_componentTypes[hashes[i]].size = sizes[i];
        return archetype;
    }

    // add and return index of component (size_t: component hash)
    // it assumes components are correctly aligning with this archtype
    template<typename... Ts>
    size_t add(Ts... components)
    {
        (..., _addComponent(components));
        return _count++;
    }

    size_t remove(size_t index, size_t count = 1)
    {
        for (auto& it : _componentTypes)
        {
            auto& componentType = it.second;
            void* ptr = componentType.componentsBuffer.data();
            void* target = (void*)((size_t)ptr + componentType.size * index);
            void* end = (void*)((size_t)ptr + componentType.size * _count);
            void* source = (void*)(std::max((size_t)end - componentType.size * count, (size_t)target + componentType.size * count));
            size_t length = (size_t)ptr + (_count * componentType.size) - (size_t)source;
            memcpy((void*)target, (void*)source, length);
            componentType.componentsBuffer.resize(componentType.size * (_count - count));
        }
        return _count -= count;
    }

    template<typename T>
    T& get(size_t index)
    {
        auto& componentType = _componentTypes[getTypeHash<T>()];
        auto* ptr = componentType.componentsBuffer.data();
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
    inline static size_t s_lastId = 1;

    Archetype(size_t componentCount, const std::vector<size_t>& componentHashes)
        : componentCount(componentCount), _componentHashes(componentHashes)
    {
    }

    // assumes caller is executing this when adding a new row
    template<typename T>
    void _addComponent(const T component)
    {
        constexpr auto hash = getTypeHash<T>();
        constexpr auto size = sizeof(T);
        auto& componentType = _componentTypes[hash];
        componentType.componentsBuffer.resize(size * (_count + 1));
        auto dest = (T*)componentType.componentsBuffer.data();
        dest[_count] = component;
    }
};

struct World
{
    ~World()
    {
        for (auto& it : exact_archetypes)
            delete it.second;
    }

    template<typename...Ts>
    void add(Ts... components)
    {
        auto& archetype = findOrCreateExactArchetype<Ts...>();
        archetype.add(components...);
    }

    template<typename... Ts>
    void remove(size_t rowId, size_t count = 1)
    {
        auto& archetype = findOrCreateExactArchetype<Ts...>();
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
        return findOrCreateExactArchetype<Ts...>();
    }

    int archetypesCount() const { return exact_archetypes.size(); }

    size_t totalEntityCount() const
    {
        size_t r = 0;
        for (auto& archetype : exact_archetypes)
            r += archetype.second->getCount();
        return r;
    }

private:
    // key: createSortedHash<Ts...>
    // if a key is createSortedHash<int,float>(), it will contain pointer to all archetypes having the two components `int` and `float`
    std::unordered_map<size_t, std::vector<Archetype*>> contain_archetypes{};
    std::unordered_map<size_t, Archetype*> exact_archetypes;

    template<typename Func, size_t... Indices>
    void _execute(Func&& func, std::index_sequence<Indices...>)
    {
        using traits = FunctionTraits<std::decay_t<Func>>;
        using args = typename traits::args;
        auto& archetypes = findMatchingArchetypes<std::decay_t<typename traits::template arg<Indices>>...>();
        for (size_t i = 0; i < archetypes.size(); i++)
            for (size_t j = 0; j < archetypes[i]->getCount(); j++)
                std::invoke(std::forward<Func>(func), archetypes[i]->get<std::decay_t<typename traits::template arg<Indices>>>(j)...);
    }

    template<typename... Ts>
    Archetype& findOrCreateExactArchetype()
    {
        constexpr size_t hash = createSortedHash<Ts...>();
        auto it = exact_archetypes.find(hash);
        if (it != exact_archetypes.end())
            return *it->second;

        // create new
        Archetype* archetype = Archetype::create<Ts...>();
        exact_archetypes[hash] = archetype;
        std::cout << "new archetype created for ";
        (..., (std::cout << typeid(Ts).name() << ", "));
        std::cout << "\n";
        addArchetypeToIncludesListRecursively<Ts...>(archetype);
        return *archetype;
    }

    template<typename... Ts>
    void addArchetypeToIncludesListRecursively(Archetype* archetype)
    {
        constexpr auto hashes = createSortedHashes<Ts...>();
        constexpr auto includeHashes = createHashForAllCombinationsOfHashes(hashes);
        for (auto& hash : includeHashes)
        {
            contain_archetypes[hash].push_back(archetype);
            std::cout << "adding to hash of " << hash << "\n";
        }
    }

    template<typename... Ts>
    std::vector<Archetype*>& findMatchingArchetypes()
    {
        constexpr size_t hash = createSortedHash<Ts...>();
        return contain_archetypes[hash];
    }
};