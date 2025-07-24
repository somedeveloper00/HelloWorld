#pragma once
#include <unordered_map>
#include <stdlib.h>
#include <span>

namespace ecs
{
    // function traits
    namespace
    {
        template<typename F>
        struct FunctionTraits;

        // free/function pointer
        template<typename R, typename... Args>
        struct FunctionTraits<R(*)(Args...)>
        {
            using returnType = R;

            static constexpr size_t argsCount = sizeof...(Args);

            template<size_t N>
            using arg = std::tuple_element_t<N, std::tuple<Args...>>;

            using args = std::tuple<Args...>;
        };

        // functor/lambda 
        template<typename F>
        struct FunctionTraits
        {
            // underlying function traits
            using _underlyingFunctionTraits = FunctionTraits<decltype(&F::operator())>;

            using returnType = _underlyingFunctionTraits::returnType;

            static constexpr size_t argsCount = _underlyingFunctionTraits::argsCount;

            template<size_t N>
            using arg = _underlyingFunctionTraits::template arg<N>;

            using args = _underlyingFunctionTraits::args;
        };

        // member function pointer
        template<typename C, typename R, typename... Args>
        struct FunctionTraits<R(C::*)(Args...)> : FunctionTraits<R(*)(Args...)> {};

        // const member function pointer
        template<typename C, typename R, typename... Args>
        struct FunctionTraits<R(C::*)(Args...) const> : FunctionTraits<R(*)(Args...)> {};

        // function reference
        template<typename R, typename... Args>
        struct FunctionTraits<R(&)(Args...)> : FunctionTraits<R(*)(Args...)> {};

        // noexcept function pointer
        template<typename R, typename... Args>
        struct FunctionTraits<R(*)(Args...) noexcept> : FunctionTraits<R(*)(Args...)> {};
    }

    // hash
    namespace
    {
        // map from full hash of components to individual hashes of components
        inline static std::unordered_map<size_t, std::vector<size_t>> _hashToSubHash;

        // get total hash of individual component hashes
        // assumes hashes are sorted (smallest at first and largest at last)
        static inline size_t getHash_(const std::vector<size_t>& hashes)
        {
            size_t hash = 0xcbf29ce484222325ULL;
            for (size_t i = 0; i < hashes.size(); ++i)
            {
                hash ^= hashes[i];
                hash *= 0x100000001b3ULL;
            }

            // update map
            const auto& it = _hashToSubHash.find(hash);
            if (it == _hashToSubHash.end())
                _hashToSubHash.insert({ hash, hashes });

            return hash;
        }

        // sub: child list
        // whole: super list
        // assumes both are sorted
        // O(N) where N is whole.size()
        static inline bool hashCollides_(const std::vector<size_t>& sub, const std::vector<size_t>& whole)
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

        static constexpr size_t fnv1a_64_(const char* s, size_t count)
        {
            size_t hash = 0xcbf29ce484222325ULL;
            for (size_t i = 0; i < count; ++i)
            {
                hash ^= static_cast<size_t>(s[i]);
                hash *= 0x100000001b3ULL;
            }
            return hash;
        }

        template <typename T>
        static constexpr size_t getTypeHash_()
        {
#if defined(__clang__) || defined(__GNUC__)
            constexpr auto& sig = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
            constexpr auto& sig = __FUNCSIG__;
#else
#error "Unsupported compiler for compile-time type hash"
#endif
            // compute FNV-1a over the full signature
            return fnv1a_64_(sig, sizeof(sig) / sizeof(char));
        }

        // first: hashes second: sizes
        template<typename... Ts>
        static constexpr std::pair<std::vector<size_t>, std::vector<size_t>> createSortedHashesAndSizes_()
        {
            constexpr size_t count = sizeof...(Ts);
            std::vector<size_t> hashes{ getTypeHash_<Ts>()... };
            std::vector<size_t> sizes{ sizeof(Ts)... };
            //bubble sort
            for (size_t i = 0; i < count - 1; i++)
                for (size_t j = i + 1; j < count; j++)
                    if (hashes[j] > hashes[i])
                    {
                        std::swap(hashes[i], hashes[j]);
                        std::swap(sizes[i], sizes[j]);
                    }
            return { std::move(hashes), std::move(sizes) };
        }
    }

    struct Entity
    {
        const size_t rowIndex;
        const size_t archetypeHash;
        const size_t worldVer;
    };

    struct Archetype
    {
        const size_t hash;
        const std::unordered_map<size_t, size_t> _componentHashMap;
        const std::vector<size_t> _componentHashes;
        const std::vector<size_t> _componentSizes;
        std::vector<std::vector<std::byte>> _componentRows;
        std::vector<size_t> _toRemove; // sorted: least value at 0 largest at last

        Archetype() : hash(), _componentHashMap(), _componentHashes(), _componentSizes(), _componentRows(), _toRemove() {}

        // assumes hashes is sorted
        Archetype(const std::vector<size_t>& sizes, const std::vector<size_t>& hashes) : hash(getHash_(hashes)), _componentHashMap(createComponentHashMap_(hashes)), _componentSizes(sizes), _componentHashes(hashes), _componentRows(), _toRemove()
        {
            _componentRows.reserve(hashes.size());
            for (size_t i = 0; i < hashes.size(); i++)
                _componentRows.push_back({});
        }

        static Archetype createAppended(const Archetype& other, const std::vector<size_t>& newSizes, const std::vector<size_t>& newHashes)
        {
            std::vector<size_t> hashes;
            std::vector<size_t> sizes;
            const size_t count = other._componentHashes.size() + newHashes.size();
            hashes.reserve(count);
            sizes.reserve(count);
            hashes.insert(hashes.end(), other._componentHashes.begin(), other._componentHashes.end());
            sizes.insert(sizes.end(), other._componentSizes.begin(), other._componentSizes.end());

            // sort based on hashes
            for (size_t i = 0; i < hashes.size() - 1; i++)
                for (size_t j = i + 1; j < hashes.size(); j++)
                    if (hashes[i] > hashes[j])
                    {
                        std::swap(hashes[i], hashes[j]);
                        std::swap(sizes[i], sizes[j]);
                    }

            return Archetype(sizes, hashes);
        }

        std::byte* getComponent(const size_t hash, const size_t rowIndex)
        {
            const size_t index = _componentHashMap.at(hash);
            const size_t size = _componentSizes[index];
            std::byte* ptr = _componentRows[index].data();
            return ptr + size * rowIndex;
        }

        std::vector<std::byte*> getRow(const size_t rowIndex)
        {
            std::vector<std::byte*> result;
            result.reserve(_componentSizes.size());
            for (size_t i = 0; i < _componentSizes.size(); i++)
            {
                auto component = getComponent(_componentHashes[i], rowIndex);
                result.push_back(component);
            }
            return result;
        }

        // hashes' indices correspond to the components' indices
        void add(const std::vector<std::span<std::byte>>& components, const std::vector<size_t>& hashes)
        {
            // per component (not per row)
            for (size_t i = 0; i < hashes.size(); i++)
            {
                const size_t hash = hashes[i];
                const auto& addingRows = components[i];

                const size_t index = _componentHashMap.at(hash);
                const size_t size = _componentSizes[index];
                auto& rows = _componentRows[index];

                rows.reserve(rows.size() + addingRows.size());
                rows.insert(rows.end(), addingRows.begin(), addingRows.end());
            }
        }

        void markForRemoval(const size_t rowIndex)
        {
            auto it = std::lower_bound(_toRemove.begin(), _toRemove.end(), rowIndex);
            _toRemove.insert(it, rowIndex);
        }

        void flushMarks() { flushRemoves_(); }

        size_t getRowsCount() const
        {
            return _componentRows[0].size() / _componentSizes[0];
        }

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
                    std::vector<std::byte>& rows = _componentRows[j];
                    const size_t size = _componentSizes[j];
                    // swap index with the last
                    std::swap_ranges(
                        /* first1 */  rows.begin() + deleteIndex * size,
                        /* last1 */  rows.begin() + deleteIndex * size + size,
                        /* first2 */ rows.end() - size);
                    rows.resize(rows.size() - size);
                }
            }

            _toRemove.clear();
        }

        static std::unordered_map<size_t, size_t> createComponentHashMap_(const std::vector<size_t>& hashes)
        {
            std::unordered_map<size_t, size_t> result;
            result.reserve(hashes.size());
            for (size_t i = 0; i < hashes.size(); i++)
                result[hashes[i]] = i;
            return result;
        }

        template<typename T>
        static constexpr std::vector<T> appendTwoVectors_(const std::vector<T>& vec1, const std::vector<T>& vec2)
        {
            std::vector<T> result;
            result.reserve(vec1.size() + vec2.size());
            result.insert(result.end(), vec1.begin(), vec1.end());
            result.insert(result.end(), vec1.begin(), vec1.end());
            return result;
        }
    };

    struct World
    {
        // exact archetype hash to archetype map
        std::unordered_map<size_t, Archetype> _archetypes;

        // cached archetypes for a components' hash search
        std::unordered_map<size_t, std::vector<Archetype*>> _includeArchetypesCache;

        size_t _ver = 0;

        template<typename... Ts>
        Entity addEntity(const Ts... components)
        {
            // find archetype
            auto [hashes, sizes] = createSortedHashesAndSizes_<Ts...>();
            auto& archetype = getOrCreateArchetype_(hashes, sizes);

            // prepare components as byte vectors
            const std::vector<size_t> unsortedHashes = { getTypeHash_<Ts>()... };
            std::vector<std::span<std::byte>> componentsAsbytes;
            componentsAsbytes.reserve(sizeof...(Ts));
            auto callback = [&componentsAsbytes](auto& val)
                {
                    std::byte* ptr = (std::byte*)&val;
                    std::span<std::byte> bytes(ptr, sizeof(decltype(val)));
                    componentsAsbytes.push_back(std::move(bytes));
                };
            (..., callback(components));

            archetype.add(componentsAsbytes, unsortedHashes);
            return Entity
            {
                .rowIndex = archetype.getRowsCount() - 1,
                .archetypeHash = archetype.hash,
                .worldVer = _ver
            };
        }

        void markEntityForRemoval(const Entity& entity)
        {
            if (entity.worldVer != _ver)
            {
                std::cerr << "version mismatch: " << entity.worldVer << " expected " << _ver << std::endl;
                abort();
            }
            auto& archetype = _archetypes.at(entity.archetypeHash);
            archetype.markForRemoval(entity.rowIndex);
        }

        void flushMarks()
        {
            for (auto& [_, archetype] : _archetypes)
                archetype.flushMarks();
        }

        template<typename T>
        T& getComponent(const Entity& entity)
        {
            if (entity.worldVer != _ver)
            {
                std::cerr << "version mismatch: " << entity.worldVer << " expected " << _ver << std::endl;
                abort();
            }
            auto& archetype = _archetypes[entity.archetypeHash];
            const auto hash = getTypeHash_<T>();
            auto* asByte = archetype.getComponent(hash, entity.rowIndex);
            return *(T*)asByte;
        }

        template<typename Func>
        constexpr void executeParallel(Func&& func)
        {
            using traits = FunctionTraits<std::decay_t<Func>>;
            constexpr size_t argsCount = traits::argsCount;
            using firstType = traits::template arg<0>;
            if constexpr (std::is_same_v<std::decay_t<firstType>, Entity>)
                executeWithEntity_<true>(std::forward<Func>(func), std::make_index_sequence<argsCount - 1>{});
            else
                execute_<true>(std::forward<Func>(func), std::make_index_sequence<argsCount>{});
        }

        template<typename Func>
        constexpr void execute(Func&& func)
        {
            using traits = FunctionTraits<std::decay_t<Func>>;
            constexpr size_t argsCount = traits::argsCount;
            using firstType = traits::template arg<0>;
            if constexpr (std::is_same_v<std::decay_t<firstType>, Entity>)
                executeWithEntity_<false>(std::forward<Func>(func), std::make_index_sequence<argsCount - 1>{});
            else
                execute_<false>(std::forward<Func>(func), std::make_index_sequence<argsCount>{});
        }

        template<bool Parallel, typename Func, size_t... Indices>
        void executeWithEntity_(Func&& func, std::index_sequence<Indices...>)
        {
            using traits = FunctionTraits<std::decay_t<Func>>;
            using args = typename traits::args;
            auto [hashes, sizes] = createSortedHashesAndSizes_<std::decay_t<typename traits::template arg<Indices + 1>>...>();
            std::vector<Archetype*> archetypes = findArchetypesWithHashes(hashes);
            for (size_t i = 0; i < archetypes.size(); i++)
            {
                Archetype& archetype = *archetypes[i];
                // get internal component arrays
                void* ptrs[sizeof...(Indices)]
                {
                    (void*)archetype.getComponent(getTypeHash_<std::decay_t<typename traits::template arg<Indices + 1>>>(), 0)...
                };

                if constexpr (Parallel)
#pragma omp parallel for
                    for (signed long long j = 0; j < archetype.getRowsCount(); j++)
                    {
                        Entity entity{ j, archetype.hash, _ver };
                        std::invoke(
                            std::forward<Func>(func),
                            entity,
                            // take indices from internal component arrays
                            ((std::decay_t<typename traits::template arg<Indices + 1>>*)ptrs[Indices])[j]...
                        );
                    }
                else
                    for (size_t j = 0; j < archetype.getRowsCount(); j++)
                    {
                        Entity entity{ j, archetype.hash, _ver };
                        std::invoke(
                            std::forward<Func>(func),
                            entity,
                            // take indices from internal component arrays
                            ((std::decay_t<typename traits::template arg<Indices + 1>>*)ptrs[Indices])[j]...
                        );
                    }
            }
        }

        template<bool Parallel, typename Func, size_t... Indices>
        void execute_(Func&& func, std::index_sequence<Indices...>)
        {
            using traits = FunctionTraits<std::decay_t<Func>>;
            using args = typename traits::args;
            auto [hashes, sizes] = createSortedHashesAndSizes_<std::decay_t<typename traits::template arg<Indices>>...>();
            std::vector<Archetype*> archetypes = findArchetypesWithHashes(hashes);
            for (size_t i = 0; i < archetypes.size(); i++)
            {
                Archetype& archetype = *archetypes[i];
                // get internal component arrays
                void* ptrs[sizeof...(Indices)]
                {
                    (void*)archetype.getComponent(getTypeHash_<std::decay_t<typename traits::template arg<Indices>>>(), 0)...
                };

                if constexpr (Parallel)
#pragma omp parallel for
                    for (signed long long j = 0; j < archetype.getRowsCount(); j++)
                    {
                        std::invoke(
                            std::forward<Func>(func),
                            // take indices from internal component arrays
                            ((std::decay_t<typename traits::template arg<Indices>>*)ptrs[Indices])[j]...
                        );
                    }
                else
                    for (size_t j = 0; j < archetype.getRowsCount(); j++)
                    {
                        std::invoke(
                            std::forward<Func>(func),
                            // take indices from internal component arrays
                            ((std::decay_t<typename traits::template arg<Indices>>*)ptrs[Indices])[j]...
                        );
                    }
            }
        }

        size_t getTotalEntityCount() const
        {
            size_t r = 0;
            for (auto& it : _archetypes)
            {
                auto& archetype = it.second;
                r += archetype.getRowsCount();
            }
            return r;
        }

        size_t getTotalArchetypesCount() const { return _archetypes.size(); }

        Archetype& getOrCreateArchetype_(const std::vector<size_t>& hashes, const std::vector<size_t>& sizes)
        {
            const size_t hash = getHash_(hashes);
            const auto& it = _archetypes.find(hash);
            if (it != _archetypes.end())
                return it->second;

            // create new archetype
            const auto& insertion = _archetypes.insert({ hash, Archetype(sizes, hashes) });
            auto& archetype = insertion.first->second;

            // add to hash caches
            for (auto& [hash, archetypes] : _includeArchetypesCache)
            {
                const auto& componentHashes = _hashToSubHash.at(hash);

                // check if this archetype belongs to this set
                if (hashCollides_(componentHashes, hashes))
                    archetypes.push_back(&archetype);
            }
            return archetype;
        }

        std::vector<Archetype*> findArchetypesWithHashes(const std::vector<size_t> hashes)
        {
            const size_t hash = getHash_(hashes);
            const auto& it = _includeArchetypesCache.find(hash);
            if (it != _includeArchetypesCache.end())
                return it->second;

            // create
            const auto& newIt = _includeArchetypesCache.insert({ hash, {} }).first;
            auto& archetypesList = newIt->second;
            for (auto& [_, archetype] : _archetypes)
                if (hashCollides_(hashes, archetype._componentHashes))
                    archetypesList.push_back(&archetype);
            return archetypesList;
        }
    };
}