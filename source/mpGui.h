#pragma once

#include "mp_maths.h"
#include "memory.h"

typedef int32_t bool32;

struct mpGuiVertex
{
    vec2 position;
    vec4 colour;
};
struct mpGuiQuad
{
    mpGuiVertex vertices[4];
    uint16_t indices[6];
};
struct mpPoint
{
    int32_t x, y;
};
struct mpRect2D
{
    mpPoint topLeft;
    mpPoint bottomRight;
};
struct mpGuiData
{
    mpGuiVertex *vertices;
    uint16_t *indices;
    uint32_t vertexCount;
    uint32_t indexCount;
};
struct mpGUI
{
    struct
    {
        int32_t activeItem;
        int32_t hotItem;
    }state;

    mpPoint extent;
    mpMemoryPool memoryPool;
    mpMemoryRegion memory;
    mpGuiData data;
    uint32_t maxElements;
    uint32_t elementCount;
};

inline static vec2 mpScreenToWorld(mpPoint coords, mpPoint extent)
{
    vec2 result = {};
    result.x = ((static_cast<float>(coords.x) / static_cast<float>(extent.x)) * 2.0f) - 1.0f;
    result.y = ((static_cast<float>(coords.y) / static_cast<float>(extent.y)) * 2.0f) - 1.0f;
    return result;
}

inline mpGUI mpInitialiseGUI(const uint32_t memoryID, const uint32_t maxElements)
{
    const size_t verticesSize = sizeof(mpGuiVertex) * maxElements;
    const size_t indicesSize = sizeof(uint16_t) * maxElements;
    mpGUI gui = {};
    gui.memoryPool = mpCreateMemoryPool(1, verticesSize + indicesSize, memoryID);
    gui.memory = mpGetMemoryRegion(&gui.memoryPool);
    gui.data.vertices = static_cast<mpGuiVertex*>(mpAllocateIntoRegion(gui.memory, verticesSize));
    gui.data.indices = static_cast<uint16_t*>(mpAllocateIntoRegion(gui.memory, indicesSize));
    gui.maxElements = maxElements;

    return gui;
}

inline void mpGUIReset(mpGUI &gui, mpPoint extent)
{
    gui.data.vertexCount = 0;
    gui.data.indexCount = 0;
    gui.extent = extent;
    gui.elementCount = 0;
    mpResetMemoryRegion(gui.memory);
}

inline void mpDrawRect2D(mpGUI &gui, const mpRect2D &rect, const vec4 colour)
{
    // Converte rect2d to quad data
    const vec2 topLeft = mpScreenToWorld(rect.topLeft, gui.extent);
    const vec2 topRight = mpScreenToWorld(mpPoint{rect.bottomRight.x, rect.topLeft.y}, gui.extent);
    const vec2 bottomRight = mpScreenToWorld(rect.bottomRight, gui.extent);
    const vec2 bottomLeft = mpScreenToWorld(mpPoint{rect.topLeft.x, rect.bottomRight.y}, gui.extent);
    // Create indices
    const uint32_t indexOffset = gui.elementCount * 4;
    const uint32_t indices[] = {
        indexOffset,
        1 + indexOffset,
        2 + indexOffset,
        2 + indexOffset,
        3 + indexOffset,
        indexOffset,
    };
    // Push data into gui memory
    gui.data.vertices[(gui.elementCount * 4)] = mpGuiVertex{topLeft, colour};
    gui.data.vertices[(gui.elementCount * 4) + 1] = mpGuiVertex{topRight, colour};
    gui.data.vertices[(gui.elementCount * 4) + 2] = mpGuiVertex{bottomRight, colour};
    gui.data.vertices[(gui.elementCount * 4) + 3] = mpGuiVertex{bottomLeft, colour};
    gui.data.vertexCount += 4;

    gui.data.indices[(gui.elementCount * 6)] = static_cast<uint16_t>(indices[0]);
    gui.data.indices[(gui.elementCount * 6) + 1] = static_cast<uint16_t>(indices[1]);
    gui.data.indices[(gui.elementCount * 6) + 2] = static_cast<uint16_t>(indices[2]);
    gui.data.indices[(gui.elementCount * 6) + 3] = static_cast<uint16_t>(indices[3]);
    gui.data.indices[(gui.elementCount * 6) + 4] = static_cast<uint16_t>(indices[4]);
    gui.data.indices[(gui.elementCount * 6) + 5] = static_cast<uint16_t>(indices[5]);
    gui.data.indexCount += 6;
    gui.elementCount++;
}
