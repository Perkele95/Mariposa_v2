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
        mpPoint mousePosition;
        mpPoint screenExtent;
        bool32 mouseButtonDown;
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

constexpr int32_t MPGUI_ITEM_NULL = 0;
constexpr int32_t MPGUI_ITEM_UNAVAILABLE = -1;

inline mpGUI mpGuiInitialise(const uint32_t memoryID, const uint32_t maxElements)
{
    const size_t verticesSize = sizeof(mpGuiVertex) * maxElements;
    const size_t indicesSize = sizeof(uint16_t) * maxElements;
    mpGUI gui = {};
    memset(&gui, 0, sizeof(mpGUI));
    gui.memoryPool = mpCreateMemoryPool(1, verticesSize + indicesSize, memoryID);
    gui.memory = mpGetMemoryRegion(&gui.memoryPool);
    gui.data.vertices = static_cast<mpGuiVertex*>(mpAllocateIntoRegion(gui.memory, verticesSize));
    gui.data.indices = static_cast<uint16_t*>(mpAllocateIntoRegion(gui.memory, indicesSize));
    gui.maxElements = maxElements;

    return gui;
}

inline void mpGuiBegin(mpGUI &gui, mpPoint extent, mpPoint mousePos, bool32 mouseButtonDown)
{
    gui.data.vertexCount = 0;
    gui.data.indexCount = 0;
    gui.extent = extent;
    gui.elementCount = 0;
    gui.state.hotItem = 0;
    gui.state.mousePosition = mousePos;
    gui.state.mouseButtonDown = mouseButtonDown;
    mpResetMemoryRegion(gui.memory);
}

inline void mpGuiEnd(mpGUI &gui)
{
    if(gui.state.mouseButtonDown == false)
        gui.state.activeItem = MPGUI_ITEM_NULL;
    else if(gui.state.activeItem == false)
        gui.state.activeItem = MPGUI_ITEM_UNAVAILABLE;
}

inline static vec2 mpScreenToVertexSpace(mpPoint coords, mpPoint extent)
{
    vec2 result = {};
    result.x = ((static_cast<float>(coords.x) / static_cast<float>(extent.x)) * 2.0f) - 1.0f;
    result.y = ((static_cast<float>(coords.y) / static_cast<float>(extent.y)) * 2.0f) - 1.0f;
    return result;
}

inline void mpDrawRect2D(mpGUI &gui, const mpRect2D &rect, const vec4 colour)
{
    // Converte rect2d to quad data
    const vec2 topLeft = mpScreenToVertexSpace(rect.topLeft, gui.extent);
    const vec2 topRight = mpScreenToVertexSpace(mpPoint{rect.bottomRight.x, rect.topLeft.y}, gui.extent);
    const vec2 bottomRight = mpScreenToVertexSpace(rect.bottomRight, gui.extent);
    const vec2 bottomLeft = mpScreenToVertexSpace(mpPoint{rect.topLeft.x, rect.bottomRight.y}, gui.extent);
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
// NOTE: width and height range from 0 to 100%
inline void mpDrawAdjustedRect2D(mpGUI &gui, int32_t widthPercent, int32_t heightPercent, vec4 colour)
{
    const mpPoint rectExtent = {gui.extent.x * widthPercent / 200, gui.extent.y * heightPercent / 200};
    const mpPoint centre = {gui.extent.x / 2, gui.extent.y / 2};
    const mpRect2D rect = {
        {centre.x - rectExtent.x, centre.y - rectExtent.y},
        {centre.x + rectExtent.x, centre.y + rectExtent.y}
    };
    mpDrawRect2D(gui, rect, colour);
}

// Centre and size values define a bounding box area
inline bool32 mpRectHit(mpPoint mousePos, mpRect2D rect)
{
    const bool32 xCheck1 = mousePos.x > rect.topLeft.x;
    const bool32 xCheck2 = mousePos.y > rect.topLeft.y;
    const bool32 yCheck1 = mousePos.x < rect.bottomRight.x;
    const bool32 yCheck2 = mousePos.y < rect.bottomRight.y;
    return xCheck1 && xCheck2 && yCheck1 && yCheck2;
}
// TODO: vec4 colour -> 32 bit rgba value
inline bool32 mpButton(mpGUI &gui, int32_t id, mpPoint centre)
{
    constexpr mpPoint btnSize = {50, 20};
    const mpRect2D button = {
        {centre.x - btnSize.x, centre.y - btnSize.y},
        {centre.x + btnSize.x, centre.y + btnSize.y}
    };
    // Check if it should be hot or active
    if(mpRectHit(gui.state.mousePosition, button)){
        gui.state.hotItem = id;
        if(gui.state.activeItem == false && gui.state.mouseButtonDown)
            gui.state.activeItem = id;
    }
    // Render button
    if (gui.state.hotItem == id){
        if (gui.state.activeItem == id)
            mpDrawRect2D(gui, button, {1.0f, 1.0f, 1.0f, 1.0f}); // Button is 'hot' & 'active'
        else
            mpDrawRect2D(gui, button, {0.6f, 0.6f, 0.6f, 1.0f}); // Button is 'hot'
    }
    else{
        // button is not hot, but it may be active
        mpDrawRect2D(gui, button, {0.2f, 0.2f, 0.2f, 1.0f});
    }
    bool32 result = false;
    if(gui.state.mouseButtonDown == false && gui.state.hotItem == id && gui.state.activeItem == id)
        result = true;
    return result;
}