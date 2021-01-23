#pragma once

#include <stdint.h>
#include <cstring>
#include "vulkan\vulkan.h"
#include "mp_string.hpp"
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

#include "mp_allocator.hpp"
#include "logger.h"//TODO: exclude this from release mode
#include "profiler.h"//TODO: exclude this from release mode
#include "mpGui.h"
#include "mp_maths.hpp"

constexpr uint64_t UINT64MAX = 0xFFFFFFFFFFFFFFFF;
constexpr float PI32 = 3.14159265359f;
constexpr float PI32_D = 6.28318530718f;
constexpr float SQRT2 = 1.41421356237f;

constexpr size_t KiloBytes(const size_t amount) {return amount * 1024LL;}
constexpr size_t MegaBytes(const size_t amount) {return amount * 1024LL * 1024LL;}
constexpr size_t GigaBytes(const size_t amount) {return amount * 1024LL * 1024LL * 1024LL;}

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))
#define arraysize3D(array3D) (sizeof(array3D) / sizeof((array3D)[0][0][0]))

constexpr int32_t MP_SUBREGION_SIZE = 20;
constexpr int32_t MP_REGION_SIZE = 10;
constexpr int32_t MP_MESHID_NULL = -1;

constexpr float MP_GRAVITY_CONSTANT = 9.81f;
constexpr vec3 gravityVec3 = {0.0f, 0.0f, -MP_GRAVITY_CONSTANT * 5};
constexpr float vScale = 1.0f;

typedef int32_t bool32;
typedef void* mpHandle;
typedef uint32_t mpFlags;
typedef uint16_t mpBitFieldShort;

struct mpWindowData
{
    int32_t width, height;
    bool32 hasResized, running, fullscreen;
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
    void (*mpFreeFileMemory)(mpFile *file);
    mpFile (*mpReadFile)(const char *filename);
    bool32 (*mpWriteFile)(const char *filename, mpFile *file);
};

typedef int32_t (*mpThreadProc)(void *parameter);
struct mpThreadContext
{
    int32_t ID;
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
    uint32_t vertexCount;
    uint32_t indexCount;
    mpAllocator allocator;
};

union mpVoxelColour
{
    uint32_t rgba;
    struct
    {
        uint8_t r, g, b, a;
    };
};

struct mpVoxel
{
    mpVoxelColour colour;
    mpFlags flags;
};

struct mpVoxelSubRegion
{
    mpVoxel voxels[MP_SUBREGION_SIZE][MP_SUBREGION_SIZE][MP_SUBREGION_SIZE];
    vec3 position;
    vec3Int index;
    mpFlags flags;
    int32_t meshID;
};

struct mpVoxelRegion
{
    mpVoxelSubRegion subRegions[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
    mpFlags reserved;
};

enum mpVoxelFlags
{
    MP_VOXEL_FLAG_DRAW_TOP    = 0x0001,
    MP_VOXEL_FLAG_DRAW_BOTTOM = 0x0002,
    MP_VOXEL_FLAG_DRAW_NORTH  = 0x0004,
    MP_VOXEL_FLAG_DRAW_SOUTH  = 0x0008,
    MP_VOXEL_FLAG_DRAW_EAST   = 0x0010,
    MP_VOXEL_FLAG_DRAW_WEST   = 0x0020,
    MP_VOXEL_FLAG_ACTIVE      = 0x0040,
};

enum mpSubRegionFlags
{
    MP_SUBREG_FLAG_ACTIVE           = 0x0001,
    MP_SUBREG_FLAG_NEIGHBOUR_NORTH  = 0x0002,
    MP_SUBREG_FLAG_NEIGHBOUR_SOUTH  = 0x0004,
    MP_SUBREG_FLAG_NEIGHBOUR_TOP    = 0x0008,
    MP_SUBREG_FLAG_NEIGHBOUR_BOTTOM = 0x0010,
    MP_SUBREG_FLAG_NEIGHBOUR_EAST   = 0x0020,
    MP_SUBREG_FLAG_NEIGHBOUR_WEST   = 0x0040,
    MP_SUBREG_FLAG_VISIBLE          = 0x0080,
    MP_SUBREG_FLAG_OUT_OF_DATE      = 0x0100,
};
// split coreflags & renderingflags
enum mpRenderFlags
{
    MP_RENDER_FLAG_ENABLE_VK_VALIDATION  = 0x0001,
    MP_RENDER_FLAG_RESERVED              = 0x0002,
    MP_RENDER_FLAG_GUI_UPDATED           = 0x0004,
    MP_RENDER_FLAG_GENERATE_PERMUTATIONS = 0x0008,
    MP_RENDER_FLAG_REGENERATE_WORLD      = 0x0010,
};

struct mpCamera
{
    mat4x4 model, view, projection;
    vec3 position;
    float pitch, yaw, pitchClamp, fov, speed, sensitivity;
};

struct mpPointLight
{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 diffuse;
    float ambient;
};

enum mpGameState
{
    MP_GAMESTATE_ACTIVE,
    MP_GAMESTATE_PAUSED,
    MP_GAMESTATE_MAINMENU,
};

struct mpMeshQueueData
{
    mpMesh *mesh;
    mpVoxelSubRegion *subRegion;
};

struct mpMeshQueueItem
{
    mpMeshQueueData data;
    mpMeshQueueItem *next;
};

struct mpMeshQueue
{
    mpMeshQueueItem *front;
    mpMeshQueueItem *rear;
    mpMeshQueueItem *freeListHead;
};

struct mpMeshArray
{
    mpMesh *data;
    uint32_t count;
    uint32_t max;
};

struct mpMeshRegistry
{
    mpMeshArray meshArray;
    mpAllocator meshAllocator;
    mpMeshQueue queue;
    mpAllocator queueAllocator;
};

struct mpRenderer;
struct mpCore
{
    const char *name;
    mpWindowData windowInfo;
    mpCallbacks callbacks;
    mpEventHandler eventHandler;
    mpFlags gameState;
    mpPoint extent;
    mpPoint screenCentre;

    mpCamera camera;

    mpRenderer *renderer;
    mpFlags renderFlags;
    mpVoxelRegion *region;
    mpMeshRegistry meshRegistry;
    mpPointLight pointLight;
    mpGUI gui;
};

struct mpEntity
{
    vec3 position, velocity, force;
    float mass, speed;
    uint32_t physState;
};
