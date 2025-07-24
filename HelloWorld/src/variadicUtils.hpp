/*
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

consteval size_t _pow2(size_t a)
{
    size_t r = 1;
    while (a-- > 0)
        r *= 2;
    return r;
}

namespace _typeHashHelper
{
    consteval size_t fnv1a_64(const char* s, size_t count)
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
*/
