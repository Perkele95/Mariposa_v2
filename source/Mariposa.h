#pragma once

#include "core.h"

constexpr uint32_t mpGetDictMax()
{
    return VOXEL_TYPE_MAX;
}

typedef uint32_t mpDictKey;
struct mpVoxelTypeDictionary
{
    vec4 data[mpGetDictMax()];
};

static mpVoxelTypeDictionary mpCreateVoxelTypeDictionary()
{
    mpVoxelTypeDictionary dict;
    memset(&dict, 0, sizeof(mpVoxelTypeDictionary));
    return dict;
}

static vec4 mpVoxelDictionaryGetValue(mpVoxelTypeDictionary *dict, mpDictKey key)
{
    vec4 value = dict->data[key];
    return value;
}

/*---------
 * ECS
 */
struct tagComponent
{
    char *tag;
};

enum ECSComponentIndices
{
    ECS_COMPONENT_INDEX_TAG,
    ECS_NUM_COMPONENTS,
};

typedef uint32_t entity, entityKey;
constexpr uint32_t ECS_MAX_ENTITIES = 20;
struct entityRegistry
{
    entityKey table[ECS_MAX_ENTITIES][ECS_NUM_COMPONENTS];
    tagComponent tagComponents[ECS_MAX_ENTITIES];
};

inline static entityRegistry createEntityRegistry()
{
}