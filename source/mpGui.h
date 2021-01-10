#pragma once

#include "mp_maths.h"
#include "memory.h"

typedef int32_t bool32;

struct mpGuiVertex
{
    vec2 position;
    vec2 texCoord;
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

struct mpGuiMesh
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
    mpMemoryRegion memory;
    mpGuiMesh *meshes;
    uint32_t meshCount;
    uint32_t maxElementsPerTexture;
};

constexpr int32_t MPGUI_ITEM_NULL = 0;
constexpr int32_t MPGUI_ITEM_UNAVAILABLE = -1;

inline mpGUI mpGuiInitialise(uint32_t maxElementsPerTexture, uint32_t textureCount)
{
    const size_t vertexAllocSize = sizeof(mpGuiVertex) * 4 * maxElementsPerTexture;
    const size_t indexAllocSize = sizeof(uint16_t) * 6 * maxElementsPerTexture;
    const size_t meshHeaderSize = sizeof(mpGuiMesh) * textureCount;
    const size_t regionAllocSize = (vertexAllocSize + indexAllocSize) * textureCount + meshHeaderSize;

    mpGUI gui = {};
    memset(&gui, 0, sizeof(mpGUI));
    gui.memory = mpCreateMemoryRegion(regionAllocSize);
    gui.meshCount = textureCount;
    gui.meshes = static_cast<mpGuiMesh*>(mpAlloc(gui.memory, meshHeaderSize));
    for(uint32_t i = 0; i < textureCount; i++){
        gui.meshes[i].vertices = static_cast<mpGuiVertex*>(mpAlloc(gui.memory, vertexAllocSize));
        gui.meshes[i].indices = static_cast<uint16_t*>(mpAlloc(gui.memory, indexAllocSize));
    }
    gui.maxElementsPerTexture = maxElementsPerTexture;

    return gui;
}

inline void mpGuiBegin(mpGUI &gui, mpPoint extent, mpPoint mousePos, bool32 mouseButtonDown)
{
    for(uint32_t i = 0; i < gui.meshCount; i++){
        mpGuiMesh &mesh = gui.meshes[i];
        mesh.vertexCount = 0;
        mesh.indexCount = 0;
        memset(mesh.vertices, 0, sizeof(mpGuiVertex) * mesh.vertexCount);
        memset(mesh.indices, 0, sizeof(uint16_t) * mesh.indexCount);
    }
    gui.extent = extent;
    gui.state.hotItem = 0;
    gui.state.mousePosition = mousePos;
    gui.state.mouseButtonDown = mouseButtonDown;
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
// TODO: vec4->32 bit colour value
inline void mpDrawRect2D(mpGUI &gui, const mpRect2D &rect, const vec4 colour, uint32_t textureIndex)
{
    // Converte rect2d to quad data
    const vec2 topLeft = mpScreenToVertexSpace(rect.topLeft, gui.extent);
    const vec2 topRight = mpScreenToVertexSpace(mpPoint{rect.bottomRight.x, rect.topLeft.y}, gui.extent);
    const vec2 bottomRight = mpScreenToVertexSpace(rect.bottomRight, gui.extent);
    const vec2 bottomLeft = mpScreenToVertexSpace(mpPoint{rect.topLeft.x, rect.bottomRight.y}, gui.extent);

    // Push data into gui memory
    mpGuiMesh &mesh = gui.meshes[textureIndex];
    mesh.vertices[mesh.vertexCount]     = mpGuiVertex{topLeft, {1.0f, 0.0f}, colour};
    mesh.vertices[mesh.vertexCount + 1] = mpGuiVertex{topRight, {0.0f, 0.0f}, colour};
    mesh.vertices[mesh.vertexCount + 2] = mpGuiVertex{bottomRight, {0.0f, 1.0f}, colour};
    mesh.vertices[mesh.vertexCount + 3] = mpGuiVertex{bottomLeft, {1.0f, 1.0f}, colour};
    mesh.vertexCount += 4;

    mesh.indices[mesh.indexCount]     = static_cast<uint16_t>(mesh.vertexCount + 0);
    mesh.indices[mesh.indexCount + 1] = static_cast<uint16_t>(mesh.vertexCount + 1);
    mesh.indices[mesh.indexCount + 2] = static_cast<uint16_t>(mesh.vertexCount + 2);
    mesh.indices[mesh.indexCount + 3] = static_cast<uint16_t>(mesh.vertexCount + 3);
    mesh.indices[mesh.indexCount + 4] = static_cast<uint16_t>(mesh.vertexCount + 4);
    mesh.indices[mesh.indexCount + 5] = static_cast<uint16_t>(mesh.vertexCount + 5);
    mesh.indexCount += 6;
}
// NOTE: width and height range from 0 to 100%
inline void mpDrawAdjustedRect2D(mpGUI &gui, int32_t widthPercent, int32_t heightPercent, vec4 colour, uint32_t textureIndex)
{
    const mpPoint rectExtent = {gui.extent.x * widthPercent / 200, gui.extent.y * heightPercent / 200};
    const mpPoint centre = {gui.extent.x / 2, gui.extent.y / 2};
    const mpRect2D rect = {
        {centre.x - rectExtent.x, centre.y - rectExtent.y},
        {centre.x + rectExtent.x, centre.y + rectExtent.y}
    };
    mpDrawRect2D(gui, rect, colour, textureIndex);
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
inline bool32 mpButton(mpGUI &gui, int32_t id, mpPoint centre, uint32_t textureIndex)
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
            mpDrawRect2D(gui, button, {1.0f, 1.0f, 1.0f, 1.0f}, textureIndex); // Button is 'hot' & 'active'
        else
            mpDrawRect2D(gui, button, {0.6f, 0.6f, 0.6f, 1.0f}, textureIndex); // Button is 'hot'
    }
    else{
        // button is not hot, but it may be active
        mpDrawRect2D(gui, button, {0.2f, 0.2f, 0.2f, 1.0f}, textureIndex);
    }
    bool32 result = false;
    if(gui.state.mouseButtonDown == false && gui.state.hotItem == id && gui.state.activeItem == id)
        result = true;
    return result;
}