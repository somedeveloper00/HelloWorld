#pragma once

#include <cassert>
#include <format>
#include <ostream>
#include <string>

namespace engine
{
struct color final
{
    float r, g, b, a;
    color(float r, float g, float b, float a = 1.f) : r(r), g(g), b(b), a(a)
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