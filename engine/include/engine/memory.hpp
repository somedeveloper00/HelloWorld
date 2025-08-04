#pragma once

#ifdef WIN32
#include <Psapi.h>
#include <Windows.h>

#else
#error "Platform not supported"
#endif

namespace engine
{
namespace memory
{

// windows: gets page file usage
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

} // namespace memory
} // namespace engine