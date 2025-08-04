#pragma once
#include "engine/log.hpp"
#include <chrono>

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
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - _startTime).count();
        engine::logInfo(Format, elapsed / 1000.f / 1000.f, label);
    }

  private:
    static constexpr auto Format = "[benchmark] > ellapsed(ms): {}\t\"{}\"";
    const std::string label;
    const std::chrono::high_resolution_clock::time_point _startTime;
};