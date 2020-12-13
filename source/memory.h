#pragma once

#include "logger.h"
#include <stdint.h>
#include <stdio.h>
#include <cstring>

struct mpMemoryRegion
{
    mpMemoryRegion *next;
    uint8_t *data;
    size_t dataSize;
    size_t regionSize;
};

struct mpMemoryPool
{
    void *start;
    mpMemoryRegion *head;
    size_t size;
    uint32_t regionCount;
};

inline mpMemoryPool mpCreateMemoryPool(uint32_t regionCount, size_t regionSize)
{
    mpMemoryPool pool = {};
    pool.regionCount = regionCount;
    pool.size = regionCount * (regionSize + sizeof(mpMemoryRegion));
    pool.start = malloc(pool.size);
    pool.head = static_cast<mpMemoryRegion*>(pool.start);
    memset(pool.head, 0, pool.size);

    // Prepare free list
    uint8_t *poolStart = reinterpret_cast<uint8_t*>(pool.head);
    for(uint32_t i = 0; i < regionCount; i++)
    {
        size_t regionStride = regionSize + sizeof(mpMemoryRegion);
        size_t stride = i * regionStride;
        mpMemoryRegion *region = reinterpret_cast<mpMemoryRegion*>(poolStart + stride);
        region->data = poolStart + stride + sizeof(mpMemoryRegion);
        region->regionSize = regionSize;
        if(i < (regionCount - 1))
            region->next = reinterpret_cast<mpMemoryRegion*>(poolStart + ((i + 1) * regionStride));
    }
    return pool;
}

inline mpMemoryRegion* mpGetMemoryRegion(mpMemoryPool *pool)
{
    mpMemoryRegion *region = pool->head;
    if(region == nullptr)
    {
        MP_LOG_ERROR
        puts("MEMORY ERROR: mpGetMemoryRegion failed: pool out of free regions");
        MP_LOG_RESET
        return nullptr;
    }
    pool->head = pool->head->next;

    return region;
}

inline void* mpAllocateIntoRegion(mpMemoryRegion *region, size_t allocationSize)
{
    void *data = region->data;
    region->data += allocationSize;
    region->dataSize += allocationSize;
    if(region->dataSize > region->regionSize)
    {
        MP_LOG_ERROR
        puts("MEMORY ERROR: mpAllocateIntoRegion failed: region is out of memory");
        MP_LOG_RESET
        return nullptr;
    }

    return data;
}

inline void mpFreeMemoryRegion(mpMemoryPool *pool, mpMemoryRegion *region)
{
    if(region == nullptr)
    {
        MP_LOG_ERROR
        puts("MEMORY ERROR: mpFreeMemoryRegion failed: region is null");
        MP_LOG_RESET
        return;
    }
    region->next = pool->head;
    pool->head = region;
}

inline void mpDestroyMemoryPool(mpMemoryPool *pool)
{
    free(pool->start);
    memset(pool, 0, sizeof(mpMemoryPool));
}