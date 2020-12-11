#include "core.h"
#include "Win32_Mariposa.h"
#include "VulkanLayer.h"
#include "profiler.h"

#include <stdlib.h>
#include <stdio.h>

#define MP_CHUNK_SIZE 16
#define MP_VOXEL_SCALE 0.5f

static uint32_t quadCount = 0;

static mpVoxelChunk mpAllocateChunk()
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

static mpQuad mpCreateQuadNorth(vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad = {};
    const vec3 normalNorth =  {1.0f, 0.0f, 0.0f};
    quad.vertices[0] = {vec3{ scale, -scale,  scale} + offset, normalNorth, colour};
    quad.vertices[1] = {vec3{ scale, -scale, -scale} + offset, normalNorth, colour};
    quad.vertices[2] = {vec3{ scale,  scale, -scale} + offset, normalNorth, colour};
    quad.vertices[3] = {vec3{ scale,  scale,  scale} + offset, normalNorth, colour};
    const uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;
    
    return quad;
}

static mpQuad mpCreateQuadSouth(vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad = {};
    const vec3 normalSouth =  {-1.0f, 0.0f, 0.0f};
    quad.vertices[0] = {vec3{-scale,  scale,  scale} + offset, normalSouth, colour};
    quad.vertices[1] = {vec3{-scale,  scale, -scale} + offset, normalSouth, colour};
    quad.vertices[2] = {vec3{-scale, -scale, -scale} + offset, normalSouth, colour};
    quad.vertices[3] = {vec3{-scale, -scale,  scale} + offset, normalSouth, colour};
    const uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;
    
    return quad;
}

static mpQuad mpCreateQuadEast(vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad = {};
    const vec3 normalEast =  {0.0f, 1.0f, 0.0f};
    quad.vertices[0] = {vec3{ scale,  scale,  scale} + offset, normalEast, colour};
    quad.vertices[1] = {vec3{ scale,  scale, -scale} + offset, normalEast, colour};
    quad.vertices[2] = {vec3{-scale,  scale, -scale} + offset, normalEast, colour};
    quad.vertices[3] = {vec3{-scale,  scale,  scale} + offset, normalEast, colour};
    const uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;
    
    return quad;
}

static mpQuad mpCreateQuadWest(vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad = {};
    const vec3 normalWest =  {0.0f, -1.0f, 0.0f};
    quad.vertices[0] = {vec3{-scale, -scale,  scale} + offset, normalWest, colour};
    quad.vertices[1] = {vec3{-scale, -scale, -scale} + offset, normalWest, colour};
    quad.vertices[2] = {vec3{ scale, -scale, -scale} + offset, normalWest, colour};
    quad.vertices[3] = {vec3{ scale, -scale,  scale} + offset, normalWest, colour};
    const uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;
    
    return quad;
}

static mpQuad mpCreateQuadTop(vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad = {};
    const vec3 normalTop = {0.0f, 0.0f, 1.0f};
    quad.vertices[0] = {vec3{-scale, -scale,  scale} + offset, normalTop, colour};
    quad.vertices[1] = {vec3{ scale, -scale,  scale} + offset, normalTop, colour};
    quad.vertices[2] = {vec3{ scale,  scale,  scale} + offset, normalTop, colour};
    quad.vertices[3] = {vec3{-scale,  scale,  scale} + offset, normalTop, colour};
    const uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;
    
    return quad;
}

static mpQuad mpCreateQuadBottom(vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad = {};
    const vec3 normalBottom =  {0.0f, 0.0f, -1.0f};
    quad.vertices[0] = {vec3{-scale,  scale, -scale} + offset, normalBottom, colour};
    quad.vertices[1] = {vec3{ scale,  scale, -scale} + offset, normalBottom, colour};
    quad.vertices[2] = {vec3{ scale, -scale, -scale} + offset, normalBottom, colour};
    quad.vertices[3] = {vec3{-scale, -scale, -scale} + offset, normalBottom, colour};
    const uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;
    
    return quad;
}

// TODO: Function needs to be cleaned up after verification
static mpMesh mpCreateChunkMesh(mpVoxelChunk *chunk)
{
    const uint16_t vertexStride = 4;
    const uint16_t indexStride = 6;
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
                uint32_t drawFlags = chunk->pBlocks[x][y][z].flags;
                if((drawFlags & VOXEL_FLAG_ACTIVE) == false)
                    continue;
                
                const vec4 colour = _mpBlockColours[chunk->pBlocks[x][y][z].type];
                const vec3 positionOffset = {
                    static_cast<float>(x) + chunk->position.x,
                    static_cast<float>(y) + chunk->position.y,
                    static_cast<float>(z) + chunk->position.z,
                };
                
                if(drawFlags & VOXEL_FLAG_DRAW_NORTH)
                {
                    const mpQuad quad = mpCreateQuadNorth(positionOffset, colour, MP_VOXEL_SCALE, vertexCount);
                    
                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);
                    
                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                    quadCount++;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_SOUTH)
                {
                    const mpQuad quad = mpCreateQuadSouth(positionOffset, colour, MP_VOXEL_SCALE, vertexCount);
                    
                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);
                    
                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                    quadCount++;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_EAST)
                {
                    const mpQuad quad = mpCreateQuadEast(positionOffset, colour, MP_VOXEL_SCALE, vertexCount);
                    
                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);
                    
                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                    quadCount++;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_WEST)
                {
                    const mpQuad quad = mpCreateQuadWest(positionOffset, colour, MP_VOXEL_SCALE, vertexCount);
                    
                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);
                    
                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                    quadCount++;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_TOP)
                {
                    const mpQuad quad = mpCreateQuadTop(positionOffset, colour, MP_VOXEL_SCALE, vertexCount);
                    
                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);
                    
                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                    quadCount++;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_BOTTOM)
                {
                    const mpQuad quad = mpCreateQuadBottom(positionOffset, colour, MP_VOXEL_SCALE, vertexCount);
                    
                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);
                    
                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                    quadCount++;
                }
            }
        }
    }
    mpMesh mesh = {};
    mesh.verticesSize = vertexCount * sizeof(mpVertex);
    mesh.indicesSize = indexCount * sizeof(uint16_t);
    mesh.vertices = static_cast<mpVertex*>(malloc(mesh.verticesSize));
    mesh.indices = static_cast<uint16_t*>(malloc(mesh.indicesSize));
    mesh.indexCount = indexCount;
    mesh.isEmpty = !(vertexCount && indexCount);
    
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

static mpVoxelChunk mpGenerateChunkTerrain(mpVoxelChunk chunk)
{
    // TODO: Use seed as paremeter to be able to generate different worlds
    float heightMap = 0.0f, globalX = 0.0f, globalY = 0.0f;
    
    for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++)
            {
                globalX = chunk.position.x + static_cast<float>(x);
                globalY = chunk.position.y + static_cast<float>(y);
                
                heightMap = Perlin(globalX / 100.0f, globalY / 100.0f);
                heightMap *= 400.0f;
                
                const uint32_t globalHeightCheck = (z + chunk.position.z) <= heightMap;
                chunk.pBlocks[x][y][z] = globalHeightCheck ? mpVoxelBlock{Voxel_Type_Grass, VOXEL_FLAG_ACTIVE} : mpVoxelBlock{Voxel_Type_Empty, 0};
            }
        }
    }
    return chunk;
}
// Loops through all blocks in a chunk and decides which faces need to be drawn based on neighbours active status
static mpVoxelChunk mpSetDrawFlags(mpVoxelChunk chunk)
{
    uint32_t max = MP_CHUNK_SIZE - 1;
    bool32 localCheck = 0;
    
    for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++)
            {
                mpVoxelBlock currentBlock = chunk.pBlocks[x][y][z];
                if((currentBlock.flags & VOXEL_FLAG_ACTIVE) == false)
                    continue;
                
                if(x > 0)
                    localCheck = chunk.pBlocks[x - 1][y][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_SOUTH)
                    localCheck = chunk.southNeighbour->pBlocks[max][y][z].flags & VOXEL_FLAG_ACTIVE;
                currentBlock.flags |= localCheck ? 0 : VOXEL_FLAG_DRAW_SOUTH;
                
                if(x < max)
                    localCheck = chunk.pBlocks[x + 1][y][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_NORTH)
                    localCheck = chunk.northNeighbour->pBlocks[0][y][z].flags & VOXEL_FLAG_ACTIVE;
                currentBlock.flags |= localCheck ? 0 : VOXEL_FLAG_DRAW_NORTH;
                
                if(y > 0)
                    localCheck = chunk.pBlocks[x][y - 1][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_WEST)
                    localCheck = chunk.westNeighbour->pBlocks[x][max][z].flags & VOXEL_FLAG_ACTIVE;
                currentBlock.flags |= localCheck ? 0 : VOXEL_FLAG_DRAW_WEST;
                
                if(y < max)
                    localCheck = chunk.pBlocks[x][y + 1][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_EAST)
                    localCheck = chunk.eastNeighbour->pBlocks[x][0][z].flags & VOXEL_FLAG_ACTIVE;
                currentBlock.flags |= localCheck ? 0 : VOXEL_FLAG_DRAW_EAST;
                
                if(z > 0)
                    localCheck = chunk.pBlocks[x][y][z - 1].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_BOTTOM)
                    localCheck = chunk.bottomNeighbour->pBlocks[x][y][max].flags & VOXEL_FLAG_ACTIVE;
                currentBlock.flags |= localCheck ? 0 : VOXEL_FLAG_DRAW_BOTTOM;
                
                if(z < max)
                    localCheck = chunk.pBlocks[x][y][z + 1].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_TOP)
                    localCheck = chunk.topNeighbour->pBlocks[x][y][0].flags & VOXEL_FLAG_ACTIVE;
                currentBlock.flags |= localCheck ? 0 : VOXEL_FLAG_DRAW_TOP;
                
                chunk.pBlocks[x][y][z] = currentBlock;
            }
        }
    }
    return chunk;
}

static mpWorldData mpGenerateWorldData()
{
    const grid3 worldSize = {5, 5, 5};
    
    mpWorldData worldData = {};
    worldData.chunkCount = worldSize.x * worldSize.y * worldSize.z;
    // This looks fugly, use memory arena instead
    worldData.chunks = static_cast<mpVoxelChunk*>(malloc(sizeof(mpVoxelChunk) * worldData.chunkCount));
    
    vec3 pos = {};
    
    for(uint32_t i = 0; i < worldData.chunkCount; i++)
    {
        worldData.chunks[i] = mpAllocateChunk();
        worldData.chunks[i].position = pos * MP_CHUNK_SIZE;
        worldData.chunks[i] = mpGenerateChunkTerrain(worldData.chunks[i]);
        
        if(pos.x > 0)
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_SOUTH;
            worldData.chunks[i].southNeighbour = &worldData.chunks[i - 1];
        }
        if(pos.x < (worldSize.x - 1))
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_NORTH;
            worldData.chunks[i].northNeighbour = &worldData.chunks[i + 1];
        }
        if(pos.y > 0)
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_WEST;
            worldData.chunks[i].westNeighbour = &worldData.chunks[i - worldSize.x];
        }
        if(pos.y < (worldSize.y - 1))
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_EAST;
            worldData.chunks[i].eastNeighbour = &worldData.chunks[i + worldSize.x];
        }
        if(pos.z > 0)
        {// TODO: unfuck top and bottom flag set
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_BOTTOM;
            worldData.chunks[i].bottomNeighbour = &worldData.chunks[i - (worldSize.x * worldSize.y)];
        }
        if(pos.z < (worldSize.z - 1))
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_TOP;
            worldData.chunks[i].topNeighbour = &worldData.chunks[i + (worldSize.x * worldSize.y)];
        }
        // This bit increments x,y and z as if the for loop was iterating over a 3D array
        pos.x++;
        pos = pos.x == worldSize.x ? vec3{0, ++pos.y, pos.z} : pos;
        pos = pos.y == worldSize.y ? vec3{pos.x, 0, ++pos.z} : pos;
    }
    
    pos = {};
    for(uint32_t i = 0; i < worldData.chunkCount; i++)
    {
        worldData.chunks[i] = mpSetDrawFlags(worldData.chunks[i]);
        // This bit increments x,y and z as if the for loop was iterating over a 3D array
        pos.x++;
        pos = pos.x == worldSize.x ? vec3{0, ++pos.y, pos.z} : pos;
        pos = pos.y == worldSize.y ? vec3{pos.x, 0, ++pos.z} : pos;
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
    
    printf("quadCount: %d\n", quadCount);
    
    return renderData;
}

inline static void ProcessKeyToCameraControl(const mpEventReceiver *receiver, mpKeyEvent key, bool32 *controlValue)
{
    if(receiver->keyPressedEvents & key)
        *controlValue = true;
    else if(receiver->keyReleasedEvents & key)
        *controlValue = false;
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
    
    // TODO: use memory for these bits of data
    engine.worldData = mpGenerateWorldData();
    engine.renderData = mpGenerateRenderData(&engine.worldData);
    
    mpVulkanInit(&engine, &vulkanMemory);
    
    engine.camera = {};
    engine.camera.speed = 10.0f;
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
        engine.camera.projection = Perspective(engine.camera.fov, static_cast<float>(engine.windowInfo.width) / static_cast<float>(engine.windowInfo.height), 0.1f, 100.0f);
        
        mpVulkanUpdate(&engine, &vulkanMemory);
        
        engine.windowInfo.hasResized = false;
        ResetEventReceiver(&engine.eventReceiver);
        timestep = PlatformUpdateClock(&lastCounter, perfCountFrequency);
        UpdateFpsSampler(&engine.fpsSampler, timestep);
        mpDbgProcessSampledRecords(5000);
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