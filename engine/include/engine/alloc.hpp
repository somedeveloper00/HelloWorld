#pragma once

#include <tracy/Tracy.hpp>

namespace engine
{
// simple allocator that tracks memory allocations with tracy
template <typename T>
struct alloc final
{
    static inline const char *name = typeid(T).name();

    alloc() = delete;

    static inline T *allocate(const size_t count)
    {
        T *ptr = static_cast<T *>(malloc(sizeof(T) * count));
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