#pragma once

#define BENCHMARK 1

#include "log.hpp"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <numeric>
#include <ranges>
#include <ratio>
#include <stdio.h>
#include <string>

namespace engine
{
struct application;
struct benchmark final
{
    friend application;

    benchmark(const char *name)
        : _name(name), _indent(s_lastAddedIndent = s_indent++), _startTime(std::chrono::high_resolution_clock::now())
    {
    }

    benchmark(const benchmark &) = delete;
    benchmark(benchmark &&) = delete;
    benchmark operator=(const benchmark &) = delete;
    benchmark operator=(benchmark &&) = delete;

    ~benchmark()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::nanoseconds(now - _startTime);
        size_t waste;
        size_t elapsed;
        if (--s_indent == s_lastAddedIndent)
        {
            // had no child benchmarks
            elapsed = diff.count();
            waste = 0;
        }
        else
        {
            // had child benchmarks
            waste = 0;
            for (size_t i = s_indent + 1; i < s_benchmarkWastes.size(); i++)
            {
                waste += s_benchmarkWastes[i].count();
                new (&s_benchmarkWastes[i]) std::chrono::nanoseconds(0);
            }
            elapsed = diff.count() - 2 * waste;
        }

        // append to scopes list
        auto &frameRef = s_frames[s_framesIndex];
        if (frameRef.scopesCount >= frame::scopesSize)
            log::logError("[benchmark] scope overflow in frame {}. increase engine::benchmark::frame::scopesSize", frameRef.frameNumber);
        else
            new (&frameRef.scopes[frameRef.scopesCount++]) scope(_name, _indent, elapsed, waste);
        // go to next frame
        if (_indent == 0)
        {
            s_frames[s_framesIndex].frameNumber = s_totalFrames++;
            // frame is ended
            // handle frame count limitting
            if (++s_framesIndex >= s_maxFrames)
            {
                // overwrite the oldest scope
                s_framesIndex = 0;
                s_framesRounded = true;
            }
            s_frames[s_framesIndex].scopesCount = 0;
        }

        // append to benchmark waste time list
        if (s_indent != 0)
        {
            s_benchmarkWastes.resize(s_indent + 1);
            s_benchmarkWastes[s_indent] += 2 * (std::chrono::high_resolution_clock::now() - now);
        }
    }

  private:
    struct scope
    {
        const char *name;
        unsigned char indent;
        size_t time;
        size_t inaccuracy;
    };
    struct frame
    {
        constexpr static size_t scopesSize = 1000000;
        size_t frameNumber = 0;
        scope scopes[scopesSize];
        size_t scopesCount = 0;
    };
    struct merged_scope
    {
        const char *name;
        unsigned char indent;
        size_t totalTime;
        size_t totalInaccuracy;
        size_t count;
    };

    // maximum number of frames to include in the report
    constexpr static size_t s_maxFrames = 50;

    // reporting frames
    static inline frame s_frames[s_maxFrames];

    // index of the current reporting frame
    static inline size_t s_framesIndex = 0;

    // whether or not the maximum number of frames has been reached (in which case, we round the index)
    static inline bool s_framesRounded = false;

    // total number of frames
    static inline size_t s_totalFrames = 0;

    // current global indent (parent/child relations)
    static inline unsigned char s_indent = 0;

    static inline unsigned char s_lastAddedIndent;

    // wasted time spent for benchmarking. this is used to correct the reports
    static inline std::vector<std::chrono::nanoseconds> s_benchmarkWastes{};

    const char *_name;
    const unsigned char _indent;
    const std::chrono::high_resolution_clock::time_point _startTime;

    // called when the application is closing
    static inline void printResultsToFile_()
    {
#if !BENCHMARK
        return;
#endif
        const auto path = ".bench/dump_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".txt";
        log::logInfo("writing benchmark results to file \"{}\"", path);
        std::filesystem::create_directory(".bench/");
        std::ofstream file(path, std::ios::app);
        if (!file)
        {
            log::logError("[benchmark] could not open file for append \"{}\"", path);
            return;
        }

        for (size_t i = 0; i < s_framesIndex; ++i)
        {
            file << "frame " << s_frames[i].frameNumber << ":\n";

            // collapse repeated scopes (contiguous repeats) into merged_scope entries
            std::vector<merged_scope> merged{};
            for (size_t j = s_frames[i].scopesCount; j-- > 0;)
            {
                auto &sc = s_frames[i].scopes[j];
                for (auto &mscope : merged | std::ranges::views::reverse)
                {
                    if (mscope.name == sc.name && mscope.indent == sc.indent)
                    {
                        mscope.totalTime += sc.time;
                        mscope.totalInaccuracy += sc.inaccuracy;
                        ++mscope.count;
                        goto nextScope;
                    }
                }
                merged.emplace_back(sc.name, sc.indent, sc.time, sc.inaccuracy, 1);
            nextScope:;
            }

            for (auto &scope : merged)
            {
                // append to file
                const auto str = std::format("    {}\"{}\": {} (x{}) (+-{})\n", std::string(scope.indent * 4, ' '), scope.name, static_cast<double>(scope.totalTime) / 1000. / 1000., scope.count, static_cast<double>(scope.totalInaccuracy) / 1000. / 1000.);
                file << str;
            }
        }
        file.close();
    }
};
} // namespace engine

#if BENCHMARK
// perform benchmark on the current context
#define bench(label)                 \
    engine::benchmark bnch##__LINE__ \
    {                                \
        label                        \
    }
#else
#define bench(label)
#endif
