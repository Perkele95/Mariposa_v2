#pragma once

#include <stdint.h>
#include <cstring>
#include "vulkan\vulkan.h"
#include "mp_maths.h"
#include "permutation.h"
#include "events.h"
#include "mpGui.h"

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

constexpr int64_t KiloBytes(const int64_t amount) {return amount * 1024LL;}
constexpr int64_t MegaBytes(const int64_t amount) {return amount * 1024LL * 1024LL;}
constexpr int64_t GigaBytes(const int64_t amount) {return amount * 1024LL * 1024LL * 1024LL;}

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))
#define arraysize3D(array3D) (sizeof(array3D) / sizeof((array3D)[0][0][0]))

constexpr int32_t MP_SUBREGION_SIZE = 20;
constexpr int32_t MP_REGION_SIZE = 10;

constexpr float MP_GRAVITY_CONSTANT = 9.81f;
constexpr vec3 gravityVec3 = {0.0f, 0.0f, -MP_GRAVITY_CONSTANT * 5};

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
    int32_t ID;
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
    mpBitField flags;
};

struct mpVoxelSubRegion
{
    mpVoxel voxels[MP_SUBREGION_SIZE][MP_SUBREGION_SIZE][MP_SUBREGION_SIZE];
    vec3 position;
    mpBitField flags;
};

struct mpVoxelRegion
{
    mpVoxelSubRegion subRegions[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
    mpMesh meshArray[MP_REGION_SIZE][MP_REGION_SIZE][MP_REGION_SIZE];
    mpBitField reserved;
};

enum mpSubRegionFlags
{
    MP_SUBREG_FLAG_EMPTY            = 0x0001,
    MP_SUBREG_FLAG_NEIGHBOUR_NORTH  = 0x0002,
    MP_SUBREG_FLAG_NEIGHBOUR_SOUTH  = 0x0004,
    MP_SUBREG_FLAG_NEIGHBOUR_TOP    = 0x0008,
    MP_SUBREG_FLAG_NEIGHBOUR_BOTTOM = 0x0010,
    MP_SUBREG_FLAG_NEIGHBOUR_EAST   = 0x0020,
    MP_SUBREG_FLAG_NEIGHBOUR_WEST   = 0x0040,
    MP_SUBREG_FLAG_DIRTY            = 0x0080,
};

enum mpVoxelFlags
{
    MP_VOXEL_FLAG_ACTIVE      = 0x00000001,
    MP_VOXEL_FLAG_DRAW_TOP    = 0x00000002,
    MP_VOXEL_FLAG_DRAW_BOTTOM = 0x00000004,
    MP_VOXEL_FLAG_DRAW_NORTH  = 0x00000008,
    MP_VOXEL_FLAG_DRAW_SOUTH  = 0x00000010,
    MP_VOXEL_FLAG_DRAW_EAST   = 0x00000020,
    MP_VOXEL_FLAG_DRAW_WEST   = 0x00000040,
};

enum mpRenderFlags
{
    MP_RENDER_FLAG_REDRAW_REQUIRED        = 0x0001,
    MP_RENDER_FLAG_ENABLE_VK_VALIDATION   = 0x0002,
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

struct mpPointLight
{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 diffuse;
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

    mpVoxelRegion *region;
    mpGUI::renderData guiData;

    mpPointLight pointLight;
};

struct mpEntity
{
    vec3 position, velocity, force;
    float mass, speed;
    uint32_t physState;
};
