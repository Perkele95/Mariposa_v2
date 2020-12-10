#pragma once

#include <stdint.h>
#include <cstring>
#include "..\Vulkan\Include\vulkan\vulkan.h"
#include "mp_maths.h"
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

#include "memory.h"
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
    bool32 hasResized, running;
};

struct mpFile
{
    mpHandle handle;
    uint32_t size;
};

struct mpCallbacks
{
    void (*GetSurface)(VkInstance instance, VkSurfaceKHR *surface);
    // FILE IO
    void (*mpCloseFile)(mpFile *file);
    mpFile (*mpReadFile)(const char *filename);
    bool32 (*mpWriteFile)(const char *filename, mpFile *file);
};

struct mpThreadContext
{
    int32_t placeholder;
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
    bool32 isEmpty;
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

enum mpVoxelFlags
{
    VOXEL_FLAG_ACTIVE      = 0x0001,
    VOXEL_FLAG_DRAW_TOP    = 0x0002,
    VOXEL_FLAG_DRAW_BOTTOM = 0x0004,
    VOXEL_FLAG_DRAW_NORTH  = 0x0008,
    VOXEL_FLAG_DRAW_SOUTH  = 0x0010,
    VOXEL_FLAG_DRAW_EAST   = 0x0020,
    VOXEL_FLAG_DRAW_WEST   = 0x0040,
};

enum mpChunkFlags
{
    CHUNK_FLAG_IS_EMPTY         = 0x0001,
    CHUNK_FLAG_NEIGHBOUR_NORTH  = 0x0002,
    CHUNK_FLAG_NEIGHBOUR_SOUTH  = 0x0004,
    CHUNK_FLAG_NEIGHBOUR_TOP    = 0x0008,
    CHUNK_FLAG_NEIGHBOUR_BOTTOM = 0x0010,
    CHUNK_FLAG_NEIGHBOUR_EAST   = 0x0020,
    CHUNK_FLAG_NEIGHBOUR_WEST   = 0x0040,
};

struct mpVoxelBlock
{
    mpVoxelType type;
    uint32_t flags;
};

struct mpVoxelChunk
{
    mpVoxelBlock ***pBlocks;
    uint32_t size;
    vec3 position;
    
    uint32_t flags;
    mpVoxelChunk *northNeighbour;
    mpVoxelChunk *southNeighbour;
    mpVoxelChunk *topNeighbour;
    mpVoxelChunk *bottomNeighbour;
    mpVoxelChunk *eastNeighbour;
    mpVoxelChunk *westNeighbour;
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
