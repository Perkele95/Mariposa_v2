#pragma once

#include "mp_maths.h"

struct mpVertex
{
    vec3 position;
    vec3 normal;
    vec4 colour;
};
struct mpVertexSmaller
{
    vec3 position;
    vec3 normal;
    uint32_t colour;
};

struct mpQuadFaceArray
{
    vec3 data[4];
    vec3 normal;
};

constexpr float vScale = 1.0f;
/*
struct mpPermutation
{
    mpVertex vertices[8];
    uint32_t vertexCount;
    uint16_t indices[36];
    uint32_t indexCount;
};

struct mpPermutationTable
{
    mpPermutation data[64];
};

constexpr vec3 mpV0 = {-vScale, -vScale,  vScale};
constexpr vec3 mpV1 = { vScale, -vScale,  vScale};
constexpr vec3 mpV2 = { vScale,  vScale,  vScale};
constexpr vec3 mpV3 = {-vScale,  vScale,  vScale};
constexpr vec3 mpV4 = {-vScale,  vScale, -vScale};
constexpr vec3 mpV5 = { vScale,  vScale, -vScale};
constexpr vec3 mpV6 = { vScale, -vScale, -vScale};
constexpr vec3 mpV7 = {-vScale, -vScale, -vScale};

constexpr vec3 mpSouthNormal = {-1.0f, 0.0f, 0.0f};
constexpr vec3 mpNorthNormal = {1.0f, 0.0f, 0.0f};
constexpr vec3 mpWestNormal = {0.0f, -1.0f, 0.0f};
constexpr vec3 mpEastNormal = {0.0f, 1.0f, 0.0f};
constexpr vec3 mpBottomNormal = {0.0f, 0.0f, -1.0f};
constexpr vec3 mpTopNormal = {0.0f, 0.0f, 1.0f};

constexpr vec3 mpVertexPositons[] = {
    {-vScale, -vScale,  vScale},
    { vScale, -vScale,  vScale},
    { vScale,  vScale,  vScale},
    {-vScale,  vScale,  vScale},
    {-vScale,  vScale, -vScale},
    { vScale,  vScale, -vScale},
    { vScale, -vScale, -vScale},
    {-vScale, -vScale, -vScale}
};
// Template vertex
constexpr mpVertex mpTmptVert(uint32_t vertex, const vec3 normal)
{
    const mpVertex templateVertex = {mpVertexPositons[vertex], normal, {1.0f, 1.0f, 1.0f, 1.0f}};
    return templateVertex;
}
*/