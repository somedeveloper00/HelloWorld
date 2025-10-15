#pragma once

#if TRACY_ENABLE
#include <common/TracySystem.hpp>
#endif

#include <string>

namespace engine
{
// provides a centralized place for adding info to threads
struct threadInfo final
{
    threadInfo() = delete;

    static inline void setName(const char *name)
    {
        s_threadName = name;
        s_threadNameString = name;
#if TRACY_ENABLE
        tracy::SetThreadName(name);
#endif
    }

    static inline char const *getName()
    {
        return s_threadName;
    }

    static inline std::string getNameAsString()
    {
        return s_threadNameString;
    }

  private:
    thread_local static inline char const *s_threadName = "";
    thread_local static inline std::string s_threadNameString = "";
};
} // namespace engine