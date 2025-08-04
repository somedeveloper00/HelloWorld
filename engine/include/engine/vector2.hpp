#pragma once
#include <concepts>
#include <type_traits>

namespace engine
{
    template <typename T>
        requires std::is_arithmetic_v<T>
    struct vector2 final
    {
        T x;
        T y;

        constexpr vector2() : x(static_cast<T>(0)), y(static_cast<T>(0)) {}
        constexpr vector2(T x, T y) : x(x), y(y) {}

        constexpr vector2 operator+(const vector2& other) const
        {
            return vector2(x + other.x, y + other.y);
        }

        constexpr vector2 operator-(const vector2& other) const
        {
            return vector2(x - other.x, y - other.y);
        }

        constexpr vector2 operator*(T scalar) const
        {
            return vector2(x * scalar, y * scalar);
        }

        constexpr vector2 operator/(T scalar) const
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                // For floating point, allow division by zero (will result in inf/nan)
                return vector2(x / scalar, y / scalar);
            }
            else
            {
                static_assert(!std::is_same_v<T, T> || std::is_floating_point_v<T>, "Division by zero for integral types is undefined.");
                return vector2(x / scalar, y / scalar);
            }
        }

        constexpr bool operator==(const vector2& other) const
        {
            return x == other.x && y == other.y;
        }

        constexpr bool operator!=(const vector2& other) const
        {
            return !(*this == other);
        }
    };
}