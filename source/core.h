#pragma once

#include <stdint.h>
#include <cstring>
#include "..\Vulkan\Include\vulkan\vulkan.h"
#include "mp_maths.h"

#define MP_INTERNAL
//#define MP_PERFORMANCE

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef MP_PERFORMANCE
    #define mp_assert(Expression)
#else
    #define mp_assert(Expression) if(!(Expression)) {DebugBreak();}
#endif

#define UINT64MAX 0xFFFFFFFFFFFFFFFF
#define PI32 3.14159265359f
#define PI32_D 6.28318530718f

typedef int32_t bool32;
typedef void* mpHandle;
typedef mpHandle mpRenderer;
typedef mpHandle mpFileHandle;

#define KiloBytes(value) (value * 1024LL)
#define MegaBytes(value) (value * 1024LL * 1024LL)
#define GigaBytes(value) (value * 1024LL * 1024LL * 1024LL)
#define TeraBytes(value) (value * 1024LL * 1024LL * 1024LL * 1024LL)

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))

struct mpWindowData
{
    int32_t width, height;
    bool32 hasResized, running;
};

struct mpFile
{
    mpFileHandle contents;
    uint32_t size;
};

struct mpCallbacks
{
    void (*GetSurface)(VkInstance instance, VkSurfaceKHR *surface);// TODO: REPLACE
    // FILE IO
    void (*mpCloseFile)(mpFileHandle *handle);
    mpFile (*mpReadFile)(const char *filename);
    bool32 (*mpWriteFile)(const char *filename, mpFile *file);
};

struct mpMemory
{
    void *CombinedStorage;
    void *PermanentStorage;
    uint64_t PermanentStorageSize;
    void *TransientStorage;
    uint64_t TransientStorageSize;
};

struct mpThreadContext
{
    int placeholder;
};

struct mpFPSsampler
{
    const uint32_t level;
    uint32_t count;
    float value;
};

struct mpVertex
{
    vec3 position;
    vec3 colour;
};

struct mpVoxelData
{
    mpVertex *vertices;
    uint16_t *indices;
    
    uint64_t voxelCount;
    uint32_t voxelsPerBatch;
    uint32_t batchCount;
    uint32_t gridXWidth;
    float voxelScale;
};

struct mpCamera
{
    mat4x4 model, view, projection;
    vec3 position;
    float pitch, yaw, fov, speed, sensitivity;
};

struct mpCameraControls
{   // R = rotation, T = translation
    bool32 rUp, rDown, rLeft, rRight, tForward, tBackward, tLeft, tRight;
};

inline static void* PushBackPermanentStorage(mpMemory* memory, size_t pushSize)
{
    void* result = memory->PermanentStorage;
    memory->PermanentStorage = static_cast<uint8_t*>(memory->PermanentStorage) + pushSize;
    return result;
}

inline static void* PushBackTransientStorage(mpMemory* memory, size_t pushSize)
{
    void* result = memory->TransientStorage;
    memory->TransientStorage = static_cast<uint8_t*>(memory->TransientStorage) + pushSize;
    return result;
}