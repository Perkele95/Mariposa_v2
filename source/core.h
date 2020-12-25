#pragma once

#include <stdint.h>
#include <cstring>
#include "vulkan\vulkan.h"
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
#include "profiler.h"

constexpr uint64_t UINT64MAX = 0xFFFFFFFFFFFFFFFF;
constexpr float PI32 = 3.14159265359f;
constexpr float PI32_D = 6.28318530718f;

#define KiloBytes(value) (value * 1024LL)
#define MegaBytes(value) (value * 1024LL * 1024LL)
#define GigaBytes(value) (value * 1024LL * 1024LL * 1024LL)
#define TeraBytes(value) (value * 1024LL * 1024LL * 1024LL * 1024LL)

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))

constexpr uint32_t MP_CHUNK_SIZE = 20;
constexpr float MP_GRAVITY_CONSTANT = 9.81f;
constexpr vec3 gravityVec3 = {0.0f, 0.0f, -MP_GRAVITY_CONSTANT};

typedef int32_t bool32;
typedef void* mpHandle;
typedef uint32_t mpBitField;
typedef uint16_t mpBitFieldShort;

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

struct mpVertex
{
    vec3 position;
    vec3 normal;
    vec4 colour;
};

struct mpQuadFaceArray
{
    vec3 data[4];
    vec3 normal;
};

struct mpQuad
{
    mpVertex vertices[4];
    uint16_t indices[6];
};

struct mpMesh
{
    mpVertex *vertices;
    uint16_t *indices;
    mpMemoryRegion *memReg;
    uint32_t vertexCount;
    uint32_t indexCount;
};

struct mpRenderData
{
    mpMesh *meshes;
    uint32_t meshCount;
};

enum mpVoxelFlags
{
    VOXEL_FLAG_ACTIVE      = 0x00000001,
    VOXEL_FLAG_DRAW_TOP    = 0x00000002,
    VOXEL_FLAG_DRAW_BOTTOM = 0x00000004,
    VOXEL_FLAG_DRAW_NORTH  = 0x00000008,
    VOXEL_FLAG_DRAW_SOUTH  = 0x00000010,
    VOXEL_FLAG_DRAW_EAST   = 0x00000020,
    VOXEL_FLAG_DRAW_WEST   = 0x00000040,
};

struct mpVoxel
{
    mpBitFieldShort type;
    mpBitFieldShort modifier;
    mpBitField flags;
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
    CHUNK_FLAG_IS_DIRTY         = 0x0080,
};

struct mpChunk
{
    mpVoxel voxels[MP_CHUNK_SIZE][MP_CHUNK_SIZE][MP_CHUNK_SIZE];
    vec3 position;

    mpBitField flags;
    mpChunk *northNeighbour;
    mpChunk *southNeighbour;
    mpChunk *topNeighbour;
    mpChunk *bottomNeighbour;
    mpChunk *eastNeighbour;
    mpChunk *westNeighbour;
};

struct mpWorldData
{
    mpChunk *chunks;
    uint32_t chunkCount;
    gridU32 bounds;
};

struct mpCamera
{
    mat4x4 model, view, projection;
    vec3 position;
    float pitch, yaw, pitchClamp, fov, speed, sensitivity;
};

struct mpCameraControls
{
    mpBitField flags;
};

enum mpRenderFlags
{
    MP_RENDER_FLAG_REDRAW_MESHES = 0x0001,
    MP_RENDER_FLAG_DRAW_DEBUG   = 0x0002,
};

struct mpGlobalLight
{
    vec3 position;
    vec3 colour;
    float ambient;
};

struct mpCore
{
    const char *name;
    mpHandle rendererHandle;
    mpBitField renderFlags;
    mpWindowData windowInfo;
    mpCallbacks callbacks;
    mpEventReceiver eventReceiver;

    mpCamera camera;
    mpCameraControls camControls;

    mpWorldData worldData;
    mpRenderData renderData;

    mpGlobalLight globalLight;
};

struct mpEntity
{
    vec3 position, velocity, force;
    float mass, speed;
    uint32_t physState;
};