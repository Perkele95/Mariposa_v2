#pragma once

#include "mp_maths.h"

namespace mpGUI
{
    struct vertex
    {
        vec2 position;
        vec4 colour;
    };
    struct quad
    {
        vertex vertices[4];
        uint16_t indices[6];
    };
    struct renderData
    {
        vertex *vertices;
        uint16_t *indices;
        uint32_t vertexCount;
        uint32_t indexCount;
    };
    struct point
    {
        uint32_t x, y;
    };
    struct rect2D
    {
        point topLeft;
        point bottomRight;
    };

    inline static vec2 screenToWorld(point coords, point extent)
    {
        vec2 result = {};
        result.x = ((static_cast<float>(coords.x) / static_cast<float>(extent.x)) * 2.0f) - 1.0f;
        result.y = ((static_cast<float>(coords.y) / static_cast<float>(extent.y)) * 2.0f) - 1.0f;
        return result;
    }

    inline static quad quadFromRect(rect2D rect, vec4 colour, point extent)
    {
        vec2 topLeft = screenToWorld(rect.topLeft, extent);
        vec2 topRight = screenToWorld(point{rect.bottomRight.x, rect.topLeft.y}, extent);
        vec2 bottomRight = screenToWorld(rect.bottomRight, extent);
        vec2 bottomLeft = screenToWorld(point{rect.topLeft.x, rect.bottomRight.y}, extent);

        const quad result = {
            {
                {topLeft, colour},
                {topRight, colour},
                {bottomRight, colour},
                {bottomLeft, colour}
            },
            {
                0, 1, 2, 2, 3, 0
            }
        };
        return result;
    }
}