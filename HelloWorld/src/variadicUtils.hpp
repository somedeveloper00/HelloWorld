#pragma once

// Helper to select N-th type from Ts...
template<size_t N, typename T, typename... Rest>
struct TypeAtHelper : TypeAtHelper<N - 1, Rest...> {};
template<typename T, typename... Rest>
struct TypeAtHelper<0, T, Rest...> { using type = T; };
template<size_t N, typename... Ts>
using TypeAt = TypeAtHelper<N, Ts...>::type;

template<typename...Ts>
constexpr size_t getVariadicCount()
{
    return sizeof...(Ts);
}

template<typename F>
struct FunctionTraits;

// free/function pointer
template<typename R, typename... Args>
struct FunctionTraits<R(*)(Args...)>
{
    using returnType = R;

    static constexpr size_t argsCount = getVariadicCount<Args...>();

    template<std::size_t N>
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

//// std::function
//template<typename R, typename... Args>
//struct FunctionTraits<std::function<R(Args...)>> : FunctionTraits<R(*)(Args...)> {};


template<typename T>
constexpr size_t getTypeHash()
{
    return typeid(T).hash_code();
}

template<typename... Ts>
constexpr size_t createSortedHash()
{
    auto hashes = createSortedHashes<Ts...>();
    size_t hash = hashes.size();
    for (auto& i : hashes)
        hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

template<typename... Ts>
constexpr std::vector<size_t> createSortedHashes()
{
    constexpr size_t count = getVariadicCount<Ts...>();
    std::vector<size_t> hashes{ getTypeHash<Ts>()... };
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
constexpr std::vector<size_t> createSortedSizes()
{
    constexpr size_t count = getVariadicCount<Ts...>();
    std::vector<size_t> hashes{ getTypeHash<Ts>()... };
    std::vector<size_t> sizes{ sizeof(Ts)... };
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

template<typename T>
constexpr size_t getInt(const int& i) { return i; }