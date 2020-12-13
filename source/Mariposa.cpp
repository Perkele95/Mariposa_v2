#include "core.h"
#include "Win32_Mariposa.h"
#include "VulkanLayer.h"
#include "profiler.h"

#include <stdlib.h>
#include <stdio.h>

#define MP_CHUNK_SIZE 16
#define MP_VOXEL_SCALE 0.5f

static uint32_t quadCount = 0;
// TODO: This function is ugly as hell, consider redoing some of this stuff
static mpVoxelChunk mpAllocateChunk(mpMemoryRegion *worldGenRegion)
{
    static const size_t chunkSize2x = MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    static const size_t chunkSize3x = MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    mpVoxelChunk chunk = {};
    chunk.size = MP_CHUNK_SIZE;
    chunk.pBlocks = static_cast<mpVoxel***>(mpAllocateIntoRegion(worldGenRegion, sizeof(mpVoxel**) * MP_CHUNK_SIZE));
    *chunk.pBlocks = static_cast<mpVoxel**>(mpAllocateIntoRegion(worldGenRegion, sizeof(mpVoxel*) * chunkSize2x));
    **chunk.pBlocks = static_cast<mpVoxel*>(mpAllocateIntoRegion(worldGenRegion, sizeof(mpVoxel) * chunkSize3x));

    // Pointer to pointer assignment
    for(uint32_t i = 1; i < MP_CHUNK_SIZE; i++)
        chunk.pBlocks[i] = &chunk.pBlocks[0][0] + i * MP_CHUNK_SIZE;
    // Pointer assignment
    for(uint32_t i = 0; i < MP_CHUNK_SIZE; i++)
        for(uint32_t k = 0; k < MP_CHUNK_SIZE; k++)
            chunk.pBlocks[i][k] = (&chunk.pBlocks[0][0][0]) + i * chunkSize2x + k * MP_CHUNK_SIZE;

    return chunk;
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
// TODO: consider creating a general purpose quad creation function that takes in some structure and spits out a quad
// Perhaps a struct to hold the arrays could work
static mpMesh mpCreateChunkMesh(mpVoxelChunk *chunk, mpMemoryRegion *worldGenRegion)
{
    const uint16_t vertexStride = 4;
    const uint16_t indexStride = 6;
    uint16_t vertexCount = 0, indexCount = 0;

    static const size_t tempVertBlockSize = sizeof(mpVertex) * 12 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    mpVertex *tempBlockVertices = static_cast<mpVertex*>(malloc(tempVertBlockSize));// TODO: this temp block could utilies transient memory
    memset(tempBlockVertices, 0, tempVertBlockSize);
    mpVertex *tempBlockVertIncrementer = tempBlockVertices;

    static const size_t tempIndexBlockSize = sizeof(uint16_t) * 18 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    uint16_t *tempBlockIndices = static_cast<uint16_t*>(malloc(tempIndexBlockSize));
    memset(tempBlockIndices, 0, tempIndexBlockSize);
    uint16_t *tempBlockIndexIncrementer = tempBlockIndices;

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
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
    mesh.vertices = static_cast<mpVertex*>(mpAllocateIntoRegion(worldGenRegion, mesh.verticesSize));
    mesh.indices = static_cast<uint16_t*>(mpAllocateIntoRegion(worldGenRegion, mesh.indicesSize));
    mesh.indexCount = indexCount;
    mesh.isEmpty = !(vertexCount && indexCount);

    memcpy(mesh.vertices, tempBlockVertices, mesh.verticesSize);
    memcpy(mesh.indices, tempBlockIndices, mesh.indicesSize);
    free(tempBlockVertices);
    free(tempBlockIndices);

    return mesh;
}

static mpVoxelChunk mpGenerateChunkTerrain(mpVoxelChunk chunk)
{
    float noiseValue = 0.0f;
    vec3 globalPos = {};
    const float threshold = -0.5f;
    const float typeThreshold = -1.0f;

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                globalPos.x = chunk.position.x + static_cast<float>(x);
                globalPos.y = chunk.position.y + static_cast<float>(y);
                globalPos.z = chunk.position.z + static_cast<float>(z);

                noiseValue = 20.0f * perlin3Dmap(globalPos / 10.0f);

                chunk.pBlocks[x][y][z].flags = noiseValue < threshold ? VOXEL_FLAG_ACTIVE : 0;
                chunk.pBlocks[x][y][z].type = noiseValue < typeThreshold ? Voxel_Type_Grass : Voxel_Type_Grass2;

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

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                mpVoxel currentBlock = chunk.pBlocks[x][y][z];
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

static mpWorldData mpGenerateWorldData(mpMemoryRegion *worldGenRegion)
{
    const grid3 worldSize = {5, 5, 5};

    mpWorldData worldData = {};
    worldData.chunkCount = worldSize.x * worldSize.y * worldSize.z;
    worldData.chunks = static_cast<mpVoxelChunk*>(mpAllocateIntoRegion(worldGenRegion, sizeof(mpVoxelChunk) * worldData.chunkCount));

    vec3 pos = {};

    for(uint32_t i = 0; i < worldData.chunkCount; i++)
    {
        worldData.chunks[i] = mpAllocateChunk(worldGenRegion);
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
        {
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

static mpRenderData mpGenerateRenderData(const mpWorldData *worldData, mpMemoryRegion *worldGenRegion)
{
    mpRenderData renderData = {};

    renderData.meshCount = worldData->chunkCount;
    renderData.meshes = static_cast<mpMesh*>(mpAllocateIntoRegion(worldGenRegion, sizeof(mpMesh) * renderData.meshCount));

    for(uint32_t i = 0; i < renderData.meshCount; i++)
        renderData.meshes[i] = mpCreateChunkMesh(&worldData->chunks[i], worldGenRegion);

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
    mpCore core = {};
    core.name = "Mariposa";
    core.windowInfo = {};

    PlatformCreateWindow(&core.windowInfo);
    // TODO: Prepare win32 sound
    core.callbacks = PlatformGetCallbacks();

    mpMemoryPool bigPool = mpCreateMemoryPool(1, MegaBytes(32));
    mpMemoryPool smallPool = mpCreateMemoryPool(10, MegaBytes(1));

    mpMemoryRegion *worldGenRegion = mpGetMemoryRegion(&bigPool);
    mpMemoryRegion *vulkanRegion = mpGetMemoryRegion(&smallPool);
    mpMemoryRegion *initRegion = mpGetMemoryRegion(&smallPool);

    core.worldData = mpGenerateWorldData(worldGenRegion);
    core.renderData = mpGenerateRenderData(&core.worldData, worldGenRegion);

    mpVulkanInit(&core, vulkanRegion);

    MP_LOG_TRACE
    printf("vulkanRegion uses %zu out of %zu kB\n", (vulkanRegion->dataSize / 1000), (vulkanRegion->regionSize) / 1000);

    core.camera = {};
    core.camera.speed = 10.0f;
    core.camera.sensitivity = 2.0f;
    core.camera.fov = PI32 / 3.0f;
    core.camera.model = Mat4x4Identity();
    core.camera.position = vec3{2.0f, 2.0f, 2.0f};
    core.camera.pitchClamp = (PI32 / 2.0f) - 0.01f;

    core.camControls = {};
    core.eventReceiver = {};

    core.fpsSampler = {};
    core.fpsSampler.level = 1000;

    float timestep = 0.0f;
    int64_t lastCounter = 0, perfCountFrequency = 0;
    PlatformPrepareClock(&lastCounter, &perfCountFrequency);

    core.windowInfo.running = true;
    core.windowInfo.hasResized = false; // WM_SIZE is triggered at startup, so we need to reset hasResized before the loop
    while(core.windowInfo.running)
    {
        PlatformPollEvents(&core.eventReceiver);

        UpdateCameraControlState(&core.eventReceiver, &core.camControls);

        if(core.camControls.rUp)
            core.camera.pitch += core.camera.sensitivity * timestep;
        else if(core.camControls.rDown)
            core.camera.pitch -= core.camera.sensitivity * timestep;
        if(core.camControls.rLeft)
            core.camera.yaw += core.camera.sensitivity * timestep;
        else if(core.camControls.rRight)
            core.camera.yaw -= core.camera.sensitivity * timestep;

        if(core.camera.pitch > core.camera.pitchClamp)
            core.camera.pitch = core.camera.pitchClamp;
        else if(core.camera.pitch < -core.camera.pitchClamp)
            core.camera.pitch = -core.camera.pitchClamp;

        const vec3 front = {cosf(core.camera.pitch) * cosf(core.camera.yaw), cosf(core.camera.pitch) * sinf(core.camera.yaw), sinf(core.camera.pitch)};
        const vec3 left = {sinf(core.camera.yaw), -cosf(core.camera.yaw), 0.0f};
        if(core.camControls.tForward)
            core.camera.position += front * core.camera.speed * timestep;
        else if(core.camControls.tBackward)
            core.camera.position -= front * core.camera.speed * timestep;
        if(core.camControls.tLeft)
            core.camera.position -= left * core.camera.speed * timestep;
        else if(core.camControls.tRight)
            core.camera.position += left * core.camera.speed * timestep;

        core.camera.view = LookAt(core.camera.position, core.camera.position + front, {0.0f, 0.0f, 1.0f});
        core.camera.projection = Perspective(core.camera.fov, static_cast<float>(core.windowInfo.width) / static_cast<float>(core.windowInfo.height), 0.1f, 100.0f);

        mpVulkanUpdate(&core, vulkanRegion);

        core.windowInfo.hasResized = false;
        ResetEventReceiver(&core.eventReceiver);
        timestep = PlatformUpdateClock(&lastCounter, perfCountFrequency);
        UpdateFpsSampler(&core.fpsSampler, timestep);
        mpDbgProcessSampledRecords(5000);
    }

    mpVulkanCleanup(&core.rendererHandle, core.renderData.meshCount);

    mpDestroyMemoryPool(&bigPool);
    mpDestroyMemoryPool(&smallPool);

    return 0;
}
