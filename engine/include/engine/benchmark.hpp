#pragma once

#define BENCHMARK 1

#include "log.hpp"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
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
        : _name(name), _indent(s_indent++), _startTime(std::chrono::high_resolution_clock::now())
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
        auto elapsed = (diff).count();

        // append scope
        auto &frameRef = s_frames[s_framesIndex];
        if (frameRef.scopesCount >= frame::scopesSize)
            log::logError("[benchmark] scope overflow in frame {}. increase engine::benchmark::frame::scopesSize", frameRef.frameNumber);
        else
            new (&frameRef.scopes[frameRef.scopesCount++]) scope(_name, _indent, elapsed, s_benchmarkWaste.count());
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

        // handle wasted time
        if (--s_indent == 0)
            new (&s_benchmarkWaste) std::chrono::nanoseconds(0);
        else
            s_benchmarkWaste += std::chrono::high_resolution_clock::now() - now;
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
        constexpr static size_t scopesSize = 300000;
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

    // wasted time spent for benchmarking. this is used to correct the reports
    static inline std::chrono::nanoseconds s_benchmarkWaste{0};

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
                        // if (mscope.totalInaccuracy > 1000000000)
                        //     __debugbreak();
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
