#include "core.h"
#include "Win32_Mariposa.h"
#include "VulkanLayer.h"
#include "events.h"

#include <stdlib.h>
#include <stdio.h>
// TODO: Unglobal these
#define MP_CHUNK_SIZE 16
#define MP_VOXEL_SCALE 0.5f

// TODO: Unglobal these
// Full cube vertex positions
const vec3 voxVerts[8] = {
    {-MP_VOXEL_SCALE, -MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE, -MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE,  MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    {-MP_VOXEL_SCALE,  MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    {-MP_VOXEL_SCALE,  MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE,  MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE, -MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
    {-MP_VOXEL_SCALE, -MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
};
const vec3 _mpVoxelFaceTop[]    = {voxVerts[0], voxVerts[1], voxVerts[2], voxVerts[3]};
const vec3 _mpVoxelFaceBottom[] = {voxVerts[4], voxVerts[5], voxVerts[6], voxVerts[7]};
const vec3 _mpVoxelFaceNorth[]  = {voxVerts[1], voxVerts[6], voxVerts[5], voxVerts[2]};
const vec3 _mpVoxelFaceSouth[]  = {voxVerts[3], voxVerts[4], voxVerts[7], voxVerts[0]};
const vec3 _mpVoxelFaceEast[]   = {voxVerts[2], voxVerts[5], voxVerts[4], voxVerts[3]};
const vec3 _mpVoxelFaceWest[]   = {voxVerts[0], voxVerts[7], voxVerts[6], voxVerts[1]};

const uint16_t _mpVoxelIndexStride = 6;
const uint16_t _mpVoxelIndices[_mpVoxelIndexStride] = {0, 1, 2, 2, 3, 0};

const vec3 _mpBlockColours[Voxel_Type_MAX] = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.2f},
    {0.5f, 0.4f, 0.2f},
    {0.4f, 0.4f, 0.4f},
};

static mpVoxelChunk mpCreateVoxelChunk()
{
    size_t chunkSize3x = MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    mpVoxelChunk chunk = {};
    chunk.size = MP_CHUNK_SIZE;
    chunk.pBlocks = static_cast<mpVoxelBlock***>(malloc(sizeof(mpVoxelBlock**) * MP_CHUNK_SIZE));
    *chunk.pBlocks = static_cast<mpVoxelBlock**>(malloc(sizeof(mpVoxelBlock*) * MP_CHUNK_SIZE * MP_CHUNK_SIZE));
    **chunk.pBlocks = static_cast<mpVoxelBlock*>(malloc(sizeof(mpVoxelBlock) * chunkSize3x));
    // Pointer to pointer assignment
    for(uint32_t i = 1; i < MP_CHUNK_SIZE; i++)
        chunk.pBlocks[i] = &chunk.pBlocks[0][0] + i * MP_CHUNK_SIZE;
    // Pointer assignment
    for(uint32_t i = 0; i < MP_CHUNK_SIZE; i++)
        for(uint32_t k = 0; k < MP_CHUNK_SIZE; k++)
            chunk.pBlocks[i][k] = (&chunk.pBlocks[0][0][0]) + i * MP_CHUNK_SIZE * MP_CHUNK_SIZE + k * MP_CHUNK_SIZE;
    // Zero out each chunk so all chunks are empty and not active by default
    memset(&(***chunk.pBlocks), 0, sizeof(mpVoxelBlock) * chunkSize3x);
    
    return chunk;
}

inline static void mpDestroyVoxelChunk(mpVoxelChunk chunk)
{
    free(**chunk.pBlocks);
    free(*chunk.pBlocks);
    free(chunk.pBlocks);
}

enum mpVoxelCullingFlags
{
    VOXEL_CULLING_FLAG_TOP = 0x0001,
    VOXEL_CULLING_FLAG_BOTTOM = 0x0002,
    VOXEL_CULLING_FLAG_NORTH = 0x0004,
    VOXEL_CULLING_FLAG_SOUTH = 0x0008,
    VOXEL_CULLING_FLAG_EAST = 0x0010,
    VOXEL_CULLING_FLAG_WEST = 0x0020,
};

// TODO: Function needs to be cleaned up after verification
static mpMesh mpCreateChunkMesh(mpVoxelChunk *chunk)
{
    uint16_t vertexCount = 0, indexCount = 0;
    // TODO: The sizes down below could be premultiplied
    const size_t tempVertBlockSize = sizeof(mpVertex) * 12 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    mpVertex *tempBlockVertices = static_cast<mpVertex*>(malloc(tempVertBlockSize));
    memset(tempBlockVertices, 0, tempVertBlockSize);
    mpVertex *tempBlockVertIncrementer = tempBlockVertices;
    
    const size_t tempIndexBlockSize = sizeof(uint16_t) * 18 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    uint16_t *tempBlockIndices = static_cast<uint16_t*>(malloc(tempIndexBlockSize));
    memset(tempBlockIndices, 0, tempIndexBlockSize);
    uint16_t *tempBlockIndexIncrementer = tempBlockIndices;
    
    for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++)
            {
                if(chunk->pBlocks[x][y][z].isActive == false)
                    continue;
                // TODO: set indices
                vec3 positionOffset = {};
                vec3 colour = _mpBlockColours[chunk->pBlocks[x][y][z].type];
                uint32_t cullingFlags = 0;
                if(x > 0 && chunk->pBlocks[x - 1][y][z].isActive)
                    cullingFlags |= VOXEL_CULLING_FLAG_SOUTH;
                if(x < (MP_CHUNK_SIZE - 1) && chunk->pBlocks[x + 1][y][z].isActive)
                    cullingFlags |= VOXEL_CULLING_FLAG_NORTH;
                
                if(y > 0 && chunk->pBlocks[x][y - 1][z].isActive)
                    cullingFlags |= VOXEL_CULLING_FLAG_WEST;
                if(x < (MP_CHUNK_SIZE - 1) && chunk->pBlocks[x][y + 1][z].isActive)
                    cullingFlags |= VOXEL_CULLING_FLAG_EAST;
                
                if(z > 0 && chunk->pBlocks[x][y][z - 1].isActive)
                    cullingFlags |= VOXEL_CULLING_FLAG_BOTTOM;
                if(x < (MP_CHUNK_SIZE - 1) && chunk->pBlocks[x][y][z + 1].isActive)
                    cullingFlags |= VOXEL_CULLING_FLAG_TOP;
                
                positionOffset.X = static_cast<float>(x);
                positionOffset.Y = static_cast<float>(y);
                positionOffset.Z = static_cast<float>(z);
                if(!(cullingFlags & VOXEL_CULLING_FLAG_NORTH))
                {
                    const mpVertex newNorthVertices[4] = {
                        {_mpVoxelFaceNorth[0] + positionOffset, colour}, {_mpVoxelFaceNorth[1] + positionOffset, colour},
                        {_mpVoxelFaceNorth[2] + positionOffset, colour}, {_mpVoxelFaceNorth[3] + positionOffset, colour}
                    };
                    memcpy(tempBlockVertIncrementer, &newNorthVertices, sizeof(mpVertex) * 4);
                    const uint16_t newIndices[_mpVoxelIndexStride] = {
                    static_cast<uint16_t>(_mpVoxelIndices[0] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[1] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[2] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[3] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[4] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[5] + vertexCount),
                    };
                    memcpy(tempBlockIndexIncrementer, &newIndices, sizeof(uint16_t) * _mpVoxelIndexStride);
                    tempBlockVertIncrementer += 4;
                    tempBlockIndexIncrementer += _mpVoxelIndexStride;
                    indexCount += _mpVoxelIndexStride;
                    vertexCount += 4;
                }
                if(!(cullingFlags & VOXEL_CULLING_FLAG_SOUTH))
                {
                    const mpVertex newSouthVertices[4] = {
                        {_mpVoxelFaceSouth[0] + positionOffset, colour}, {_mpVoxelFaceSouth[1] + positionOffset, colour},
                        {_mpVoxelFaceSouth[2] + positionOffset, colour}, {_mpVoxelFaceSouth[3] + positionOffset, colour}
                    };
                    memcpy(tempBlockVertIncrementer, &newSouthVertices, sizeof(mpVertex) * 4);
                    const uint16_t newIndices[_mpVoxelIndexStride] = {
                    static_cast<uint16_t>(_mpVoxelIndices[0] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[1] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[2] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[3] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[4] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[5] + vertexCount),
                    };
                    memcpy(tempBlockIndexIncrementer, &newIndices, sizeof(uint16_t) * _mpVoxelIndexStride);
                    tempBlockVertIncrementer += 4;
                    tempBlockIndexIncrementer += _mpVoxelIndexStride;
                    indexCount += _mpVoxelIndexStride;
                    vertexCount += 4;
                }
                if(!(cullingFlags & VOXEL_CULLING_FLAG_EAST))
                {
                    const mpVertex newEastVertices[4] = {
                        {_mpVoxelFaceEast[0] + positionOffset, colour}, {_mpVoxelFaceEast[1] + positionOffset, colour},
                        {_mpVoxelFaceEast[2] + positionOffset, colour}, {_mpVoxelFaceEast[3] + positionOffset, colour}
                    };
                    memcpy(tempBlockVertIncrementer, &newEastVertices, sizeof(mpVertex) * 4);
                    const uint16_t newIndices[_mpVoxelIndexStride] = {
                    static_cast<uint16_t>(_mpVoxelIndices[0] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[1] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[2] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[3] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[4] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[5] + vertexCount),
                    };
                    memcpy(tempBlockIndexIncrementer, &newIndices, sizeof(uint16_t) * _mpVoxelIndexStride);
                    tempBlockVertIncrementer += 4;
                    tempBlockIndexIncrementer += _mpVoxelIndexStride;
                    indexCount += _mpVoxelIndexStride;
                    vertexCount += 4;
                }
                if(!(cullingFlags & VOXEL_CULLING_FLAG_WEST))
                {
                    const mpVertex newWestVertices[4] = {
                        {_mpVoxelFaceWest[0] + positionOffset, colour}, {_mpVoxelFaceWest[1] + positionOffset, colour},
                        {_mpVoxelFaceWest[2] + positionOffset, colour}, {_mpVoxelFaceWest[3] + positionOffset, colour}
                    };
                    memcpy(tempBlockVertIncrementer, &newWestVertices, sizeof(mpVertex) * 4);
                    const uint16_t newIndices[_mpVoxelIndexStride] = {
                    static_cast<uint16_t>(_mpVoxelIndices[0] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[1] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[2] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[3] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[4] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[5] + vertexCount),
                    };
                    memcpy(tempBlockIndexIncrementer, &newIndices, sizeof(uint16_t) * _mpVoxelIndexStride);
                    tempBlockVertIncrementer += 4;
                    tempBlockIndexIncrementer += _mpVoxelIndexStride;
                    indexCount += _mpVoxelIndexStride;
                    vertexCount += 4;
                }
                if(!(cullingFlags & VOXEL_CULLING_FLAG_TOP))
                {
                    const mpVertex newTopVertices[4] = {
                        {_mpVoxelFaceTop[0] + positionOffset, colour}, {_mpVoxelFaceTop[1] + positionOffset, colour},
                        {_mpVoxelFaceTop[2] + positionOffset, colour}, {_mpVoxelFaceTop[3] + positionOffset, colour}
                    };
                    memcpy(tempBlockVertIncrementer, &newTopVertices, sizeof(mpVertex) * 4);
                    const uint16_t newIndices[_mpVoxelIndexStride] = {
                    static_cast<uint16_t>(_mpVoxelIndices[0] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[1] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[2] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[3] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[4] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[5] + vertexCount),
                    };
                    memcpy(tempBlockIndexIncrementer, &newIndices, sizeof(uint16_t) * _mpVoxelIndexStride);
                    tempBlockVertIncrementer += 4;
                    tempBlockIndexIncrementer += _mpVoxelIndexStride;
                    indexCount += _mpVoxelIndexStride;
                    vertexCount += 4;
                }
                if(!(cullingFlags & VOXEL_CULLING_FLAG_BOTTOM))
                {
                    const mpVertex newBottomVertices[4] = {
                        {_mpVoxelFaceBottom[0] + positionOffset, colour}, {_mpVoxelFaceBottom[1] + positionOffset, colour},
                        {_mpVoxelFaceBottom[2] + positionOffset, colour}, {_mpVoxelFaceBottom[3] + positionOffset, colour}
                    };
                    memcpy(tempBlockVertIncrementer, &newBottomVertices, sizeof(mpVertex) * 4);
                    const uint16_t newIndices[_mpVoxelIndexStride] = {
                    static_cast<uint16_t>(_mpVoxelIndices[0] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[1] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[2] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[3] + vertexCount),
                    static_cast<uint16_t>(_mpVoxelIndices[4] + vertexCount), static_cast<uint16_t>(_mpVoxelIndices[5] + vertexCount),
                    };
                    memcpy(tempBlockIndexIncrementer, &newIndices, sizeof(uint16_t) * _mpVoxelIndexStride);
                    tempBlockVertIncrementer += 4;
                    tempBlockIndexIncrementer += _mpVoxelIndexStride;
                    indexCount += _mpVoxelIndexStride;
                    vertexCount += 4;
                }
            }
        }
    }
    mpMesh mesh = {};
    mesh.verticesSize = static_cast<size_t>(vertexCount) * sizeof(mpVertex);
    mesh.indicesSize = static_cast<size_t>(indexCount) * sizeof(uint16_t);
    mesh.vertices = static_cast<mpVertex*>(malloc(mesh.verticesSize));
    mesh.indices = static_cast<uint16_t*>(malloc(mesh.indicesSize));
    mesh.indexCount = indexCount;
    
    memcpy(mesh.vertices, tempBlockVertices, mesh.verticesSize);
    memcpy(mesh.indices, tempBlockIndices, mesh.indicesSize);
    free(tempBlockVertices);
    free(tempBlockIndices);
    
    return mesh;
}

inline static void mpDestroyChunkMesh(mpMesh mesh)
{
    free(mesh.vertices);
    free(mesh.indices);
}

inline static void ProcessKeyToCameraControl(const mpEventReceiver *receiver, mpKeyEvent key, bool32 *controlValue)
{
    if(receiver->keyPressedEvents & key)
        (*controlValue) = true;
    else if(receiver->keyReleasedEvents & key)
        (*controlValue) = false;
}

static void GetCurrentCameraControls(const mpEventReceiver *eventReceiver, mpCameraControls *cameraControls)
{
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_W, &cameraControls->rUp);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_S, &cameraControls->rDown);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_A, &cameraControls->rLeft);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_D, &cameraControls->rRight);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_UP, &cameraControls->tForward);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_DOWN, &cameraControls->tBackward);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_LEFT, &cameraControls->tLeft);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_RIGHT, &cameraControls->tRight);
}

inline static void UpdateFpsSampler(mpFPSsampler *sampler, float timestep)
{
    if(sampler->count < sampler->level)
    {
        sampler->value += timestep;
        sampler->count++;
    }
    else
    {
        float sampledFps = static_cast<float>(sampler->level) / sampler->value;
        //printf("Sampled fps: %f\n", sampledFps);
        sampler->count = 0;
        sampler->value = 0;
    }
}

int main(int argc, char *argv[])
{    
    mpWindowData windowData = {};
    PlatformCreateWindow(&windowData);
    // TODO: Prepare win32 sound
    mpCallbacks callbacks = PlatformGetCallbacks();
        
    mpMemoryArena rendererArena = mpCreateMemoryArena(MegaBytes(64));
    mpMemoryArena transientArena = mpCreateMemoryArena(MegaBytes(64));
    mpMemorySubdivision vulkanMemory = mpSubdivideMemoryArena(&rendererArena, MegaBytes(1));
    
    // TODO: use memory arena here
    mpWorldData worldData = {};
    worldData.chunkCount = 1;
    worldData.chunks = static_cast<mpVoxelChunk*>(malloc(sizeof(mpVoxelChunk) * worldData.chunkCount));
    
    mpRenderData renderData = {};
    renderData.meshCount = worldData.chunkCount;
    renderData.meshes = static_cast<mpMesh*>(malloc(sizeof(mpMesh) * renderData.meshCount));
    
    for(uint32_t i = 0; i < worldData.chunkCount; i++)
        worldData.chunks[i] = mpCreateVoxelChunk();
    
    worldData.chunks[0].pBlocks[5][4][5].isActive = true;
    worldData.chunks[0].pBlocks[5][4][5].type = Voxel_Type_Grass;
    worldData.chunks[0].pBlocks[5][5][5].isActive = true;
    worldData.chunks[0].pBlocks[5][5][5].type = Voxel_Type_Dirt;
    worldData.chunks[0].pBlocks[5][6][5].isActive = true;
    worldData.chunks[0].pBlocks[5][6][5].type = Voxel_Type_Stone;
    
    worldData.chunks[0].pBlocks[4][5][5].isActive = true;
    worldData.chunks[0].pBlocks[4][5][5].type = Voxel_Type_Grass;
    worldData.chunks[0].pBlocks[5][5][5].isActive = true;
    worldData.chunks[0].pBlocks[5][5][5].type = Voxel_Type_Dirt;
    worldData.chunks[0].pBlocks[6][5][5].isActive = true;
    worldData.chunks[0].pBlocks[6][5][5].type = Voxel_Type_Stone;
    
    worldData.chunks[0].pBlocks[5][5][4].isActive = true;
    worldData.chunks[0].pBlocks[5][5][4].type = Voxel_Type_Grass;
    worldData.chunks[0].pBlocks[5][5][5].isActive = true;
    worldData.chunks[0].pBlocks[5][5][5].type = Voxel_Type_Dirt;
    worldData.chunks[0].pBlocks[5][5][6].isActive = true;
    worldData.chunks[0].pBlocks[5][5][6].type = Voxel_Type_Stone;
    
    for(uint32_t i = 0; i < worldData.chunkCount; i++)
        renderData.meshes[i] = mpCreateChunkMesh(&worldData.chunks[i]);
    
    mpCamera camera = {};
    camera.speed = 2.0f;
    camera.sensitivity = 2.0f;
    camera.fov = PI32 / 3.0f;
    camera.model = Mat4x4Identity();
    camera.position = vec3{2.0f, 2.0f, 2.0f};
    camera.pitchClamp = (PI32 / 2.0f) - 0.01f;
    
    mpRenderer renderer = nullptr;
    mpVulkanInit(&renderer, &vulkanMemory, &windowData, &renderData, &callbacks);
    
    mpCameraControls cameraControls = {};
    mpEventReceiver eventReceiver = {};
    
    mpFPSsampler fpsSampler = {1000,0,0};
    float timestep = 0.0f;
    int64_t lastCounter = 0, perfCountFrequency = 0;
    PlatformPrepareClock(&lastCounter, &perfCountFrequency);
    
    windowData.running = true;
    windowData.hasResized = false; // WM_SIZE is triggered at startup, so we need to reset hasResized before the loop
    while(windowData.running)
    {
        PlatformPollEvents(&eventReceiver);
        
        GetCurrentCameraControls(&eventReceiver, &cameraControls);
        
        if(cameraControls.rUp)
            camera.pitch += camera.sensitivity * timestep;
        else if(cameraControls.rDown)
            camera.pitch -= camera.sensitivity * timestep;
        if(cameraControls.rLeft)
            camera.yaw += camera.sensitivity * timestep;
        else if(cameraControls.rRight)
            camera.yaw -= camera.sensitivity * timestep;
        
        if(camera.pitch > camera.pitchClamp)
            camera.pitch = camera.pitchClamp;
        else if(camera.pitch < -camera.pitchClamp)
            camera.pitch = -camera.pitchClamp;
        
        vec3 front = {cosf(camera.pitch) * cosf(camera.yaw), cosf(camera.pitch) * sinf(camera.yaw), sinf(camera.pitch)};
        vec3 left = {sinf(camera.yaw), -cosf(camera.yaw), 0.0f};
        if(cameraControls.tForward)
            camera.position += front * camera.speed * timestep;
        else if(cameraControls.tBackward)
            camera.position -= front * camera.speed * timestep;
        if(cameraControls.tLeft)
            camera.position -= left * camera.speed * timestep;
        else if(cameraControls.tRight)
            camera.position += left * camera.speed * timestep;
        
        camera.view = LookAt(camera.position, camera.position + front, {0.0f, 0.0f, 1.0f});
        camera.projection = Perspective(camera.fov, static_cast<float>(windowData.width) / static_cast<float>(windowData.height), 0.1f, 20.0f);
        
        mpVulkanUpdate(&renderer, &renderData, &camera, &windowData);
        
        windowData.hasResized = false;
        ResetEventReceiver(&eventReceiver);
        timestep = PlatformUpdateClock(&lastCounter, perfCountFrequency);
        UpdateFpsSampler(&fpsSampler, timestep);
    }
    
    mpVulkanCleanup(&renderer, renderData.meshCount);
    
    for(uint32_t k = 0; k < renderData.meshCount; k++)
    {
        mpDestroyChunkMesh(renderData.meshes[k]);
        mpDestroyVoxelChunk(worldData.chunks[k]);
    }
    free(renderData.meshes);
    free(worldData.chunks);
    
    mpDestroyMemoryArena(rendererArena);
    mpDestroyMemoryArena(transientArena);
    
    return 0;
}