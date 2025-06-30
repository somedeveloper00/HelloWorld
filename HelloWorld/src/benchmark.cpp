#pragma once
#include <chrono>
#include <iostream>

struct Benchmark
{
    Benchmark(const char* label) : _startTime(std::chrono::high_resolution_clock::now()), label(label)
    {
    }

    ~Benchmark()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto ellapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - _startTime).count();
        std::cout << "[benchmark] >" << label << " ellapsed(ms): " << (double)ellapsed / 1000000 << std::endl;
    }

private:
    const char* label;
    const std::chrono::high_resolution_clock::time_point _startTime;
};