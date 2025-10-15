#pragma once

#include <tuple>

namespace functionTraits
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
} // namespace functionTraints
