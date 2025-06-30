#pragma once
#include <array>

// Helper to select N-th type from Ts...
template<size_t N, typename T, typename... Rest>
struct TypeAtHelper : TypeAtHelper<N - 1, Rest...> {};
template<typename T, typename... Rest>
struct TypeAtHelper<0, T, Rest...> { using type = T; };
template<size_t N, typename... Ts>
using TypeAt = TypeAtHelper<N, Ts...>::type;

template<typename...Ts>
consteval size_t getVariadicCount() { return sizeof...(Ts); }

template<size_t...Ts>
consteval size_t getVariadicCount() { return sizeof...(Ts); }

template<typename F>
struct FunctionTraits;

// free/function pointer
template<typename R, typename... Args>
struct FunctionTraits<R(*)(Args...)>
{
    using returnType = R;

    static constexpr size_t argsCount = getVariadicCount<Args...>();

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

consteval size_t _pow2(size_t a)
{
    size_t r = 1;
    while (a-- > 0)
        r *= 2;
    return r;
}

template<size_t N>
consteval void _createCombinations(
    std::array<std::array<bool, N>, _pow2(N) - 1>& result,
    int& rind,
    int point,
    std::array<bool, N>& current)
{
    if (point == current.size())
        return;

    while (point < current.size())
    {
        current[point] = true;

        // take snapshot
        result[rind++] = current;

        _createCombinations(result, rind, point + 1, current);
        current[point] = false;
        point++;
    }

}

template<size_t N>
consteval std::array<std::array<bool, N>, _pow2(N) - 1> createCombinations()
{
    std::array<std::array<bool, N>, _pow2(N) - 1> result{};
    int rind = 0;
    int point = 0;
    std::array<bool, N> current{};
    _createCombinations(result, rind, point, current);
    return result;
}

namespace _typeHashHelper
{
    consteval size_t fnv1a_64(const char* s, size_t count) {
        size_t hash = 0xcbf29ce484222325ULL;
        for (size_t i = 0; i < count; ++i) {
            hash ^= static_cast<size_t>(s[i]);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    template <typename T>
    consteval size_t type_hash()
    {
#if defined(__clang__) || defined(__GNUC__)
        constexpr auto& sig = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
        constexpr auto& sig = __FUNCSIG__;
#else
#error "Unsupported compiler for compile-time type hash"
#endif
        // compute FNV-1a over the full signature
        return fnv1a_64(sig, sizeof(sig) / sizeof(char));
    }
};

template<typename T>
consteval size_t getTypeHash()
{
    return _typeHashHelper::type_hash<T>();
}

template<size_t N>
consteval std::array<size_t, _pow2(N) - 1> createHashForAllCombinationsOfHashes(const std::array<size_t, N>& hashes)
{
    constexpr auto combinations = createCombinations<N>();
    std::array<size_t, _pow2(N) - 1> r{};
    for (size_t i = 0; i < combinations.size(); i++)
    {
        // count size
        size_t size = 0;
        for (size_t j = 0; j < hashes.size(); j++)
            if (combinations[i][j])
                size++;

        // create hash
        size_t hash = size;
        for (size_t j = 0; j < hashes.size(); j++)
            if (combinations[i][j])
                hash ^= hashes[j] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        r[i] = hash;
    }
    return r;
}

template<typename... Ts>
consteval size_t createSortedHash()
{
    auto hashes = createSortedHashes<Ts...>();
    size_t hash = hashes.size();
    for (auto& i : hashes)
        hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

template<typename... Ts>
consteval std::array<size_t, getVariadicCount<Ts...>()> createSortedHashes()
{
    constexpr size_t count = getVariadicCount<Ts...>();
    std::array<size_t, getVariadicCount<Ts...>()> hashes{ getTypeHash<Ts>()... };
    //bubble sort
    for (size_t i = 0; i < count - 1; i++)
        for (size_t j = i + 1; j < count; j++)
            if (hashes[j] > hashes[i])
            {
                auto tmp = hashes[j];
                hashes[j] = hashes[i];
                hashes[i] = tmp;
            }
    return hashes;
}

template<typename... Ts>
consteval std::array<size_t, getVariadicCount<Ts...>()> createSortedSizes()
{
    constexpr size_t count = getVariadicCount<Ts...>();
    std::array<size_t, getVariadicCount<Ts...>()> hashes{ getTypeHash<Ts>()... };
    std::array<size_t, getVariadicCount<Ts...>()> sizes{ sizeof(Ts)... };
    //bubble sort
    for (size_t i = 0; i < count - 1; i++)
        for (size_t j = i + 1; j < count; j++)
            if (hashes[j] > hashes[i])
            {
                auto tmp = hashes[j];
                hashes[j] = hashes[i];
                hashes[i] = tmp;
                auto tmp1 = sizes[j];
                sizes[j] = sizes[i];
                sizes[i] = tmp1;
            }
    return sizes;
}
