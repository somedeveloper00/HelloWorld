#pragma once

namespace engine
{
// conditionally includes a variable
template <typename T, T Init, bool Condition>
struct ConditionalVariable;

template <typename T, T Init>
struct ConditionalVariable<T, Init, false>
{
};

template <typename T, T Init>
struct ConditionalVariable<T, Init, true>
{
    T value = Init;
};
} // namespace engine