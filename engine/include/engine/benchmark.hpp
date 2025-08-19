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

    benchmark(const char *label)
        : _label(label), _indent(s_indent++), _startTime(std::chrono::system_clock::now())
    {
    }

    benchmark(const benchmark &) = delete;
    benchmark(benchmark &&) = delete;
    benchmark operator=(const benchmark &) = delete;
    benchmark operator=(benchmark &&) = delete;

    ~benchmark()
    {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration<size_t, std::nano>(now - _startTime).count();

        s_frames[s_framesIndex].scopes.emplace_back(scope{const_cast<char *>(_label), _indent, elapsed});

        if (--s_indent == 0)
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
            s_frames[s_framesIndex].scopes.clear();
        }
    }

  private:
    struct scope
    {
        const char *name;
        const unsigned char indent;
        const size_t time;
    };
    struct frame
    {
        size_t frameNumber;
        std::vector<scope> scopes;
    };
    struct merged_scope
    {
        std::string name;
        unsigned char indent;
        size_t total_time;
        size_t count;
    };
    constexpr static size_t s_maxFrames = 100;
    static inline frame s_frames[s_maxFrames];
    static inline size_t s_framesIndex = 0;
    static inline bool s_framesRounded = false;
    static inline size_t s_totalFrames = 0;
    static inline unsigned char s_indent = 0;

    const unsigned char _indent;
    const char *_label;
    const std::chrono::system_clock::time_point _startTime;

    static inline void printResultsToFile_()
    {
#if !BENCHMARK
        return;
#endif
        log::logInfo("writing benchmark results to file");
        std::filesystem::create_directory(".bench/");
        const auto path = ".bench/dump_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".txt";
        std::ofstream file(path, std::ios::app);
        if (!file)
        {
            log::logError("[benchmark] could not open file for append \"{}\"", path);
            return;
        }

        if (s_framesRounded)
        {
            std::rotate(s_frames, &s_frames[s_framesIndex], &s_frames[s_maxFrames]);
            s_framesIndex = s_maxFrames;
        }
        for (size_t i = 0; i < s_framesIndex; ++i)
        {
            file << "frame " << s_frames[i].frameNumber << ":\n";

            // collapse repeated scopes (contiguous repeats) into merged_scope entries
            std::vector<merged_scope> merged{};
            for (auto &sc : s_frames[i].scopes | std::ranges::views::reverse)
            {
                for (auto &mscope : merged | std::ranges::views::reverse)
                {
                    if (mscope.name == sc.name && mscope.indent == sc.indent)
                    {
                        mscope.total_time += sc.time;
                        ++mscope.count;
                        goto nextScope;
                    }
                }
                merged.push_back(merged_scope{std::string(sc.name), sc.indent, sc.time, 1});
            nextScope:;
            }

            for (auto &scope : merged)
            {
                // append to file
                const auto str = std::format("    {}\"{}\": {} (x{})\n", std::string(scope.indent * 4, ' '), scope.name, static_cast<double>(scope.total_time) / 1000. / 1000., scope.count);
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
