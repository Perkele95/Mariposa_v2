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

struct mpRayCastHitInfo
{
    mpVoxel *voxel;
    vec3 position;
};

/*---------
 * ECS
 */
/* TODO: Figure out this shit
typedef int32_t Entity;
typedef uint32_t Signature;
typedef int32_t EntityKey;
constexpr int32_t ECS_NULL = -1;
constexpr uint32_t ECS_MAX_ENTITIES = 20;

enum mpComponentIndex
{
    ECS_COMP_TAG,
    ECS_COMP_PHYS,
    ECS_NUM_COMPONENTS
};

struct mpComponent
{
    Signature type;
    union
    {
        struct TagComponent
        {
            char tag[24];
        }asTag;
        struct PhysicsComponent
        {
            vec3 position, velocity;
            float mass;
        }asPhys;
    };
};

struct EntityRegistry
{
    EntityKey keyTable[ECS_MAX_ENTITIES][ECS_NUM_COMPONENTS];

    mpComponent::TagComponent tagComponents[ECS_NUM_COMPONENTS];
    mpComponent::PhysicsComponent physComponents[ECS_NUM_COMPONENTS];
};

inline static EntityRegistry* mpCreateEntityRegistry(void *buffer)
{
    EntityRegistry* reg = static_cast<EntityRegistry*>(buffer);
    for(uint32_t ID = 0; ID < ECS_NUM_COMPONENTS; ID++)
        reg->keyTable[ID][0] = ECS_NULL;

    return reg;
}

inline static Entity mpRegisterEntity(EntityRegistry *reg, Signature componentSignature)
{
    Entity newEntity = ECS_NULL;
    // => Table[entity][FreeFlag + component]
    // the 0th column is reserved for flags
    for(uint32_t ID = 0; ID < ECS_MAX_ENTITIES; ID++)
    {
        if(reg->keyTable[ID][0] == ECS_NULL)
        {
            newEntity = ID;
            reg->keyTable[ID][0] = 0;
            break;
        }
    }
    if(componentSignature & ECS_COMP_TAG)
    {
        // Tag Comp active
    }
    if(componentSignature & ECS_COMP_PHYS)
    {
        // Phys Comp active
    }
}
*/