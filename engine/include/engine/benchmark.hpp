#pragma once

#include <ittnotify.h>
#include <tracy/Tracy.hpp>

namespace engine
{
struct application;
struct benchmark final
{
    friend application;

    benchmark(__itt_string_handle *name)
    {
        __itt_task_begin(s_vtuneDomain, __itt_null, __itt_null, name);
    }

    benchmark(const benchmark &) = delete;
    benchmark(benchmark &&) = delete;
    benchmark operator=(const benchmark &) = delete;
    benchmark operator=(benchmark &&) = delete;

    ~benchmark()
    {
        __itt_task_end(s_vtuneDomain);
    }

    static inline auto *s_vtuneDomain = __itt_domain_create("application");
};
} // namespace engine

// perform benchmark on the current context
#ifndef DEPLOY
#ifdef TRACY_ENABLE
#define bench(label) ZoneScopedN(label);
#else
#define bench(label)                                                         \
    static auto *s_vtuneLabel##__LINE__ = __itt_string_handle_create(label); \
    engine::benchmark bench##__LINE__                                        \
    {                                                                        \
        s_vtuneLabel##__LINE__                                               \
    }
#endif
#else
#define bench(label)
#endif
