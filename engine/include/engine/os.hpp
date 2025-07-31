#pragma once

#ifdef WIN32
#include <Windows.h>
#include <Psapi.h>
#else
#error "Platform not supported"
#endif

namespace engine
{
    namespace os
    {
        // windows: gets working set size
        static inline size_t getTotalMemory()
        {
#ifdef WIN32
            PROCESS_MEMORY_COUNTERS pmc;
            HANDLE process = GetCurrentProcess();
            if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc)))
                return pmc.PagefileUsage;
            return -1;
#endif
        }
    }
}