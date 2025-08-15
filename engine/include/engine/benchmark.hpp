#pragma once

#include "log.hpp"
#include <chrono>

namespace engine
{
struct benchmark final
{
    benchmark(const std::string &label) : label(label), _startTime(std::chrono::high_resolution_clock::now())
    {
    }
    benchmark(std::string &&label) : label(std::move(label)), _startTime(std::chrono::high_resolution_clock::now())
    {
    }

    benchmark(const benchmark &) = delete;
    benchmark(benchmark &&) = delete;
    benchmark operator=(const benchmark &) = delete;
    benchmark operator=(benchmark &&) = delete;

    ~benchmark()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<float, std::milli>(now - _startTime).count();
        engine::log::logInfo(Format, elapsed, label);
    }

  private:
    static constexpr auto Format = "[benchmark] > ellapsed(ms): {}\t\"{}\"";
    const std::string label;
    const std::chrono::high_resolution_clock::time_point _startTime;
};
} // namespace engine

#define __benchmark_mix_paste_(a, b) a##b
#define __benchmark_mix_(a, b) __benchmark_mix_paste_(a, b)

// perform benchmark on the current context (use this if you want to use a custom label)
#define bench(label) engine::benchmark __benchmark_mix_(bench_, __LINE__){std::string(__FILE__) + ":" + label};
