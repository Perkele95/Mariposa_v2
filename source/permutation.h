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
constexpr vec3 mpV0 = {-vScale, -vScale,  vScale};
constexpr vec3 mpV1 = { vScale, -vScale,  vScale};
constexpr vec3 mpV2 = { vScale,  vScale,  vScale};
constexpr vec3 mpV3 = {-vScale,  vScale,  vScale};
constexpr vec3 mpV4 = {-vScale,  vScale, -vScale};
constexpr vec3 mpV5 = { vScale,  vScale, -vScale};
constexpr vec3 mpV6 = { vScale, -vScale, -vScale};
constexpr vec3 mpV7 = {-vScale, -vScale, -vScale};
*/