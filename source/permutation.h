#pragma once

#include "core.h"

//#define MP_INCLUDE_PERMUTATIONS

#ifdef MP_INCLUDE_PERMUTATIONS
struct mpPermutation
{
    mpVertex vertices[8];
    uint32_t vertexCount;
    uint16_t indices[6 * 6];
    uint32_t indexCount;
};

struct mpPermutationTable
{
    mpPermutation data[64];
};

constexpr size_t mpGetPermTableMemoryRequirements()
{
    const size_t result = sizeof(mpPermutationTable);
    return result;
}

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

inline void mpRotateVec3Clockwise(float &a, float &b)
{
    if(a > 0){
        if(b > 0)
            b = -b;
        else
            a = -a;
    }
    else{
        if(b > 0)
            a = -a;
        else
            b = -b;
    }
}

inline void mpXyRotatePermutation(mpPermutation &permuation)
{
    for(uint32_t i = 0; i < permuation.vertexCount; i++){
        float pA = permuation.vertices[i].position.x;
        float pB = permuation.vertices[i].position.y;
        float nA = permuation.vertices[i].normal.x;
        float nB = permuation.vertices[i].normal.y;
        mpRotateVec3Clockwise(pA, pB);
        mpRotateVec3Clockwise(nA, nB);
    }
}

inline void mpXzRotatePermutation(mpPermutation &permuation)
{
    for(uint32_t i = 0; i < permuation.vertexCount; i++){
        float pA = permuation.vertices[i].position.x;
        float pB = permuation.vertices[i].position.z;
        float nA = permuation.vertices[i].normal.x;
        float nB = permuation.vertices[i].normal.z;
        mpRotateVec3Clockwise(pA, pB);
        mpRotateVec3Clockwise(nA, nB);
    }
}

inline void mpYzRotatePermutation(mpPermutation &permuation)
{
    for(uint32_t i = 0; i < permuation.vertexCount; i++){
        float pA = permuation.vertices[i].position.y;
        float pB = permuation.vertices[i].position.z;
        float nA = permuation.vertices[i].normal.y;
        float nB = permuation.vertices[i].normal.z;
        mpRotateVec3Clockwise(pA, pB);
        mpRotateVec3Clockwise(nA, nB);
    }
}

enum mpUniquePermutation
{
    MP_UNIQUE_PERMUTATIONS_SINGLE_FACE,
    MP_UNIQUE_PERMUTATIONS_TWO_FACES_1,
    MP_UNIQUE_PERMUTATIONS_TWO_FACES_2,
    MP_UNIQUE_PERMUTATIONS_THREE_FACES_1,
    MP_UNIQUE_PERMUTATIONS_THREE_FACES_2,
    MP_UNIQUE_PERMUTATIONS_FOUR_FACES_1,
    MP_UNIQUE_PERMUTATIONS_FOUR_FACES_2,
    MP_UNIQUE_PERMUTATIONS_FIVE_FACES,
    MP_UNIQUE_PERMUTATIONS_SIX_FACES,
};

inline mpPermutation mpGenerateUniquePermutation(mpUniquePermutation uniqueType, uint32_t clockwiseRotations)
{
    mpPermutation permutation = {};
    switch (uniqueType)
    {
    case MP_UNIQUE_PERMUTATIONS_SINGLE_FACE:
    // top
        permutation.vertices[0].position = mpVertexPositons[0];
        permutation.vertices[0].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertices[1].position = mpVertexPositons[1];
        permutation.vertices[1].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertices[2].position = mpVertexPositons[2];
        permutation.vertices[2].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertices[3].position = mpVertexPositons[3];
        permutation.vertices[3].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertexCount = 4;
        permutation.indices[0] = 0;
        permutation.indices[1] = 1;
        permutation.indices[2] = 2;
        permutation.indices[3] = 2;
        permutation.indices[4] = 3;
        permutation.indices[5] = 0;
        permutation.indexCount = 6;
        break;
    case MP_UNIQUE_PERMUTATIONS_TWO_FACES_1:
    // Top and south
        permutation.vertices[0].position = mpVertexPositons[0];
        permutation.vertices[0].normal = {-1.0f, 0.0f, 1.0f};
        permutation.vertices[1].position = mpVertexPositons[1];
        permutation.vertices[1].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertices[2].position = mpVertexPositons[2];
        permutation.vertices[2].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertices[3].position = mpVertexPositons[3];
        permutation.vertices[3].normal = {-1.0f, 0.0f, 1.0f};
        permutation.vertices[4].position = mpVertexPositons[4];
        permutation.vertices[4].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertices[5].position = mpVertexPositons[7];
        permutation.vertices[5].normal = {0.0f, 0.0f, 1.0f};
        permutation.vertexCount = 6;
        permutation.indices[0] = 0;
        permutation.indices[1] = 1;
        permutation.indices[2] = 2;
        permutation.indices[3] = 2;
        permutation.indices[4] = 3;
        permutation.indices[5] = 0;
        permutation.indices[6] = 0;
        permutation.indices[7] = 3;
        permutation.indices[8] = 4;
        permutation.indices[9] = 4;
        permutation.indices[10] = 5;
        permutation.indices[11] = 0;
        permutation.indexCount = 12;
        break;
    case MP_UNIQUE_PERMUTATIONS_TWO_FACES_2:
    // Top and bottom
    case MP_UNIQUE_PERMUTATIONS_THREE_FACES_1:
    // Top, south and bottom
    case MP_UNIQUE_PERMUTATIONS_THREE_FACES_2:
    // Top, south and west
    case MP_UNIQUE_PERMUTATIONS_FOUR_FACES_1:
    // Top, bottom, south, north
    case MP_UNIQUE_PERMUTATIONS_FOUR_FACES_2:
    // Top, bottom, south, west
    case MP_UNIQUE_PERMUTATIONS_FIVE_FACES:
    // Top, bottom, south, west, north
    case MP_UNIQUE_PERMUTATIONS_SIX_FACES:
    // All faces
        break;

    default:
        break;
    }
}

#endif