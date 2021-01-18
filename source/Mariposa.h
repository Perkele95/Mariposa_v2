#pragma once

#include "core.h"

// Helpers

inline mpVoxelSubRegion *mpGetContainintSubRegion(mpVoxelRegion &region, vec3 target)
{
    const int32_t x = static_cast<int32_t>(target.x) / MP_SUBREGION_SIZE;
    const int32_t y = static_cast<int32_t>(target.y) / MP_SUBREGION_SIZE;
    const int32_t z = static_cast<int32_t>(target.z) / MP_SUBREGION_SIZE;
    constexpr uint32_t bounds = arraysize(mpVoxelRegion::subRegions);

    if(x > bounds || y > bounds || z > bounds)
        return nullptr;

    mpVoxelSubRegion *result = &region.subRegions[x][y][z];
    return result;
}

struct mpVoxelQueryInfo
{
    mpVoxel *voxel;
    mpVoxelSubRegion *subRegion;
};

struct mpRayCastHitInfo
{
    mpVoxel *voxel;
    vec3 position;
};
// TODO: Handle edge case when outside bounds
inline mpVoxelQueryInfo mpQueryVoxelLocation(mpVoxelRegion &region, vec3 target)
{
    const int32_t srx = static_cast<int32_t>(target.x) / MP_SUBREGION_SIZE;
    const int32_t sry = static_cast<int32_t>(target.y) / MP_SUBREGION_SIZE;
    const int32_t srz = static_cast<int32_t>(target.z) / MP_SUBREGION_SIZE;
    constexpr uint32_t bounds = arraysize(mpVoxelRegion::subRegions);

    //if(srx > bounds || sry > bounds || srz > bounds)
        // Error, outside bounds

    mpVoxelSubRegion *subRegion = &region.subRegions[srx][sry][srz];
    const vec3Int vi = mpVec3ToVec3Int(target - subRegion->position);

    mpVoxelQueryInfo queryInfo = {};
    queryInfo.subRegion = subRegion;
    queryInfo.voxel = &subRegion->voxels[vi.x][vi.y][vi.z];

    return queryInfo;
}

inline vec4 mpConvertToDenseColour(mpVoxelColour colour)
{
    vec4 result = {};
    result.x = static_cast<float>(colour.r) / 255.0f;
    result.y = static_cast<float>(colour.g) / 255.0f;
    result.z = static_cast<float>(colour.b) / 255.0f;
    result.w = static_cast<float>(colour.a) / 255.0f;

    return result;
}

inline uint8_t mpRandomUint8()
{
    return static_cast<uint8_t>(rand() / 500);
}
