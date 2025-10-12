#pragma once

#include <array>
#include <span>
#include <string>
#include <type_traits>

// a constexpr sin
template <typename Value, size_t Terms = 10>
consteval Value constSin(Value x)
{
    Value result = 0;
    Value power = x;
    Value factorial = 1;
    int sign = 1;

    for (int n = 0; n < Terms; ++n)
    {
        result += sign * power / factorial;
        sign = -sign;
        int k = 2 * n + 2;
        power *= x * x;
        factorial *= k * (k + 1);
    }
    return result;
}

// a constexpr cosine
template <typename Value, size_t Terms = 10>
consteval Value constCos(Value x)
{
    Value result = 0;
    Value power = 1; // x^0
    Value factorial = 1;
    int sign = 1;

    for (int n = 0; n < Terms; ++n)
    {
        result += sign * power / factorial;
        sign = -sign;
        int k = 2 * n + 1;
        int l = 2 * n + 2;
        power *= x * x;
        factorial *= k * l;
    }
    return result;
}

static constexpr size_t fnv1a_64_(const char *s, size_t count)
{
    size_t hash = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < count; ++i)
    {
        hash ^= static_cast<size_t>(s[i]);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// returns a unique compile-time hash for the given type
template <typename T>
static constexpr size_t getTypeHash_()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr auto &sig = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    constexpr auto &sig = __FUNCSIG__;
#else
#error "Unsupported compiler for compile-time type hash"
#endif
    // compute FNV-1a over the full signature
    return fnv1a_64_(sig, sizeof(sig) / sizeof(char));
}

// apend into an array
template <typename T, size_t N>
constexpr std::array<T, N + 1> append(const std::array<T, N> &other, const T value)
{
    std::array<T, N + 1> result[N + 1]{};
    for (size_t i = 0; i < N; i++)
        result[i] = other[i];
    result[N] = value;
    return result;
}

// insert into an array at index
template <typename T, size_t N>
constexpr std::array<T, N + 1> insert(const std::array<T, N> &other, const T value, const size_t index)
{
    std::array<T, N + 1> result{};
    for (size_t i = index; i < N; i++)
        result[i + 1] = other[i];
    result[index] = value;
    return result;
}

// joins elements into a string
template <typename T, typename Func>
constexpr std::string join(const std::span<T> &span, const std::string seperator, Func &&func)
{
    std::string r;
    for (size_t i = 0; i < span.size() - 1; i++)
        r += func(span[i]) + seperator;
    if (span.size() > 0)
        r += func(span[span.size() - 1]);
    return r;
}

// joins elements into a string
template <typename T>
    requires std::is_constructible_v<std::string, T>
constexpr std::string join(const std::span<T> &span, const std::string seperator)
{
    std::string r;
    for (size_t i = 0; i < span.size() - 1; i++)
        r += span[i] + seperator;
    if (span.size() > 0)
        r += span[span.size() - 1];
    return r;
}

// joins elements into a string
template <typename T, typename Func>
constexpr std::string joinReverse(const std::span<T> &span, const std::string seperator, Func &&func)
{
    std::string r;
    for (size_t i = span.size(); i-- > 1;)
        r += func(span[i]) + seperator;
    if (span.size() > 0)
        r += func(span[0]);
    return r;
}

// joins elements into a string
template <typename T>
    requires std::is_constructible_v<std::string, T>
constexpr std::string joinReverse(const std::span<T> &span, const std::string seperator)
{
    std::string r;
    for (size_t i = span.size(); i-- > 1;)
        r += span[i] + seperator;
    if (span.size() > 0)
        r += span[0];
    return r;
}