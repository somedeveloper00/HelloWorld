#pragma once
#include <string>
#include <functional>
#include <format>
#include <mutex>

namespace engine
{
    using logHandler = std::function<void(const std::string&)>;

    // internal log handler storage
    inline logHandler& getLogHandler()
    {
        static logHandler handler = [](const std::string& msg)
            {
                puts(msg.c_str());
            };
        return handler;
    }

    // set a custom log handler
    inline void setLogHandler(logHandler customHandler)
    {
        getLogHandler() = std::move(customHandler);
    }

    template<typename... Args>
    inline void log(const std::string& formatStr, Args&&... args)
    {
        const std::string msg = std::vformat(formatStr, std::make_format_args(args...));
        getLogHandler()(msg);
    }
    template<typename... Args>
    inline void logInfo(const std::string& formatStr, Args&&... args)
    {
        log("[ENGINE INFO] " + formatStr, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void logWarning(const std::string& formatStr, Args&&... args)
    {
        log("[ENGINE WARNING] " + formatStr, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void logError(const std::string& formatStr, Args&&... args)
    {
        log("[ENGINE ERROR] " + formatStr, std::forward<Args>(args)...);
    }
}