#pragma once

#include <cassert>
#include <cmath>
#include <format>
#include <ostream>
#include <string>

namespace engine
{
// represents an RGBA (0-1) color
struct color final
{
    float r, g, b, a;

    color() = default;

    color(float r, float g, float b, float a = 1.f)
        : r(r), g(g), b(b), a(a)
    {
    }

    friend std::ostream &operator<<(std::ostream &os, const color &c)
    {
        os << "(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ")";
        return os;
    }

    operator std::string() const
    {
        return std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b) + " " + std::to_string(a);
    }

    // lerps fast with SIMD and FMA
    void lerp(const color other, const float t) noexcept
    {
#pragma omp simd
        for (size_t i = 0; i < 4; i++)
            (&r)[i] = fma((&other.r)[i], t, fma(-t, (&r)[i], (&r)[i]));
    }

    // lerps fast with SIMD and FMA
    static inline color lerp(const color a, const color b, const float t) noexcept
    {
        color c = a;
        c.lerp(b, t);
        return c;
    }
};

} // namespace engine

namespace std
{
template <>
struct formatter<engine::color>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const engine::color &c, FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", c.r, c.g, c.b, c.a);
    }
};
} // namespace std