#include "core.h"
#include "Win32_Mariposa.h"
#include "VulkanLayer.h"

#include <stdlib.h>
#include <stdio.h>

enum mpGlobalConstants
{
    MP_CHUNK_SIZE = 20,
};

static mpVoxelChunk mpAllocateChunk(mpMemoryRegion *worldGenRegion)
{
// TODO: This function is ugly as hell, consider redoing some of this stuff
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
// TODO: consider creating a general purpose quad creation function that takes in some structure and spits out a quad
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
// mpCreateMesh resets tempMemory after use.
static mpMesh mpCreateMesh(mpVoxelChunk *chunk, mpMemoryRegion *meshRegion, mpMemoryRegion *tempMemory)
{
    static const float VOXEL_SCALE = 0.5f;
    const uint16_t vertexStride = 4;
    const uint16_t indexStride = 6;
    uint16_t vertexCount = 0, indexCount = 0;

    static const size_t TEMP_VERT_BLOCK_SIZE = sizeof(mpVertex) * 12 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    mpVertex *tempBlockVertices = static_cast<mpVertex*>(mpAllocateIntoRegion(tempMemory, TEMP_VERT_BLOCK_SIZE));
    mpVertex *tempBlockVertIncrementer = tempBlockVertices;

    static const size_t TEMP_INDEX_BLOCK_SIZE = sizeof(uint16_t) * 18 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    uint16_t *tempBlockIndices = static_cast<uint16_t*>(mpAllocateIntoRegion(tempMemory, TEMP_INDEX_BLOCK_SIZE));
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
                    const mpQuad quad = mpCreateQuadNorth(positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_SOUTH)
                {
                    const mpQuad quad = mpCreateQuadSouth(positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_EAST)
                {
                    const mpQuad quad = mpCreateQuadEast(positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_WEST)
                {
                    const mpQuad quad = mpCreateQuadWest(positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_TOP)
                {
                    const mpQuad quad = mpCreateQuadTop(positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_BOTTOM)
                {
                    const mpQuad quad = mpCreateQuadBottom(positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
            }
        }
    }
    mpMesh mesh = {};
    mesh.vertexCount = vertexCount;
    mesh.indexCount = indexCount;
    mesh.memReg = meshRegion;

    mpResetMemoryRegion(meshRegion);
    const size_t verticesSize = vertexCount * sizeof(mpVertex);
    const size_t indicesSize = indexCount * sizeof(uint16_t);

    mesh.vertices = static_cast<mpVertex*>(mpAllocateIntoRegion(mesh.memReg, verticesSize));
    mesh.indices = static_cast<uint16_t*>(mpAllocateIntoRegion(mesh.memReg, indicesSize));

    memcpy(mesh.vertices, tempBlockVertices, verticesSize);
    memcpy(mesh.indices, tempBlockIndices, indicesSize);
    mpResetMemoryRegion(tempMemory);

    return mesh;
}

static mpVoxelChunk mpGenerateChunkTerrain(mpVoxelChunk chunk)
{
    float noiseValue = 0.0f;
    vec3 globalPos = {};
    const float threshold = -0.5f;
    const float typeThreshold = -1.0f;
    const float noiseFactor = 10.0f;

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                globalPos.x = chunk.position.x + static_cast<float>(x);
                globalPos.y = chunk.position.y + static_cast<float>(y);
                globalPos.z = chunk.position.z + static_cast<float>(z);

                noiseValue = noiseFactor * perlin3Dmap(globalPos / noiseFactor);

                if(noiseValue < threshold)
                    chunk.pBlocks[x][y][z].flags = VOXEL_FLAG_ACTIVE;
                chunk.pBlocks[x][y][z].type = noiseValue < typeThreshold ? Voxel_Type_Grass : Voxel_Type_Grass2;
            }
        }
    }
    return chunk;
}
// Loops through all blocks in a chunk and decides which faces need to be drawn based on neighbours active status
static mpVoxelChunk mpSetDrawFlags(mpVoxelChunk chunk)
{
    const uint32_t max = MP_CHUNK_SIZE - 1;
    bool32 localCheck = 0;
    uint32_t voxelFlags = 0;

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                voxelFlags = chunk.pBlocks[x][y][z].flags;
                if((voxelFlags & VOXEL_FLAG_ACTIVE) == false)
                    continue;

                if(x > 0)
                    localCheck = chunk.pBlocks[x - 1][y][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_SOUTH)
                    localCheck = chunk.southNeighbour->pBlocks[max][y][z].flags & VOXEL_FLAG_ACTIVE;
                voxelFlags |= localCheck ? 0 : VOXEL_FLAG_DRAW_SOUTH;

                if(x < max)
                    localCheck = chunk.pBlocks[x + 1][y][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_NORTH)
                    localCheck = chunk.northNeighbour->pBlocks[0][y][z].flags & VOXEL_FLAG_ACTIVE;
                voxelFlags |= localCheck ? 0 : VOXEL_FLAG_DRAW_NORTH;

                if(y > 0)
                    localCheck = chunk.pBlocks[x][y - 1][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_WEST)
                    localCheck = chunk.westNeighbour->pBlocks[x][max][z].flags & VOXEL_FLAG_ACTIVE;
                voxelFlags |= localCheck ? 0 : VOXEL_FLAG_DRAW_WEST;

                if(y < max)
                    localCheck = chunk.pBlocks[x][y + 1][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_EAST)
                    localCheck = chunk.eastNeighbour->pBlocks[x][0][z].flags & VOXEL_FLAG_ACTIVE;
                voxelFlags |= localCheck ? 0 : VOXEL_FLAG_DRAW_EAST;

                if(z > 0)
                    localCheck = chunk.pBlocks[x][y][z - 1].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_BOTTOM)
                    localCheck = chunk.bottomNeighbour->pBlocks[x][y][max].flags & VOXEL_FLAG_ACTIVE;
                voxelFlags |= localCheck ? 0 : VOXEL_FLAG_DRAW_BOTTOM;

                if(z < max)
                    localCheck = chunk.pBlocks[x][y][z + 1].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_TOP)
                    localCheck = chunk.topNeighbour->pBlocks[x][y][0].flags & VOXEL_FLAG_ACTIVE;
                voxelFlags |= localCheck ? 0 : VOXEL_FLAG_DRAW_TOP;

                chunk.pBlocks[x][y][z].flags = voxelFlags;
            }
        }
    }

    return chunk;
}

static mpWorldData mpGenerateWorldData(const grid3 worldSize, mpMemoryRegion *chunkMemory)
{
    mpWorldData worldData = {};
    worldData.chunkCount = worldSize.x * worldSize.y * worldSize.z;
    worldData.chunks = static_cast<mpVoxelChunk*>(mpAllocateIntoRegion(chunkMemory, sizeof(mpVoxelChunk) * worldData.chunkCount));

    vec3 pos = {};

    for(uint32_t i = 0; i < worldData.chunkCount; i++)
    {
        worldData.chunks[i] = mpAllocateChunk(chunkMemory);
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

static mpRenderData mpGenerateRenderData(const mpWorldData *worldData, mpMemoryPool *meshPool, mpMemoryRegion *meshHeaderMemory, mpMemoryRegion *tempMemory)
{
    mpRenderData renderData = {};

    renderData.meshCount = worldData->chunkCount;
    renderData.meshes = static_cast<mpMesh*>(mpAllocateIntoRegion(meshHeaderMemory, sizeof(mpMesh) * renderData.meshCount));

    for(uint32_t i = 0; i < renderData.meshCount; i++)
    {
        mpMemoryRegion *meshRegion = mpGetMemoryRegion(meshPool);
        renderData.meshes[i] = mpCreateMesh(&worldData->chunks[i], meshRegion, tempMemory);
    }
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
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_UP, &cameraControls->rUp);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_DOWN, &cameraControls->rDown);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_LEFT, &cameraControls->rLeft);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_RIGHT, &cameraControls->rRight);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_W, &cameraControls->tForward);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_S, &cameraControls->tBackward);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_A, &cameraControls->tLeft);
    ProcessKeyToCameraControl(eventReceiver, MP_KEY_D, &cameraControls->tRight);
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

// NOTE: could perhaps return raycasthit data or smth
static mpVoxel* mpRaycast(mpCamera *cam, mpWorldData *worldData, vec3 direction)
{
    const vec3 chunkDiagonal = {MP_CHUNK_SIZE, MP_CHUNK_SIZE, MP_CHUNK_SIZE};
    vec3 raycastHit = cam->position + direction;

    mpVoxel *result = nullptr;
    for(uint32_t i = 0; i < worldData->chunkCount; i++)
    {
        const vec3 chunkCorner = worldData->chunks[i].position + chunkDiagonal;
        // Simple bounding box search to find which chunk the destination is located within
        if(raycastHit.x < worldData->chunks[i].position.x)
            continue;
        if(raycastHit.x > chunkCorner.x)
            continue;

        if(raycastHit.y < worldData->chunks[i].position.y)
            continue;
        if(raycastHit.y > chunkCorner.y)
            continue;

        if(raycastHit.z < worldData->chunks[i].position.z)
            continue;
        if(raycastHit.z > chunkCorner.z)
            continue;

        // Convert raycastHit to local chunk space
        raycastHit -= worldData->chunks[i].position;
        // Use that as indices to the pBlocks array
        const uint32_t x = static_cast<uint32_t>(raycastHit.x);
        const uint32_t y = static_cast<uint32_t>(raycastHit.y);
        const uint32_t z = static_cast<uint32_t>(raycastHit.z);
        result = &worldData->chunks[i].pBlocks[x][y][z];
        worldData->chunks[i].flags |= CHUNK_FLAG_IS_DIRTY;
        break;
    }
    return result;
}

int main(int argc, char *argv[])
{
    mpCore core;
    memset(&core, 0, sizeof(mpCore));
    core.name = "Mariposa 3D Voxel Engine";

    PlatformCreateWindow(&core.windowInfo, core.name);
    // TODO: Prepare win32 sound
    core.callbacks = PlatformGetCallbacks();
    // TODO: allocate pool header on heap as well
    mpMemoryPool smallPool = mpCreateMemoryPool(10, MegaBytes(10), 2);

    mpMemoryRegion *chunkMemory = mpGetMemoryRegion(&smallPool);
    mpMemoryRegion *meshHeaderMemory = mpGetMemoryRegion(&smallPool);
    mpMemoryRegion *tempMemory = mpGetMemoryRegion(&smallPool);

    const grid3 worldSize = {5, 5, 5};
    core.worldData = mpGenerateWorldData(worldSize, chunkMemory);

    mpMemoryPool meshPool = mpCreateMemoryPool(core.worldData.chunkCount, MegaBytes(2), 42);
    core.renderData = mpGenerateRenderData(&core.worldData, &meshPool, meshHeaderMemory, tempMemory);

    mpMemoryRegion *vulkanMemory = mpGetMemoryRegion(&smallPool);
    mpVulkanInit(&core, vulkanMemory);

    // MP_LOG_TRACE
    // printf("chunkMemory uses %zu out of %zu kB\n", (chunkMemory->dataSize / 1000), (chunkMemory->regionSize) / 1000);
    // printf("meshMemory uses %zu out of %zu kB\n", (meshMemory->dataSize / 1000), (meshMemory->regionSize) / 1000);

    core.camera.speed = 10.0f;
    core.camera.sensitivity = 2.0f;
    core.camera.fov = PI32 / 3.0f;
    core.camera.model = Mat4x4Identity();
    core.camera.position = vec3{12.0f, 12.0f, 12.0f};
    core.camera.pitchClamp = (PI32 / 2.0f) - 0.01f;

    core.fpsSampler.level = 1000;

    float timestep = 0.0f;
    int64_t lastCounter = 0, perfCountFrequency = 0;
    PlatformPrepareClock(&lastCounter, &perfCountFrequency);

    core.windowInfo.running = true;
    core.windowInfo.hasResized = false; // Windows likes to set this to true at startup
    while(core.windowInfo.running)
    {
        PlatformPollEvents(&core.eventReceiver);

        UpdateCameraControlState(&core.eventReceiver, &core.camControls);

        // Increment camera transform
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

        // Update view & projection matrices
        const vec3 front = {cosf(core.camera.pitch) * cosf(core.camera.yaw), cosf(core.camera.pitch) * sinf(core.camera.yaw), sinf(core.camera.pitch)};
        const vec3 left = {sinf(core.camera.yaw), -cosf(core.camera.yaw), 0.0f};
        const vec3 up = {0.0f, 0.0f, 1.0f};
        if(core.camControls.tForward)
            core.camera.position += front * core.camera.speed * timestep;
        else if(core.camControls.tBackward)
            core.camera.position -= front * core.camera.speed * timestep;
        if(core.camControls.tLeft)
            core.camera.position -= left * core.camera.speed * timestep;
        else if(core.camControls.tRight)
            core.camera.position += left * core.camera.speed * timestep;

        core.camera.view = LookAt(core.camera.position, core.camera.position + front, up);
        core.camera.projection = Perspective(core.camera.fov, static_cast<float>(core.windowInfo.width) / static_cast<float>(core.windowInfo.height), 0.1f, 100.0f);

        if(core.eventReceiver.keyPressedEvents & MP_KEY_F)
        {
            mpVoxel *raycastHit = mpRaycast(&core.camera, &core.worldData, front * 3.0f);
            if(raycastHit)
                raycastHit->flags ^= VOXEL_FLAG_ACTIVE;
        }
        // Recreate mesh for dirty chunks
        for(uint32_t i = 0; i < core.worldData.chunkCount; i++)
        {
            if(core.worldData.chunks[i].flags & CHUNK_FLAG_IS_DIRTY)
            {
                // TODO: need a drawFlags function that sets flags for a single voxel instead of redoing the whole chunk
                core.worldData.chunks[i] = mpSetDrawFlags(core.worldData.chunks[i]);
                core.renderData.meshes[i] = mpCreateMesh(&core.worldData.chunks[i], core.renderData.meshes[i].memReg, tempMemory);
                core.worldData.chunks[i].flags ^= CHUNK_FLAG_IS_DIRTY;
                mpVulkanRecreateGeometryBuffer(core.rendererHandle, &core.renderData.meshes[i], i);
                core.renderFlags |= MP_RENDER_FLAG_REDRAW_MESHES;
            }
        }

        mpVulkanUpdate(&core, vulkanMemory);

        core.windowInfo.hasResized = false;
        ResetEventReceiver(&core.eventReceiver);
        timestep = PlatformUpdateClock(&lastCounter, perfCountFrequency);
        UpdateFpsSampler(&core.fpsSampler, timestep);
        mpDbgProcessSampledRecords(5000);
    }

    mpVulkanCleanup(&core.rendererHandle, core.renderData.meshCount);

    mpDestroyMemoryPool(&meshPool);
    mpDestroyMemoryPool(&smallPool);

    return 0;
}
