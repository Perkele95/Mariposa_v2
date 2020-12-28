#pragma once

#include "core.h"

// Helpers

inline grid32 mpVec3ToGrid32(vec3 a)
{
    grid32 result = {
        static_cast<int32_t>(a.x),
        static_cast<int32_t>(a.y),
        static_cast<int32_t>(a.z)
    };
    return result;
}
inline vec3 mpGrid32ToVec3(grid32 a)
{
    vec3 result = {
        static_cast<float>(a.x),
        static_cast<float>(a.y),
        static_cast<float>(a.z)
    };
    return result;
}
inline gridU32 mpVec3ToGridU32(vec3 a)
{
    gridU32 result = {
        static_cast<uint32_t>(a.x),
        static_cast<uint32_t>(a.y),
        static_cast<uint32_t>(a.z)
    };
    return result;
}
inline vec3 mpGridU32ToVec3(gridU32 a)
{
    vec3 result = {
        static_cast<float>(a.x),
        static_cast<float>(a.y),
        static_cast<float>(a.z)
    };
    return result;
}

static mpChunk* mpGetContainingChunk(const mpWorldData &worldData, vec3 position)
{
    const grid32 gIndex = mpVec3ToGrid32(position);
    const uint32_t chunkIndexX = gIndex.x / MP_CHUNK_SIZE;
    const uint32_t chunkIndexY = gIndex.y / MP_CHUNK_SIZE;
    const uint32_t chunkIndexZ = gIndex.z / MP_CHUNK_SIZE;

    if(chunkIndexX > worldData.bounds.x || chunkIndexY > worldData.bounds.y || chunkIndexZ > worldData.bounds.z)
        return nullptr;
    const uint32_t index = chunkIndexX + worldData.bounds.x * (chunkIndexY + chunkIndexZ * worldData.bounds.y);
    return &worldData.chunks[index];
}

// Colour Array
enum mpVoxelType
{
    VOXEL_TYPE_GRASS,
    VOXEL_TYPE_DIRT,
    VOXEL_TYPE_STONE,
    VOXEL_TYPE_WOOD,
    VOXEL_TYPE_MAX
};
enum mpVoxelTypeModifier
{
    VOXEL_TYPE_MOD_DEFAULT,
    VOXEL_TYPE_MOD_DARK,
    VOXEL_TYPE_MOD_MAX
};

struct mpVoxelTypeTable
{
    vec4 colourArray[VOXEL_TYPE_MAX][VOXEL_TYPE_MOD_MAX];
};

mpVoxelTypeTable* mpCreateVoxelTypeTable(void *buffer)
{
    mpVoxelTypeTable *list = static_cast<mpVoxelTypeTable*>(buffer);
    return list;
}

void mpRegisterVoxelType(mpVoxelTypeTable *table, vec4 colour, uint32_t type, uint32_t modifier = VOXEL_TYPE_MOD_DEFAULT)
{
    table->colourArray[type][modifier] = colour;
}

inline vec4 mpGetVoxelColour(mpVoxelTypeTable *table, uint32_t type, uint32_t modifier = VOXEL_TYPE_MOD_DEFAULT)
{
    return table->colourArray[type][modifier];
}

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