#pragma once
#include "common/typeHash.hpp"
#include <iostream>
#include <span>
#include <stdlib.h>
#include <unordered_map>

namespace ecs
{
// function traits
namespace
{
template <typename F>
struct FunctionTraits;

// free/function pointer
template <typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)>
{
    using returnType = R;

    static constexpr size_t argsCount = sizeof...(Args);

    template <size_t N>
    using arg = std::tuple_element_t<N, std::tuple<Args...>>;

    using args = std::tuple<Args...>;
};

// functor/lambda
template <typename F>
struct FunctionTraits
{
    // underlying function traits
    using _underlyingFunctionTraits = FunctionTraits<decltype(&F::operator())>;

    using returnType = _underlyingFunctionTraits::returnType;

    static constexpr size_t argsCount = _underlyingFunctionTraits::argsCount;

    template <size_t N>
    using arg = _underlyingFunctionTraits::template arg<N>;

    using args = _underlyingFunctionTraits::args;
};

// member function pointer
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)> : FunctionTraits<R (*)(Args...)>
{
};

// const member function pointer
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const> : FunctionTraits<R (*)(Args...)>
{
};

// function reference
template <typename R, typename... Args>
struct FunctionTraits<R (&)(Args...)> : FunctionTraits<R (*)(Args...)>
{
};

// noexcept function pointer
template <typename R, typename... Args>
struct FunctionTraits<R (*)(Args...) noexcept> : FunctionTraits<R (*)(Args...)>
{
};
} // namespace

// hash
namespace
{
// map from full hash of components to individual hashes of components
inline static std::unordered_map<size_t, std::vector<size_t>> _hashToSubHash;

// get total hash of individual component hashes
// assumes hashes are sorted (smallest at first and largest at last)
static inline size_t getHash_(const std::vector<size_t> &hashes)
{
    if (hashes.size() == 1)
    {
        _hashToSubHash.insert({hashes[0], {hashes[0]}});
        return hashes[0];
    }
    size_t hash = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < hashes.size(); ++i)
    {
        hash ^= hashes[i];
        hash *= 0x100000001b3ULL;
    }

    // update map
    const auto &it = _hashToSubHash.find(hash);
    if (it == _hashToSubHash.end())
        _hashToSubHash.insert({hash, hashes});

    return hash;
}

// sub: child list
// whole: super list
// assumes both are sorted
// O(N) where N is whole.size()
static inline bool hashCollides_(const std::vector<size_t> &sub, const std::vector<size_t> &whole)
{
    size_t s = 0;
    for (size_t i = 0; i < whole.size(); i++)
        if (whole[i] == sub[s])
        {
            s++;
            if (s == sub.size())
                return true;
        }
    return false;
}

static constexpr void sortHashesAndSizes__(std::vector<size_t> &hashes, std::vector<size_t> &sizes)
{
    const size_t count = hashes.size();
    // bubble sort
    for (size_t i = 0; i < count - 1; i++)
        for (size_t j = i + 1; j < count; j++)
            if (hashes[j] > hashes[i])
            {
                std::swap(hashes[i], hashes[j]);
                std::swap(sizes[i], sizes[j]);
            }
}

static constexpr void ensureNeitherEqual__(std::vector<size_t> &hashes)
{
    for (size_t i = 0; i < hashes.size() - 1; i++)
        if (hashes[i] == hashes[i + 1])
        {
            std::cerr << "usage error: duplicate component hashes found: " << hashes[i] << std::endl;
            abort();
        }
}

// first: hashes second: sizes
template <typename... Ts>
static constexpr std::pair<std::vector<size_t>, std::vector<size_t>> createSortedHashesAndSizes_()
{
    constexpr size_t count = sizeof...(Ts);
    std::vector<size_t> hashes{getTypeHash_<Ts>()...};
    std::vector<size_t> sizes{sizeof(Ts)...};
    sortHashesAndSizes__(hashes, sizes);
    ensureNeitherEqual__(hashes);
    return {std::move(hashes), std::move(sizes)};
}

// first: hashes second: sizes
template <typename... Ts>
static constexpr std::pair<std::vector<size_t>, std::vector<size_t>> createAppendedSortedHashesAndSizes_(const std::vector<size_t> &oldHashes, const std::vector<size_t> &oldSizes)
{
    std::vector<size_t> hashes;
    std::vector<size_t> sizes;
    hashes.reserve(oldHashes.size() + sizeof...(Ts));
    sizes.reserve(oldHashes.size() + sizeof...(Ts));

    hashes.insert(hashes.end(), oldHashes.begin(), oldHashes.end());
    sizes.insert(sizes.end(), oldSizes.begin(), oldSizes.end());

    (..., hashes.push_back(getTypeHash_<Ts>()));
    (..., sizes.push_back(sizeof(Ts)));

    sortHashesAndSizes__(hashes, sizes);
    ensureNeitherEqual__(hashes);
    return {std::move(hashes), std::move(sizes)};
}

// first: hashes second: sizes
template <typename... Ts>
static constexpr std::pair<std::vector<size_t>, std::vector<size_t>> createRemovedSortedHashesAndSizes_(const std::vector<size_t> &oldHashes, const std::vector<size_t> &oldSizes)
{
    std::vector<size_t> hashes;
    std::vector<size_t> sizes;
    hashes.reserve(oldHashes.size() - sizeof...(Ts));
    sizes.reserve(oldHashes.size() - sizeof...(Ts));

    const std::vector<size_t> removingHashes{getTypeHash_<Ts>()...};

    for (size_t i = 0; i < oldHashes.size(); i++)
        if (std::find(removingHashes.begin(), removingHashes.end(), oldHashes[i]) == removingHashes.end())
        {
            hashes.push_back(oldHashes[i]);
            sizes.push_back(oldSizes[i]);
        }
    return {std::move(hashes), std::move(sizes)};
}
} // namespace

struct Entity
{
    const size_t rowIndex;
    const size_t archetypeHash;
    const size_t worldVer;
};

namespace
{
struct Archetype
{
    const size_t hash;
    const std::vector<size_t> componentHashes;
    const std::vector<size_t> componentSizes;
    const std::unordered_map<size_t, size_t> componentHashMap;

    Archetype()
        : hash(), componentHashMap(), componentHashes(), componentSizes(), _componentRows(), _toRemove()
    {
    }

    // assumes hashes is sorted
    Archetype(const std::vector<size_t> &hashes, const std::vector<size_t> &sizes)
        : hash(getHash_(hashes)), componentHashMap(createComponentHashMap_(hashes)), componentSizes(sizes), componentHashes(hashes), _componentRows(), _toRemove()
    {
        _componentRows.reserve(hashes.size());
        for (size_t i = 0; i < hashes.size(); i++)
            _componentRows.push_back({});
    }

    std::span<std::byte> getComponent(const size_t hash, const size_t rowIndex)
    {
        const size_t index = componentHashMap.at(hash);
        const size_t size = componentSizes[index];
        std::byte *ptr = _componentRows[index].data();
        return std::span<std::byte>(ptr + size * rowIndex, size);
    }

    std::vector<std::span<std::byte>> getRow(const size_t rowIndex)
    {
        std::vector<std::span<std::byte>> result;
        result.reserve(componentSizes.size());
        for (size_t i = 0; i < componentSizes.size(); i++)
        {
            auto component = getComponent(componentHashes[i], rowIndex);
            result.push_back(component);
        }
        return result;
    }

    // hashes' indices correspond to the components' indices
    void add(const std::vector<std::span<std::byte>> &components, const std::vector<size_t> &hashes)
    {
        // per component (not per row)
        for (size_t i = 0; i < hashes.size(); i++)
        {
            const size_t hash = hashes[i];
            const auto &addingRows = components[i];

            const size_t index = componentHashMap.at(hash);
            const size_t size = componentSizes[index];
            auto &rows = _componentRows[index];

            rows.reserve(rows.size() + addingRows.size());
            rows.insert(rows.end(), addingRows.begin(), addingRows.end());
        }
    }

    void markForRemoval(const size_t rowIndex)
    {
        auto it = std::lower_bound(_toRemove.begin(), _toRemove.end(), rowIndex);
        _toRemove.insert(it, rowIndex);
    }

    void flushMarks()
    {
        flushRemoves_();
    }

    size_t getRowsCount() const
    {
        return _componentRows[0].size() / componentSizes[0];
    }

  private:
    std::vector<std::vector<std::byte>> _componentRows;
    std::vector<size_t> _toRemove; // sorted: least value at 0 largest at last

    void flushRemoves_()
    {
        if (_toRemove.size() == 0)
            return;
        // loop from largest-index to smallest-index
        for (size_t i = _toRemove.size(); i-- > 0;)
        {
            const size_t deleteIndex = _toRemove[i];
            for (size_t j = 0; j < _componentRows.size(); j++)
            {
                std::vector<std::byte> &rows = _componentRows[j];
                const size_t size = componentSizes[j];
                // swap index with the last
                if (deleteIndex < rows.size() / size - 1)
                    std::swap_ranges(
                        /* first1 */ rows.begin() + deleteIndex * size,
                        /* last1 */ rows.begin() + deleteIndex * size + size,
                        /* first2 */ rows.end() - size);
                rows.resize(rows.size() - size);
            }
        }

        _toRemove.clear();
    }

    static std::unordered_map<size_t, size_t> createComponentHashMap_(const std::vector<size_t> &hashes)
    {
        std::unordered_map<size_t, size_t> result;
        result.reserve(hashes.size());
        for (size_t i = 0; i < hashes.size(); i++)
            result[hashes[i]] = i;
        return result;
    }

    template <typename T>
    static constexpr std::vector<T> appendTwoVectors_(const std::vector<T> &vec1, const std::vector<T> &vec2)
    {
        std::vector<T> result;
        result.reserve(vec1.size() + vec2.size());
        result.insert(result.end(), vec1.begin(), vec1.end());
        result.insert(result.end(), vec1.begin(), vec1.end());
        return result;
    }
};
} // namespace

struct World
{
    // adds an entity right away
    template <typename... Ts>
    Entity addEntity(const Ts... components)
    {
        // find archetype
        auto [hashes, sizes] = createSortedHashesAndSizes_<Ts...>();
        auto &archetype = getOrCreateArchetype_(hashes, sizes);

        // prepare components as byte vectors
        const std::vector<size_t> unsortedHashes = {getTypeHash_<Ts>()...};
        std::vector<std::span<std::byte>> componentsAsbytes;
        componentsAsbytes.reserve(sizeof...(Ts));
        auto callback = [&componentsAsbytes](auto &val) {
            std::byte *ptr = (std::byte *)&val;
            std::span<std::byte> bytes(ptr, sizeof(decltype(val)));
            componentsAsbytes.push_back(std::move(bytes));
        };
        (..., callback(components));

        archetype.add(componentsAsbytes, unsortedHashes);
        return Entity{
            .rowIndex = archetype.getRowsCount() - 1,
            .archetypeHash = archetype.hash,
            .worldVer = _ver};
    }

    // removes an entity. needs a flush
    void removeEntity(const Entity &entity)
    {
        abortIfEntityNotUpdated_(entity);
        auto &archetype = _archetypes.at(entity.archetypeHash);
        archetype.markForRemoval(entity.rowIndex);
    }

    // executes tasks awaiting a flush
    void flush()
    {
        while (_executingCount != 0)
        {
        }
        for (auto &[_, archetype] : _archetypes)
            archetype.flushMarks();
        _ver++;
    }

    // returns whether this entity contains this component type
    template <typename T>
    bool componentExists(const Entity &entity)
    {
        abortIfEntityNotUpdated_(entity);
        auto &archetype = _archetypes[entity.archetypeHash];
        constexpr auto hash = getTypeHash_<T>();
        return std::find(archetype.componentHashes.begin(), archetype.componentHashes.end(), hash) != archetype.componentHashes.end();
    }

    // returns a component from this entity
    // assumes component exists in this entity (no error checking)
    template <typename T>
    T &getComponent(const Entity &entity)
    {
        abortIfEntityNotUpdated_(entity);
        auto &archetype = _archetypes[entity.archetypeHash];
        constexpr auto hash = getTypeHash_<T>();
        auto asByte = archetype.getComponent(hash, entity.rowIndex);
        return *(T *)asByte.data();
    }

    // adds components to the entity. needs a flush
    template <typename... Ts>
    void addComponents(const Entity &entity, const Ts... components)
    {
        abortIfEntityNotUpdated_(entity);
        auto &archetype = _archetypes.at(entity.archetypeHash);
        archetype.markForRemoval(entity.rowIndex);

        // find target archetype
        auto [hashes, sizes] = createAppendedSortedHashesAndSizes_<Ts...>(archetype.componentHashes, archetype.componentSizes);
        auto &targetArchetype = getOrCreateArchetype_(hashes, sizes);

        // add entity
        auto row = archetype.getRow(entity.rowIndex);
        row.reserve(row.size() + sizeof...(Ts));
        auto callback = [&row](auto &val) {
            std::byte *ptr = (std::byte *)&val;
            std::span<std::byte> bytes(ptr, sizeof(decltype(val)));
            row.push_back(std::move(bytes));
        };
        (..., callback(components));
        std::vector<size_t> unsortedHashes;
        unsortedHashes.reserve(hashes.size());
        unsortedHashes.insert(unsortedHashes.end(), archetype.componentHashes.begin(), archetype.componentHashes.end());
        (..., unsortedHashes.push_back(getTypeHash_<Ts>()));

        targetArchetype.add(row, unsortedHashes);
    }

    // removes components from the entity. needs a flush
    template <typename... Ts>
    void removeComponents(const Entity &entity)
    {
        abortIfEntityNotUpdated_(entity);
        auto &archetype = _archetypes.at(entity.archetypeHash);
        archetype.markForRemoval(entity.rowIndex);

        // find target archetype
        auto [hashes, sizes] = createRemovedSortedHashesAndSizes_<Ts...>(archetype.componentHashes, archetype.componentSizes);
        auto &targetArchetype = getOrCreateArchetype_(hashes, sizes);

        // add entity
        auto row = archetype.getRow(entity.rowIndex);
        std::vector<size_t> removingHashes{getTypeHash_<Ts>()...};
        for (size_t i = row.size(); i-- > 0;)
            if (std::find(removingHashes.begin(), removingHashes.end(), archetype.componentHashes[i]) != removingHashes.end())
                row.erase(row.begin() + i);

        targetArchetype.add(row, hashes);
    }

    // executes function on this world's entities in multiple threads
    template <typename Func>
    void executeParallel(Func &&func)
    {
        _executingCount++;
        using traits = FunctionTraits<std::decay_t<Func>>;
        constexpr size_t argsCount = traits::argsCount;
        using firstType = traits::template arg<0>;
        if constexpr (std::is_same_v<firstType, Entity &>)
            executeWithEntity_<true>(std::forward<Func>(func), std::make_index_sequence<argsCount - 1>{});
        else
            execute_<true>(std::forward<Func>(func), std::make_index_sequence<argsCount>{});
        _executingCount--;
    }

    // executes function on this world's entities
    template <typename Func>
    void execute(Func &&func)
    {
        _executingCount++;
        using traits = FunctionTraits<std::decay_t<Func>>;
        constexpr size_t argsCount = traits::argsCount;
        using firstType = traits::template arg<0>;
        if constexpr (std::is_same_v<firstType, Entity &>)
            executeWithEntity_<false>(std::forward<Func>(func), std::make_index_sequence<argsCount - 1>{});
        else
            execute_<false>(std::forward<Func>(func), std::make_index_sequence<argsCount>{});
        _executingCount--;
    }

    size_t getTotalEntityCount() const
    {
        size_t r = 0;
        for (auto &[_, archetype] : _archetypes)
            r += archetype.getRowsCount();
        return r;
    }

    size_t getTotalArchetypesCount() const
    {
        return _archetypes.size();
    }

  private:
    // exact archetype hash to archetype map
    std::unordered_map<size_t, Archetype> _archetypes;

    // cached archetypes for a components' hash search
    std::unordered_map<size_t, std::vector<Archetype *>> _includeArchetypesCache;

    size_t _ver = 0;
    size_t _executingCount = 0;

    template <bool Parallel, typename Func, size_t... Indices>
    void executeWithEntity_(Func &&func, std::index_sequence<Indices...>)
    {
        using traits = FunctionTraits<std::decay_t<Func>>;
        using args = typename traits::args;
        const auto [hashes, sizes] = createSortedHashesAndSizes_<std::decay_t<typename traits::template arg<Indices + 1>>...>();
        std::vector<Archetype *> archetypes = findArchetypesWithHashes_(hashes);
        void *ptrs[sizeof...(Indices)]; // for later use
        for (size_t i = 0; i < archetypes.size(); i++)
        {
            Archetype &archetype = *archetypes[i];

            // get internal component arrays
            ((ptrs[Indices] = (void *)archetype.getComponent(getTypeHash_<std::decay_t<typename traits::template arg<Indices + 1>>>(), 0).data()), ...);

            if constexpr (Parallel)
#pragma omp parallel for schedule(static)
                for (signed long long j = 0; j < archetype.getRowsCount(); j++)
                {
                    Entity entity{static_cast<size_t>(j), archetype.hash, _ver};
                    std::invoke(
                        std::forward<Func>(func),
                        entity,
                        // take indices from internal component arrays
                        ((std::decay_t<typename traits::template arg<Indices + 1>> *)ptrs[Indices])[j]...);
                }
            else
                for (size_t j = 0; j < archetype.getRowsCount(); j++)
                {
                    Entity entity{j, archetype.hash, _ver};
                    std::invoke(
                        std::forward<Func>(func),
                        entity,
                        // take indices from internal component arrays
                        ((std::decay_t<typename traits::template arg<Indices + 1>> *)ptrs[Indices])[j]...);
                }
        }
    }

    template <bool Parallel, typename Func, size_t... Indices>
    void execute_(Func &&func, std::index_sequence<Indices...>)
    {
        using traits = FunctionTraits<std::decay_t<Func>>;
        using args = typename traits::args;
        const auto [hashes, sizes] = createSortedHashesAndSizes_<std::decay_t<typename traits::template arg<Indices>>...>();
        std::vector<Archetype *> archetypes = findArchetypesWithHashes_(hashes);
        void *ptrs[sizeof...(Indices)]; // to be used later
        for (size_t i = 0; i < archetypes.size(); i++)
        {
            Archetype &archetype = *archetypes[i];

            // get internal component arrays
            ((ptrs[Indices] = (void *)archetype.getComponent(getTypeHash_<std::decay_t<typename traits::template arg<Indices>>>(), 0).data()), ...);

            if constexpr (Parallel)
#pragma omp parallel for schedule(static)
                for (signed long long j = 0; j < archetype.getRowsCount(); j++)
                    std::invoke(
                        std::forward<Func>(func),
                        // take indices from internal component arrays
                        ((std::decay_t<typename traits::template arg<Indices>> *)ptrs[Indices])[j]...);
            else
                for (size_t j = 0; j < archetype.getRowsCount(); j++)
                    std::invoke(
                        std::forward<Func>(func),
                        // take indices from internal component arrays
                        ((std::decay_t<typename traits::template arg<Indices>> *)ptrs[Indices])[j]...);
        }
    }

    void abortIfEntityNotUpdated_(const Entity &entity) const
    {
        if (entity.worldVer != _ver)
        {
            std::cerr << "usage error: version mismatch: " << entity.worldVer << " expected " << _ver << std::endl;
            abort();
        }
    }

    Archetype &getOrCreateArchetype_(const std::vector<size_t> &hashes, const std::vector<size_t> &sizes)
    {
        const size_t hash = getHash_(hashes);
        const auto &it = _archetypes.find(hash);
        if (it != _archetypes.end())
            return it->second;

        // create new archetype
        const auto &insertion = _archetypes.insert({hash, Archetype(hashes, sizes)});
        auto &archetype = insertion.first->second;

        // add to hash caches
        for (auto &[hash, archetypes] : _includeArchetypesCache)
        {
            const auto &componentHashes = _hashToSubHash.at(hash);

            // check if this archetype belongs to this set
            if (hashCollides_(componentHashes, hashes))
                archetypes.push_back(&archetype);
        }
        return archetype;
    }

    std::vector<Archetype *> findArchetypesWithHashes_(const std::vector<size_t> hashes)
    {
        const size_t hash = getHash_(hashes);
        const auto &it = _includeArchetypesCache.find(hash);
        if (it != _includeArchetypesCache.end())
            return it->second;

        // create
        const auto &newIt = _includeArchetypesCache.insert({hash, {}}).first;
        auto &archetypesList = newIt->second;
        for (auto &[_, archetype] : _archetypes)
            if (hashCollides_(hashes, archetype.componentHashes))
                archetypesList.push_back(&archetype);
        return archetypesList;
    }
};
} // namespace ecs