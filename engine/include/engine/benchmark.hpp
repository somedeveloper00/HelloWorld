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

#ifndef DEPLOY
#ifdef TRACY_ENABLE
// perform benchmark on the current context
#define bench(label) ZoneScopedN(label);
// same as bench, but with runtime string
#define benchDynamic(label, length) \
    bench("不明");                  \
    ZoneName(label, length);
// exeuctes the code inside a hidden scope. useful for oneliner benchmarks
#define benchCode(label, Code) \
    {                          \
        bench(label);          \
        Code;                  \
    };
#else // !TRACY_ENABLE
// perform benchmark on the current context
#define bench(label)                                                         \
    static auto *s_vtuneLabel##__LINE__ = __itt_string_handle_create(label); \
    engine::benchmark bench##__LINE__                                        \
    {                                                                        \
        s_vtuneLabel##__LINE__                                               \
    }
// same as bench, but with runtime string
#define benchDynamic(label, length) bench(label);
// exeuctes the code inside a hidden scope. useful for oneliner benchmarks
#define benchCode(label, Code) \
    {                          \
        bench(label);          \
        Code;                  \
    };
#endif
#else // DEPLOY
// no-op (because `DEPLOY` is true)
#define bench(label) ;
// no-op (because `DEPLOY` is true)
#define benchDynamic(label, length) ;
// no-op (because `DEPLOY` is true)
#define benchCode(label, Code) ;
#endif
