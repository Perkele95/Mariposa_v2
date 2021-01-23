#pragma once

#include "core.h"

inline mpMeshRegistry mpCreateMeshRegistry(uint32_t meshCapacity, uint32_t queueCapacity)
{
    mpMeshRegistry registry = {};
    memset(&registry, 0, sizeof(mpMeshRegistry));

    // Init mesharray
    registry.meshAllocator = mpCreateAllocator(meshCapacity * sizeof(mpMesh));
    registry.meshArray.data = mpAllocate<mpMesh>(registry.meshAllocator, meshCapacity);
    registry.meshArray.count = 0;
    registry.meshArray.max = meshCapacity;

    // Init queue
    registry.queueAllocator = mpCreateAllocator(queueCapacity * sizeof(mpMeshQueueItem));
    // Init queue free list
    registry.queue.freeListHead = mpAllocate<mpMeshQueueItem>(registry.queueAllocator, queueCapacity);
    for(uint32_t i = 0; i < queueCapacity - 1; i++)
        registry.queue.freeListHead[i].next = &registry.queue.freeListHead[i + 1];

    return registry;
}

inline void mpDestroyMeshRegistry(mpMeshRegistry &registry)
{
    mpDestroyAllocator(registry.meshAllocator);
    mpDestroyAllocator(registry.queueAllocator);
    memset(&registry, 0, sizeof(mpMeshRegistry));
}
// outMeshID is not optional
inline mpMesh &mpGetMesh(mpMeshRegistry &registry, int32_t *outMeshID)
{
#if 0
    if(registry.meshArray.count + 1 > registry.meshArray.max){
        registry.meshArray.max *= 2;
    }
#endif
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