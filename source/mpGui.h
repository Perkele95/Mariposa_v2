#pragma once

#include "mp_maths.hpp"
#include "mp_allocator.hpp"

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
    mpAllocator allocator;
    mpGuiMesh *meshes;
    uint32_t meshCount;
    uint32_t maxElementsPerTexture;
};

enum mpGuiValues
{
    MPGUI_ITEM_NULL = -1,
    MPGUI_ITEM_UNAVAILABLE = -2,
};

inline mpGUI mpGuiInitialise(uint32_t maxElementsPerTexture, uint32_t textureCount)
{
    const uint32_t vertexAllocCount = 4 * maxElementsPerTexture;
    const uint32_t indexAllocCount = 6 * maxElementsPerTexture;
    const size_t combinedSize = sizeof(mpGuiVertex) * vertexAllocCount + sizeof(uint16_t) * indexAllocCount;
    const size_t regionAllocSize = combinedSize * textureCount;
    const size_t meshHeaderSize = sizeof(mpGuiMesh) * textureCount;

    mpGUI gui = {};
    memset(&gui, 0, sizeof(mpGUI));
    gui.allocator = mpCreateAllocator(meshHeaderSize + regionAllocSize);
    gui.meshCount = textureCount;
    gui.meshes = mpAllocate<mpGuiMesh>(gui.allocator, textureCount);
    for(uint32_t i = 0; i < textureCount; i++){
        gui.meshes[i].vertices = mpAllocate<mpGuiVertex>(gui.allocator, vertexAllocCount);
        gui.meshes[i].indices = mpAllocate<uint16_t>(gui.allocator, indexAllocCount);
    }
    gui.maxElementsPerTexture = maxElementsPerTexture;

    return gui;
}

inline void mpDestroyGui(mpGUI &gui)
{
    mpDestroyAllocator(gui.allocator);
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
    gui.state.hotItem = MPGUI_ITEM_NULL;
    gui.state.mousePosition = mousePos;
    gui.state.mouseButtonDown = mouseButtonDown;
}

inline void mpGuiEnd(mpGUI &gui)
{
    if(gui.state.mouseButtonDown == false)
        gui.state.activeItem = MPGUI_ITEM_NULL;
    //else if(gui.state.activeItem == MPGUI_ITEM_NULL)
    //    gui.state.activeItem = MPGUI_ITEM_UNAVAILABLE;
}

inline static vec2 mpScreenToVertexSpace(mpPoint coords, mpPoint extent)
{
    vec2 result = {};
    result.x = ((static_cast<float>(coords.x) / static_cast<float>(extent.x)) * 2.0f) - 1.0f;
    result.y = ((static_cast<float>(coords.y) / static_cast<float>(extent.y)) * 2.0f) - 1.0f;
    return result;
}
// Span is valid between 0-100, where the value represents percentage of screen extent
inline void mpDrawRect2D(mpGUI &gui, mpRect2D rect, const vec4 colour, uint32_t textureIndex = 0)
{
    // Converte rect2d to quad data
    const vec2 topLeft = mpScreenToVertexSpace(rect.topLeft, gui.extent);
    const vec2 topRight = mpScreenToVertexSpace(mpPoint{rect.bottomRight.x, rect.topLeft.y}, gui.extent);
    const vec2 bottomRight = mpScreenToVertexSpace(rect.bottomRight, gui.extent);
    const vec2 bottomLeft = mpScreenToVertexSpace(mpPoint{rect.topLeft.x, rect.bottomRight.y}, gui.extent);

    // Texture coordinates
    constexpr vec2 uvs[] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    // Push data into gui memory
    mpGuiMesh &mesh = gui.meshes[textureIndex];
    mesh.vertices[mesh.vertexCount]     = mpGuiVertex{topLeft,     uvs[0], colour};
    mesh.vertices[mesh.vertexCount + 1] = mpGuiVertex{topRight,    uvs[1], colour};
    mesh.vertices[mesh.vertexCount + 2] = mpGuiVertex{bottomRight, uvs[2], colour};
    mesh.vertices[mesh.vertexCount + 3] = mpGuiVertex{bottomLeft,  uvs[3], colour};

    mesh.indices[mesh.indexCount]     = static_cast<uint16_t>(mesh.vertexCount);
    mesh.indices[mesh.indexCount + 1] = static_cast<uint16_t>(mesh.vertexCount + 1);
    mesh.indices[mesh.indexCount + 2] = static_cast<uint16_t>(mesh.vertexCount + 2);
    mesh.indices[mesh.indexCount + 3] = static_cast<uint16_t>(mesh.vertexCount + 2);
    mesh.indices[mesh.indexCount + 4] = static_cast<uint16_t>(mesh.vertexCount + 3);
    mesh.indices[mesh.indexCount + 5] = static_cast<uint16_t>(mesh.vertexCount);
    mesh.vertexCount += 4;
    mesh.indexCount += 6;
}

inline mpRect2D mpGetAdjustedRect2D(mpPoint extent, mpPoint centre, mpPoint span)
{
    const mpPoint actualSpan = {
        extent.x * span.x / 200,
        extent.y * span.y / 200
    };
    const mpRect2D rect = {
        {centre.x - actualSpan.x, centre.y - actualSpan.y},
        {centre.x + actualSpan.x, centre.y + actualSpan.y}
    };
    return rect;
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
    constexpr mpPoint span = {10, 5};
    const mpRect2D button = mpGetAdjustedRect2D(gui.extent, centre, span);
    // Check if it should be hot or active
    if(mpRectHit(gui.state.mousePosition, button)){
        gui.state.hotItem = id;
        if(gui.state.activeItem == MPGUI_ITEM_NULL && gui.state.mouseButtonDown)
            gui.state.activeItem = id;
    }
    // Render button
    vec4 buttonColour = {};
    if (gui.state.hotItem == id){
        if (gui.state.activeItem == id)
            buttonColour = {0.1f, 0.1f, 0.1f, 1.0f}; // Button is 'hot' & 'active'
        else
            buttonColour = {0.3f, 0.3f, 0.3f, 1.0f}; // Button is 'hot'
    }
    else{
        buttonColour = {1.0f, 1.0f, 1.0f, 1.0f}; // button is not hot, but it may be active
    }
    mpDrawRect2D(gui, button, buttonColour, textureIndex);

    bool32 result = false;
    if(gui.state.mouseButtonDown == false && gui.state.hotItem == id && gui.state.activeItem == id)
        result = true;
    return result;
}