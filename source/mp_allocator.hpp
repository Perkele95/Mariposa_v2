#pragma once

#include "logger.h"
#include <stdint.h>
#include <stdlib.h>

struct mpAllocator_T
{
    void *data;
    uint32_t size;
    uint32_t maxSize;
};

typedef mpAllocator_T *mpAllocator;

inline mpAllocator mpCreateAllocator(uint32_t size)
{
    mpAllocator allocator = static_cast<mpAllocator>(malloc(size + sizeof(mpAllocator_T)));
    allocator->data = (allocator + 1);
    allocator->maxSize = size;
    allocator->size = 0;
    memset(allocator->data, 0, size);
    return allocator;
}

inline void mpDestroyAllocator(mpAllocator allocator)
{
    free(allocator);
}

inline void mpResetAllocator(mpAllocator allocator)
{
    allocator->data = static_cast<uint8_t*>(allocator->data) - allocator->size;
    memset(allocator->data, 0, allocator->size);
    allocator->size = 0;
}

template<typename T>
inline T *mpAllocate(mpAllocator allocator, uint32_t count)
{
    T *block = nullptr;

    const size_t allocSize = count * sizeof(T);
    if(allocator->size + allocSize > allocator->maxSize){
        MP_PUTS_WARN("ALLOCATOR ERROR: allocation denied: not enough memory left in allocator")
    }
    else{
        block = static_cast<T*>(allocator->data);
        allocator->data = static_cast<uint8_t*>(allocator->data) + allocSize;
        allocator->size += allocSize;
    }
    return block;
}

template<typename T>
inline void mpResizeAllocator(mpAllocator source, uint32_t newCount)
{
    const size_t newSize = count * sizeof(T);
    source->data = realloc(source->data, newSize);
    source->maxSize = newSize;
}

inline void mpResizeAllocator(mpAllocator source, size_t newSize)
{
    source->data = realloc(source->data, newSize);
    source->maxSize = newSize;
}