#pragma once

#include "logFormats/glmLogFormats.hpp" // for glm formats
#include <cstdio>
#include <format>
#include <stdio.h>
#include <string>

#ifdef _WIN32
#include <corecrt_io.h>
#include <windows.h>
#else
#include <stdio.h>
#include <unistd.h>
#endif

namespace engine
{
class log final
{
  public:
    using logHandler = void (*)(const std::string &);

    // current info log handler in use
    static inline logHandler infoLogHandle = nullptr;
    // current info log handler in use
    static inline logHandler warningLogHandle = nullptr;
    // current info log handler in use
    static inline logHandler errorLogHandle = nullptr;

    template <typename... Args>
    static inline void logInfo(const std::string &formatStr, Args &&...args)
    {
        infoLogHandle("[ENGINE INFO] " + std::vformat(formatStr, std::make_format_args(args...)));
    }

    template <typename... Args>
    static inline void logWarning(const std::string &formatStr, Args &&...args)
    {
        warningLogHandle("[ENGINE WARNING] " + std::vformat(formatStr, std::make_format_args(args...)));
    }

    template <typename... Args>
    static inline void logError(const std::string &formatStr, Args &&...args)
    {
        errorLogHandle("[ENGINE ERROR] " + std::vformat(formatStr, std::make_format_args(args...)));
    }

    static inline void initialize()
    {
        static bool s_initialized = false;
        if (s_initialized)
        {
            logError("log already initialized");
            return;
        }
        infoLogHandle = defaultInfoLogHandle;
        warningLogHandle = defaultWarningLogHandle;
        errorLogHandle = defaultErrorLogHandle;
        s_initialized = true;
    }

    // uses isatty
    static inline bool isConnectedToTerminal()
    {
#ifdef _WIN32
        return _isatty(_fileno(stdout));
#else
        return isatty(fileno(stdout));
#endif
    }

  private:
    static inline auto defaultInfoLogHandle = [](const std::string &msg) {
        fputs("\033[1;32m", stdout);
        fputs(msg.c_str(), stdout);
        fputs("\033[1;0m\n", stdout);
    };
    static inline auto defaultWarningLogHandle = [](const std::string &msg) {
        fputs("\033[1;33m", stdout);
        fputs(msg.c_str(), stdout);
        fputs("\033[1;0m\n", stdout);
    };
    static inline auto defaultErrorLogHandle = [](const std::string &msg) {
        fputs("\033[1;31m", stdout);
        fputs(msg.c_str(), stdout);
        fputs("\033[1;0m\n", stdout);
    };
};
} // namespace engine