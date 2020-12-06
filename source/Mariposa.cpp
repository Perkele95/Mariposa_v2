#include "core.h"
#include "Win32_Mariposa.h"
#include "VulkanLayer.h"

#include <stdlib.h>
#include <stdio.h>

#define MP_CHUNK_SIZE 16
#define MP_VOXEL_SCALE 0.5f
// TODO: static global variables
// Full cube vertex positions
static const vec3 voxVerts[8] = {
    {-MP_VOXEL_SCALE, -MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE, -MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE,  MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    {-MP_VOXEL_SCALE,  MP_VOXEL_SCALE,  MP_VOXEL_SCALE},
    {-MP_VOXEL_SCALE,  MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE,  MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
    { MP_VOXEL_SCALE, -MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
    {-MP_VOXEL_SCALE, -MP_VOXEL_SCALE, -MP_VOXEL_SCALE},
};
static const vec3 _mpVoxelFaceTop[]    = {voxVerts[0], voxVerts[1], voxVerts[2], voxVerts[3]};
static const vec3 _mpVoxelFaceBottom[] = {voxVerts[4], voxVerts[5], voxVerts[6], voxVerts[7]};
static const vec3 _mpVoxelFaceNorth[]  = {voxVerts[1], voxVerts[6], voxVerts[5], voxVerts[2]};
static const vec3 _mpVoxelFaceSouth[]  = {voxVerts[3], voxVerts[4], voxVerts[7], voxVerts[0]};
static const vec3 _mpVoxelFaceEast[]   = {voxVerts[2], voxVerts[5], voxVerts[4], voxVerts[3]};
static const vec3 _mpVoxelFaceWest[]   = {voxVerts[0], voxVerts[7], voxVerts[6], voxVerts[1]};

static const uint16_t _mpVoxelIndexStride = 6;
static const uint16_t _mpVoxelIndices[_mpVoxelIndexStride] = {0, 1, 2, 2, 3, 0};

static const vec3 _mpBlockColours[Voxel_Type_MAX] = {
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

// TODO: Function needs to be cleaned up after verification
static mpMesh mpCreateChunkMesh(mpVoxelChunk *chunk)
{
    enum mpVoxelCullingFlagBits
    {
        VOXEL_CULLING_FLAG_TOP = 0x0001,
        VOXEL_CULLING_FLAG_BOTTOM = 0x0002,
        VOXEL_CULLING_FLAG_NORTH = 0x0004,
        VOXEL_CULLING_FLAG_SOUTH = 0x0008,
        VOXEL_CULLING_FLAG_EAST = 0x0010,
        VOXEL_CULLING_FLAG_WEST = 0x0020,
    };
    
    uint16_t vertexCount = 0, indexCount = 0;
    
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
                
                vec3 positionOffset = {};
                positionOffset.X = static_cast<float>(x) + chunk->position.X;
                positionOffset.Y = static_cast<float>(y) + chunk->position.Y;
                positionOffset.Z = static_cast<float>(z) + chunk->position.Z;
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

static void UpdateCameraControlState(const mpEventReceiver *eventReceiver, mpCameraControls *cameraControls)
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

static void mpGenerateTerrain(mpVoxelChunk *chunk)
{
    // TODO: Use seed as paremeter to be able to generate different worlds
    float heightMap = 0.0f, globalX = 0.0f, globalY = 0.0f;
    
    for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++)
            {
                globalX = chunk->position.X + static_cast<float>(x);
                globalY = chunk->position.Y + static_cast<float>(y);
                
                heightMap = Perlin(globalX / 100.0f, globalY / 100.0f);
                heightMap *= 400.0f;
                
                if(static_cast<float>(z) <= heightMap)
                {
                    chunk->pBlocks[x][y][z].isActive = true;
                    chunk->pBlocks[x][y][z].type = Voxel_Type_Grass;
                }
            }
        }
    }
}

static mpWorldData mpGenerateWorldData()
{
    mpWorldData worldData = {};
    worldData.chunkCount = 2;
    worldData.chunks = static_cast<mpVoxelChunk*>(malloc(sizeof(mpVoxelChunk) * worldData.chunkCount));
    
    for(uint32_t i = 0; i < worldData.chunkCount; i++)
    {
        worldData.chunks[i] = mpCreateVoxelChunk();
        worldData.chunks[i].position.X = static_cast<float>(i * MP_CHUNK_SIZE);
        mpGenerateTerrain(&worldData.chunks[i]);
    }
    
    return worldData;
}

static mpRenderData mpGenerateRenderData(const mpWorldData *worldData)
{
    mpRenderData renderData = {};
    
    renderData.meshCount = worldData->chunkCount;
    renderData.meshes = static_cast<mpMesh*>(malloc(sizeof(mpMesh) * renderData.meshCount));
    
    for(uint32_t i = 0; i < renderData.meshCount; i++)
        renderData.meshes[i] = mpCreateChunkMesh(&worldData->chunks[i]);
    
    return renderData;
}

int main(int argc, char *argv[])
{
    mpEngine engine = {};
    engine.name = "Mariposa";
    engine.windowInfo = {};
    
    PlatformCreateWindow(&engine.windowInfo);
    // TODO: Prepare win32 sound
    engine.callbacks = PlatformGetCallbacks();
    
    mpMemoryArena rendererArena = mpCreateMemoryArena(MegaBytes(64));
    mpMemoryArena transientArena = mpCreateMemoryArena(MegaBytes(64));
    mpMemorySubdivision vulkanMemory = mpSubdivideMemoryArena(&rendererArena, MegaBytes(1));
    
    // TODO: use memory arena here
    engine.worldData = mpGenerateWorldData();
    engine.renderData = mpGenerateRenderData(&engine.worldData);
    
    mpVulkanInit(&engine, &vulkanMemory);
    
    engine.camera = {};
    engine.camera.speed = 2.0f;
    engine.camera.sensitivity = 2.0f;
    engine.camera.fov = PI32 / 4.0f;
    engine.camera.model = Mat4x4Identity();
    engine.camera.position = vec3{2.0f, 2.0f, 2.0f};
    engine.camera.pitchClamp = (PI32 / 2.0f) - 0.01f;
    
    engine.camControls = {};
    engine.eventReceiver = {};
    
    engine.fpsSampler = {};
    engine.fpsSampler.level = 1000;
    
    float timestep = 0.0f;
    int64_t lastCounter = 0, perfCountFrequency = 0;
    PlatformPrepareClock(&lastCounter, &perfCountFrequency);
    
    engine.windowInfo.running = true;
    engine.windowInfo.hasResized = false; // WM_SIZE is triggered at startup, so we need to reset hasResized before the loop
    while(engine.windowInfo.running)
    {
        PlatformPollEvents(&engine.eventReceiver);
        
        UpdateCameraControlState(&engine.eventReceiver, &engine.camControls);
        
        if(engine.camControls.rUp)
            engine.camera.pitch += engine.camera.sensitivity * timestep;
        else if(engine.camControls.rDown)
            engine.camera.pitch -= engine.camera.sensitivity * timestep;
        if(engine.camControls.rLeft)
            engine.camera.yaw += engine.camera.sensitivity * timestep;
        else if(engine.camControls.rRight)
            engine.camera.yaw -= engine.camera.sensitivity * timestep;
        
        if(engine.camera.pitch > engine.camera.pitchClamp)
            engine.camera.pitch = engine.camera.pitchClamp;
        else if(engine.camera.pitch < -engine.camera.pitchClamp)
            engine.camera.pitch = -engine.camera.pitchClamp;
        
        vec3 front = {cosf(engine.camera.pitch) * cosf(engine.camera.yaw), cosf(engine.camera.pitch) * sinf(engine.camera.yaw), sinf(engine.camera.pitch)};
        vec3 left = {sinf(engine.camera.yaw), -cosf(engine.camera.yaw), 0.0f};
        if(engine.camControls.tForward)
            engine.camera.position += front * engine.camera.speed * timestep;
        else if(engine.camControls.tBackward)
            engine.camera.position -= front * engine.camera.speed * timestep;
        if(engine.camControls.tLeft)
            engine.camera.position -= left * engine.camera.speed * timestep;
        else if(engine.camControls.tRight)
            engine.camera.position += left * engine.camera.speed * timestep;
        
        engine.camera.view = LookAt(engine.camera.position, engine.camera.position + front, {0.0f, 0.0f, 1.0f});
        engine.camera.projection = Perspective(engine.camera.fov, static_cast<float>(engine.windowInfo.width) / static_cast<float>(engine.windowInfo.height), 0.1f, 40.0f);
        
        mpVulkanUpdate(&engine, &vulkanMemory);
        
        engine.windowInfo.hasResized = false;
        ResetEventReceiver(&engine.eventReceiver);
        timestep = PlatformUpdateClock(&lastCounter, perfCountFrequency);
        UpdateFpsSampler(&engine.fpsSampler, timestep);
    }
    
    mpVulkanCleanup(&engine.rendererHandle, engine.renderData.meshCount);
    
    for(uint32_t k = 0; k < engine.renderData.meshCount; k++)
    {
        mpDestroyChunkMesh(engine.renderData.meshes[k]);
        mpDestroyVoxelChunk(engine.worldData.chunks[k]);
    }
    free(engine.renderData.meshes);
    free(engine.worldData.chunks);
    
    mpDestroyMemoryArena(rendererArena);
    mpDestroyMemoryArena(transientArena);
    
    return 0;
}