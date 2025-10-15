#pragma once

#include <cstdlib>
#include <tracy/Tracy.hpp>

namespace engine
{
enum allocType
{
    // normal malloc. bits are random and unpredictable
    mallocAllocType,
    // c++'s standard new
    newAllocType,
    // normal calloc. bits are set to zero
    callocAllocType
};

// simple allocator that tracks memory allocations with tracy and uses customizable allocation method (configurable at compile time with AllocType templated parameter)
template <typename T, allocType AllocType = allocType::mallocAllocType>
struct rawAlloc final
{
    static inline const char *name = typeid(T).name();

    rawAlloc() = delete;

    static inline T *allocate(const size_t count)
    {
        T *ptr;
        if constexpr (AllocType == allocType::mallocAllocType)
            ptr = static_cast<T *>(malloc(sizeof(T) * count));
        else if constexpr (AllocType == allocType::newAllocType)
            ptr = new T[count];
        else
            ptr = static_cast<T *>(calloc(count, sizeof(T)));
        TracyAlloc(ptr, sizeof(T) * count);
        return ptr;
    }

    static inline void deallocate(T *ptr, const size_t count)
    {
        if (ptr == nullptr)
            return;
        TracyFree(ptr);
        free(ptr);
    }
};
} // namespace engine