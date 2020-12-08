#pragma once

#include <stdint.h>
#include <cstring>
#include "..\Vulkan\Include\vulkan\vulkan.h"
#include "mp_maths.h"
#include "memory.h"
#include "events.h"

#define MP_INTERNAL
//#define MP_PERFORMANCE


#ifdef _WIN32
#include <windows.h>
#endif

#ifdef MP_PERFORMANCE
    #define mp_assert(Expression)
#else
    #define mp_assert(Expression) if(!(Expression)) {DebugBreak();}
    #define PROFILER_ENABLE
#endif

#include "logger.h"

#define UINT64MAX 0xFFFFFFFFFFFFFFFF
#define PI32 3.14159265359f
#define PI32_D 6.28318530718f

#define KiloBytes(value) (value * 1024LL)
#define MegaBytes(value) (value * 1024LL * 1024LL)
#define GigaBytes(value) (value * 1024LL * 1024LL * 1024LL)
#define TeraBytes(value) (value * 1024LL * 1024LL * 1024LL * 1024LL)

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))

typedef int32_t bool32;
typedef void* mpHandle;

struct mpWindowData
{
    int32_t width, height;
    volatile bool32 hasResized, running;
};

struct mpFile
{
    mpHandle handle;
    uint32_t size;
};

struct mpCallbacks
{
    void (*GetSurface)(VkInstance instance, VkSurfaceKHR *surface);// TODO: REPLACE
    // FILE IO
    void (*mpCloseFile)(mpFile *file);
    mpFile (*mpReadFile)(const char *filename);
    bool32 (*mpWriteFile)(const char *filename, mpFile *file);
};

struct mpThreadContext
{
    int placeholder;
};

struct mpFPSsampler
{
    uint32_t level;
    uint32_t count;
    float value;
};

struct mpVertex
{
    vec3 position;
    vec3 colour;
    vec3 normal;
};

struct mpMesh
{
    mpVertex *vertices;
    uint16_t *indices;
    size_t verticesSize;
    size_t indicesSize;
    uint32_t indexCount;
};

struct mpRenderData
{
    mpMesh *meshes;
    uint32_t meshCount;
};

enum mpVoxelType
{
    Voxel_Type_Empty,
    Voxel_Type_Grass,
    Voxel_Type_Dirt,
    Voxel_Type_Stone,
    Voxel_Type_MAX,
};

struct mpVoxelBlock
{
    mpVoxelType type;
    bool32 isActive;
};

struct mpVoxelChunk
{
    mpVoxelBlock ***pBlocks;
    uint32_t size;
    vec3 position;
};

struct mpWorldData
{
    mpVoxelChunk *chunks;
    uint32_t chunkCount;
};

struct mpCamera
{
    mat4x4 model, view, projection;
    vec3 position;
    float pitch, yaw, pitchClamp, fov, speed, sensitivity;
};
// R = rotation, T = translation
struct mpCameraControls
{
    bool32 rUp, rDown, rLeft, rRight, tForward, tBackward, tLeft, tRight;
};

struct mpEngine
{
    const char *name;
    mpHandle rendererHandle;
    mpWindowData windowInfo;
    mpCallbacks callbacks;
    mpFPSsampler fpsSampler;
    mpEventReceiver eventReceiver;
    
    mpCamera camera;
    mpCameraControls camControls;
    
    mpWorldData worldData;
    mpRenderData renderData;
};
