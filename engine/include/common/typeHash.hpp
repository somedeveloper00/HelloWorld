#pragma once
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