#pragma once

#include "logger.h"
#include <stdint.h>
#include <stdio.h>
#include <cstring>

struct mpMemoryArena
{
    uint8_t *begin, *data;
    uint32_t subDivCount;
    size_t size, sizeMax;
};

struct mpMemorySubdivision
{
    mpMemoryArena *parent;
    uint8_t *data;
    size_t size, sizeMax;
};

inline mpMemoryArena mpCreateMemoryArena(size_t size)
{
    mpMemoryArena arena = {};
    arena.begin = static_cast<uint8_t*>(malloc(size));
    memset(arena.begin, 0, size);
    arena.data = arena.begin;
    arena.sizeMax = size;
    
    return arena;
}

inline void mpDestroyMemoryArena(mpMemoryArena arena)
{
    free(arena.begin);
    arena.size = 0;
}

inline mpMemorySubdivision mpSubdivideMemoryArena(mpMemoryArena *arena, size_t size)
{
    mpMemorySubdivision subDiv = {};
    if((arena->size + size) < arena->sizeMax)
    {
        subDiv.parent = arena;
        subDiv.data = arena->data;
        subDiv.sizeMax = size;
        arena->data += size;
        arena->size += size;
        arena->subDivCount++;
    }
    else
    {
        MP_LOG_ERROR
        printf("MEMORY ERROR: Subdivision request denied; not enough space in arena");
        MP_LOG_RESET
    }
    return subDiv;
}

inline uint8_t* mpPushBackMemorySubdivision(mpMemorySubdivision *subDiv, size_t size)
{
    uint8_t* bytePtr = nullptr;
    if((subDiv->size + size) < subDiv->sizeMax)
    {
        bytePtr = (subDiv->data + subDiv->size);
        subDiv->size += size;
    }
    else
    {
        MP_LOG_ERROR
        printf("MEMORY ERROR: Subdivision push denied; not enough space in subdivision");
        MP_LOG_RESET
    }
    return bytePtr;
}
