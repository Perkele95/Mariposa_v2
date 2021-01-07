#pragma once

#include "core.h"
/*
constexpr vec3 mpVertexPositons[] = {
    {-vScale, -vScale,  vScale},// 0
    { vScale, -vScale,  vScale},// 1
    { vScale,  vScale,  vScale},// 2
    {-vScale,  vScale,  vScale},// 3
    {-vScale,  vScale, -vScale},// 4
    { vScale,  vScale, -vScale},// 5
    { vScale, -vScale, -vScale},// 6
    {-vScale, -vScale, -vScale},// 7
};

constexpr vec3 mpNormals[] = {
    {-1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, 1.0f},
};

struct mpPermutation
{
    mpVertex vertices[8];
    uint32_t vertexCount;
    uint16_t indices[6 * 6];
    uint32_t indexCount;
};

static mpPermutation mpGenerateVoxelPermutation(mpBitField drawFlags)
{
    mpPermutation perm;
    memset(&perm, 0, sizeof(mpPermutation));
    if(drawFlags & MP_VOXEL_FLAG_DRAW_NORTH){
        // do
    }
    if(drawFlags & MP_VOXEL_FLAG_DRAW_SOUTH){
        // do
    }
    if(drawFlags & MP_VOXEL_FLAG_DRAW_EAST){
        // do
    }
    if(drawFlags & MP_VOXEL_FLAG_DRAW_WEST){
        // do
    }
    if(drawFlags & MP_VOXEL_FLAG_DRAW_TOP){
        // do
    }
    if(drawFlags & MP_VOXEL_FLAG_DRAW_BOTTOM){
        // do
    }
    return perm;
}
*/