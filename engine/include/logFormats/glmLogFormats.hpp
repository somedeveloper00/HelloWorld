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
            ctx.out(), "[[{}, {}, {}, {}] [{}, {}, {}, {}] [{}, {}, {}, {}] [{}, {}, {}, {}]]",
            ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]);
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
} // namespace std