#pragma once

#ifdef _Win32
#include <Windows.h>
#endif

namespace engine
{
static inline void fatalAssert(bool condition, const char *const msg)
{
    if (!condition)
    {
        log::logError(msg);
#ifndef NDEBUG
        assert(false);
#elif _WIN32
        MessageBox(NULL, msg, "Fatal Error", MB_OK | MB_ICONERROR);
        exit(-1);
#endif
    }
}
} // namespace engine
