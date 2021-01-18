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

/*
 *  Mesh Registry
 */

inline mpMeshRegistry mpCreateMeshRegistry(uint32_t meshCapacity, uint32_t queueCapacity)
{
    mpMeshRegistry registry = {};
    memset(&registry, 0, sizeof(mpMeshRegistry));

    // Init mesharray
    const size_t meshDataSize = meshCapacity * sizeof(mpMesh);
    registry.meshArrayMemory = mpCreateMemoryRegion(meshDataSize);
    registry.meshArray.data = static_cast<mpMesh*>(mpAlloc(registry.meshArrayMemory, meshDataSize));
    registry.meshArray.count = 0;
    registry.meshArray.max = meshCapacity;

    // Init queue
    registry.queueMemory = mpCreateMemoryRegion(queueCapacity * sizeof(mpMeshQueueItem));
    // Init queue free list
    registry.queue.freeListHead = static_cast<mpMeshQueueItem*>(mpAlloc(registry.queueMemory, queueCapacity * sizeof(mpMeshQueueItem)));
    for(uint32_t i = 0; i < queueCapacity - 1; i++)
        registry.queue.freeListHead[i].next = &registry.queue.freeListHead[i + 1];

    return registry;
}

inline void mpDestroyMeshRegistry(mpMeshRegistry &registry)
{
    mpDestroyMemoryRegion(registry.meshArrayMemory);
    mpDestroyMemoryRegion(registry.queueMemory);
    memset(&registry, 0, sizeof(mpMeshRegistry));
}
// outMeshID is not optional
inline mpMesh &mpGetMesh(mpMeshRegistry &registry, int32_t *outMeshID)
{
    if(registry.meshArray.count + 1 > registry.meshArray.max){
        registry.meshArray.max *= 2;
        mpResizeMemory(registry.meshArrayMemory, sizeof(mpMesh) * registry.meshArray.max);
    }
    mpMesh &result = registry.meshArray.data[registry.meshArray.count];
    *outMeshID = registry.meshArray.count;
    registry.meshArray.count++;
    return result;
}
// TODO
inline void mpReplaceMesh(mpMeshRegistry &registry, mpMesh &oldMesh, mpMesh &newMesh);
// TODO
inline void mpDeregisterMesh(mpMeshRegistry &registry, mpMesh &mesh);

inline void mpEnqueueMesh(mpMeshRegistry &registry, mpMeshQueueData &queueData)
{
    mpMeshQueue &queue = registry.queue;
    if(queue.freeListHead == nullptr){
        // TODO: realloc
        MP_PUTS_WARN("MESHQUEUE WARNING, enqueue denied: queue full");
        return;
    }
    mpMeshQueueItem *enqeuedItem = queue.freeListHead;
    queue.freeListHead = queue.freeListHead->next;

    if(queue.front == nullptr){
        queue.front = enqeuedItem;
        queue.rear = enqeuedItem;
    }
    else{
        queue.rear->next = enqeuedItem;
        queue.rear = enqeuedItem;
    }
    enqeuedItem->data = queueData;
    enqeuedItem->next = nullptr;// <- NOTE: perhaps not required
}

inline mpMeshQueueData mpDequeueMesh(mpMeshRegistry &registry)
{
    mpMeshQueue &queue = registry.queue;
#ifdef MP_INTERNAL
    if(queue.front == nullptr){
        MP_PUTS_WARN("MESHQUEUE WARNING, dequeue denied: queue empty");
        mp_assert(0)
    }
#endif
    mpMeshQueueData result = queue.front->data;
    if(queue.front == queue.rear){
        queue.front->next = queue.freeListHead;
        queue.freeListHead = queue.front;
        queue.front = nullptr;
        queue.rear = nullptr;
    }
    else{
        mpMeshQueueItem *dequeuedItem = queue.front;
        queue.front = queue.front->next;
        dequeuedItem->next = queue.freeListHead;
        queue.freeListHead = dequeuedItem;
    }
    return result;
}