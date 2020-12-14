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
    uint32_t poolID;
};

struct mpMemoryPool
{
    void *start;
    mpMemoryRegion *head;
    uint32_t ID;
};
// Creates a memory pool header
inline mpMemoryPool mpCreateMemoryPool(uint32_t regionCount, size_t regionSize, uint32_t uniqueID)
{
    size_t poolSize = regionCount * (regionSize + sizeof(mpMemoryRegion));
    mpMemoryPool pool = {};
    pool.start = malloc(poolSize);
    pool.ID = uniqueID;
    pool.head = static_cast<mpMemoryRegion*>(pool.start);
    memset(pool.head, 0, poolSize);

    // Prepare free list
    uint8_t *poolStart = reinterpret_cast<uint8_t*>(pool.head);
    for(uint32_t i = 0; i < regionCount; i++)
    {
        size_t regionStride = regionSize + sizeof(mpMemoryRegion);
        size_t stride = i * regionStride;
        mpMemoryRegion *region = reinterpret_cast<mpMemoryRegion*>(poolStart + stride);
        region->data = poolStart + stride + sizeof(mpMemoryRegion);
        region->regionSize = regionSize;
        region->poolID = uniqueID;
        if(i < (regionCount - 1))
            region->next = reinterpret_cast<mpMemoryRegion*>(poolStart + ((i + 1) * regionStride));
    }
    return pool;
}
// Fetches a fresh region from the given pool
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
// Fetches an alias to the regions data ptr, then bumbs the regions data ptr forward by allocationSize
inline void* mpAllocateIntoRegion(mpMemoryRegion *region, size_t allocationSize)
{
    void *data = region->data;
    region->data += allocationSize;
    region->dataSize += allocationSize;
    if(region->dataSize > region->regionSize)
    {
        MP_LOG_ERROR
        printf("MEMORY ERROR: mpAllocateIntoRegion failed: region(pool: %d) is out of memory\n", region->poolID);
        MP_LOG_RESET
        return nullptr;
    }

    return data;
}
// Allows reuse of the same region by deleting it's memory.
inline void mpResetMemoryRegion(mpMemoryRegion *region)
{
    region->data -= region->dataSize;
    memset(region->data, 0, region->dataSize);
    region->dataSize = 0;
}
// Free the region and releases it back to the memory pool
inline void mpFreeMemoryRegion(mpMemoryPool *pool, mpMemoryRegion *region)
{
    if(region == nullptr)
    {
        MP_LOG_ERROR
        puts("MEMORY ERROR: mpFreeMemoryRegion failed: region is null");
        MP_LOG_RESET
        return;
    }
    if(pool->ID != region->poolID)
    {
        MP_LOG_ERROR
        printf("MEMORY ERROR: mpFreeMemoryRegion failed: given region does not belong to pool %d\n", pool->ID);
        MP_LOG_RESET
        return;
    }
    region->data -= region->dataSize;
    memset(region->data, 0, region->dataSize);
    region->dataSize = 0;

    region->next = pool->head;
    pool->head = region;
}
// Completely terminates a pool and makes it unusable
inline void mpDestroyMemoryPool(mpMemoryPool *pool)
{
    free(pool->start);
    memset(pool, 0, sizeof(mpMemoryPool));
}