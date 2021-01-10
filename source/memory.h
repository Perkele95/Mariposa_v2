#pragma once

#include "logger.h"
#include <stdint.h>
#include <stdio.h>
#include <cstring>

struct mpMemoryRegion_T
{
    void *data;
    size_t totalSize;
    size_t contentSize;
};
typedef mpMemoryRegion_T *mpMemoryRegion;

inline mpMemoryRegion mpCreateMemoryRegion(size_t regionSize)
{
    mpMemoryRegion region = static_cast<mpMemoryRegion>(malloc(regionSize + sizeof(mpMemoryRegion_T)));
    region->totalSize = regionSize;
    region->contentSize = 0;
    region->data = reinterpret_cast<uint8_t*>(region) + sizeof(mpMemoryRegion_T);
    memset(region->data, 0, regionSize);
    return region;
}

inline void mpDestroyMemoryRegion(mpMemoryRegion region)
{
    free(region);
}

inline void mpResetMemoryRegion(mpMemoryRegion region)
{
    region->data = static_cast<uint8_t*>(region->data) - region->contentSize;
    region->contentSize = 0;
    memset(region->data, 0, region->totalSize);
}

inline void *mpAlloc(mpMemoryRegion region, size_t size)
{
    void *result = nullptr;
    if(region->contentSize + size > region->totalSize){
        MP_PUTS_ERROR("MEMORY ERROR: Allocation request denied, cannot allocate past region");
        mp_assert(false);
    }
    else{
        result = region->data;
        region->data = static_cast<uint8_t*>(region->data) + size;
        region->contentSize += size;
    }
    return result;
}