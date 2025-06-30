#pragma once
#include "variadicUtils.hpp"
#include <functional>
#include <iostream>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ecs
{
    template<typename T>
    struct _QuickMap
    {
        size_t size; // size of _entries

        struct Entry
        {
            size_t key = 0;
            T value{};
            bool occupied = false;
        };

        _QuickMap(const size_t sizeA)
        {
            // find the appropriate power-of-2 capacity for the size
            size_t n = 1;
            while (n < sizeA)
                n *= 2;
            size = n;
            _entries = new Entry[size];
        }
        ~_QuickMap()
        {
            delete[] _entries;
        }

        T& getFromIndex(const size_t index) { return _entries[index].value; }
        void setFromIndex(const size_t index, const T value) { _entries[index].value = value; }

        T& get(const size_t key)
        {
            const size_t mask = size - 1; // faster than divisor
            size_t idx = key & size;
            for (size_t i = 0; i < size; i++)
            {
                idx = (idx + 1) & mask; // increment but don't over-increment
                if (!_entries[idx].occupied)
                {
                    std::cerr << "key not found" << std::endl;
                    return _entries[0].value;
                }
                if (_entries[idx].key == key)
                    return _entries[idx].value;
            }
            std::cerr << "key not found" << std::endl;
            return _entries[0].value;
        }

        void set(size_t key, const T& value)
        {
            const size_t mask = size - 1;
            size_t idx = key & size;
            for (size_t i = 0; i < size; i++)
            {
                idx = (idx + 1) & mask;
                if (_entries[idx].occupied)
                {
                    if (_entries[idx].key == key)
                    {
                        _entries[idx].value = value;
                        return;
                    }
                }
                else
                {
                    _entries[idx].occupied = true;
                    _entries[idx].key = key;
                    _entries[idx].value = value;
                    return;
                }
            }
            std::cerr << "no empty place found" << std::endl;
        }

        size_t getIndex(size_t key)
        {
            const size_t mask = size - 1;
            size_t idx = key & size;
            for (size_t i = 0; i < size; i++)
            {
                idx = (idx + 1) & mask;
                if (_entries[idx].occupied & (_entries[idx].key == key))
                    return idx;
            }
            return -1;
        }

    private:
        Entry* _entries;
    };

    struct Entity
    {
        const size_t archetypeHash;
        const size_t rowId;
        Entity(size_t archetypeHash, size_t rowId) :archetypeHash(archetypeHash), rowId(rowId)
        {
        }
    };

    // assumes array border handling is done by consumer (Archetype)
    struct ComponentType
    {
        ComponentType() : _size(0)
        {
            std::cerr << "empty ComponentType was created!" << std::endl;
        }

        ComponentType(size_t size) : _size(size)
        {
        }

        void swap(size_t destinationIndex, size_t sourceIndex)
        {
            size_t ptr = (size_t)componentsBuffer.data();
            void* dst = (void*)(ptr + destinationIndex * _size);
            void* src = (void*)(ptr + sourceIndex * _size);
            memcpy(dst, src, _size);
        }

        template<typename T>
        T& get(const size_t index)
        {
            T* ptr = (T*)componentsBuffer.data();
            return ptr[index];
        }

        template<typename T>
        void add(const T& value, const size_t index)
        {
            // ensure size
            if (index * _size >= componentsBuffer.size())
            {
                // new buffer twice the size (starting from 2)
                const auto count = componentsBuffer.size() / _size;
                componentsBuffer.resize(sizeof(T) * (count > 0 ? count : 1) * 2);
            }

            T* ptr = (T*)componentsBuffer.data();
            ptr[index] = value;
        }

    private:
        size_t _size;
        std::vector<std::byte> componentsBuffer{};
    };

    struct Archetype
    {
        const size_t componentCount;
        const size_t id = s_lastId++;
        const std::vector<size_t> componentHashes;
        const size_t archetypeHash;

        template<typename... Ts>
        inline static Archetype* create()
        {
            constexpr size_t count = getVariadicCount<Ts...>();
            constexpr std::array<size_t, count> hashes = createSortedHashes<Ts...>();
            constexpr std::array<size_t, count> sizes = createSortedSizes<Ts...>();
            constexpr size_t hash = createSortedHash<Ts...>();
            Archetype* archetype = new Archetype(
                count,
                std::vector<size_t>(hashes.begin(), hashes.end()),
                hash);
            for (size_t i = 0; i < count; i++)
                archetype->_componentTypes.set(hashes[i], ComponentType(sizes[i]));
            return archetype;
        }

        // add and return index of component (size_t: component hash)
        // it assumes components are correctly aligning with this archtype
        template<typename... Ts>
        size_t add(const Ts... components)
        {
            (..., _addComponent(components));
            return _count++;
        }

        // assumes indices are sorted (least at 0, most at the end)
        // also assumes indices isn't over-sized
        void remove(const std::vector<size_t>& indices)
        {
            size_t i = indices.size() - 1;
            while (true)
            {
                auto row = indices[i];
                // remove row
                for (size_t i = 0; i < _componentTypes.size; i++)
                    _componentTypes.getFromIndex(i).swap(row, _count - 1);
                _count--;

                if (i == 0)
                    break;
                i--;
            }
        }

        template<typename T>
        T& get(size_t index)
        {
            constexpr auto hash = getTypeHash<T>();
            auto& componentType = _componentTypes.get(hash);
            return componentType.get<T>(index);
        }

        size_t getCount() const { return _count; }

        bool hasComponents(const std::vector<size_t>& componentHashes)
        {
            size_t ind = 0;
            size_t count = componentHashes.size();
            for (size_t i = 0; i < componentHashes.size(); i++)
                while (true)
                {
                    if (ind >= count)
                        return false;
                    if (componentHashes[ind] == componentHashes[i])
                        break;
                    ind++;
                }
            return true;
        }

        bool hasExactComponents(const std::vector<size_t>& componentHashes)
        {
            if (componentHashes.size() != componentHashes.size())
                return false;
            for (size_t i = 0; i < componentHashes.size(); i++)
                if (componentHashes[i] != componentHashes[i])
                    return false;
            return true;
        }

    private:
        _QuickMap<ComponentType> _componentTypes;
        size_t _count = 0;
        inline static size_t s_lastId = 1;

        Archetype(size_t componentCount, const std::vector<size_t>& componentHashes, const size_t archetypeHash)
            : componentCount(componentCount), componentHashes(componentHashes), archetypeHash(archetypeHash), _componentTypes(componentCount)
        {
        }

        // assumes caller is executing this when adding a new row
        template<typename T>
        void _addComponent(const T component)
        {
            constexpr auto hash = getTypeHash<T>();
            constexpr auto size = sizeof(T);
            auto& componentType = _componentTypes.get(hash);
            componentType.add(component, _count);
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
        void add(const Ts... components)
        {
            auto& archetype = findOrCreateExactArchetype<Ts...>();
            archetype.add(components...);
        }

        template<size_t N, typename... Ts>
        void remove(const size_t rows[N])
        {
            auto& archetype = findOrCreateExactArchetype<Ts...>();
            archetype.remove(rows);
        }

        template<typename... Ts>
        void remove(const size_t row)
        {
            auto& archetype = findOrCreateExactArchetype<Ts...>();
            archetype.remove({ row });
        }

        template<typename Func>
        void execute(Func&& func)
        {
            using traits = FunctionTraits<std::decay_t<Func>>;
            constexpr auto argsCount = traits::argsCount;
            using firstType = traits::template arg<0>;
            if constexpr (std::is_same_v<firstType, Entity>)
                _executeWithEntity(std::forward<Func>(func), std::make_index_sequence<argsCount - 1>{});
            else
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

        template<typename Func, size_t... Indices>
        void _executeWithEntity(Func&& func, std::index_sequence<Indices...>)
        {
            using traits = FunctionTraits<std::decay_t<Func>>;
            using args = typename traits::args;
            auto& archetypes = findMatchingArchetypes<std::decay_t<typename traits::template arg<Indices + 1>>...>();
            for (size_t i = 0; i < archetypes.size(); i++)
                for (size_t j = 0; j < archetypes[i]->getCount(); j++)
                {
                    Entity entity(archetypes[i]->archetypeHash, j);
                    std::invoke(std::forward<Func>(func), entity, archetypes[i]->get<std::decay_t<typename traits::template arg<Indices + 1>>>(j)...);
                }
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
            std::cout << " hash: " << archetype->archetypeHash << "\n";
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
}