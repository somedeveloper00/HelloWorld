#pragma once
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <format>

namespace std
{
template <>
struct formatter<glm::mat4>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const glm::mat4 &c, FormatContext &ctx) const
    {
        const auto *ptr = glm::value_ptr(c);
        return std::format_to(
            ctx.out(), "[row0: [{}, {}, {}, {}] row1: [{}, {}, {}, {}] row2: [{}, {}, {}, {}] row3: [{}, {}, {}, {}]]",
            ptr[4 * 0], ptr[4 * 1], ptr[4 * 2], ptr[4 * 3],                  // row 0
            ptr[4 * 0 + 1], ptr[4 * 1 + 1], ptr[4 * 2 + 1], ptr[4 * 3 + 1],  // row 1
            ptr[4 * 0 + 2], ptr[4 * 1 + 2], ptr[4 * 2 + 2], ptr[4 * 3 + 2],  // row 2
            ptr[4 * 0 + 3], ptr[4 * 1 + 3], ptr[4 * 2 + 3], ptr[4 * 3 + 3]); // row 3
    }
};
template <>
struct formatter<glm::vec4>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const glm::vec4 &c, FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "[{}, {}, {}, {}]", c.x, c.y, c.z, c.w);
    }
};
template <>
struct formatter<glm::vec3>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const glm::vec3 &c, FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "[{}, {}, {}]", c.x, c.y, c.z);
    }
};
} // namespace std