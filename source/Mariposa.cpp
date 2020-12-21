#include "Mariposa.h"
#include "Win32_Mariposa.h"
#include "VulkanLayer.h"

#include <stdlib.h>
#include <stdio.h>

constexpr static mpQuadFaceArray mpQuadFaceNorth()
{
    constexpr float scale = 1.0f;
    constexpr mpQuadFaceArray arr = {
    vec3{ scale, -scale,  scale},
    vec3{ scale, -scale, -scale},
    vec3{ scale,  scale, -scale},
    vec3{ scale,  scale,  scale},
    vec3{1.0f, 0.0f, 0.0f}};
    return arr;
}

constexpr static mpQuadFaceArray mpQuadFaceSouth()
{
    constexpr float scale = 1.0f;
    constexpr mpQuadFaceArray arr = {
    vec3{-scale,  scale,  scale},
    vec3{-scale,  scale, -scale},
    vec3{-scale, -scale, -scale},
    vec3{-scale, -scale,  scale},
    vec3{-1.0f, 0.0f, 0.0f}};
    return arr;
}

constexpr static mpQuadFaceArray mpQuadFaceEast()
{
    constexpr float scale = 1.0f;
    constexpr mpQuadFaceArray arr = {
    vec3{ scale,  scale,  scale},
    vec3{ scale,  scale, -scale},
    vec3{-scale,  scale, -scale},
    vec3{-scale,  scale,  scale},
    vec3{0.0f, 1.0f, 0.0f}};
    return arr;
}

constexpr static mpQuadFaceArray mpQuadFaceWest()
{
    constexpr float scale = 1.0f;
    constexpr mpQuadFaceArray arr = {
    vec3{-scale, -scale,  scale},
    vec3{-scale, -scale, -scale},
    vec3{ scale, -scale, -scale},
    vec3{ scale, -scale,  scale},
    vec3{0.0f, -1.0f, 0.0f}};
    return arr;
}

constexpr static mpQuadFaceArray mpQuadFaceTop()
{
    constexpr float scale = 1.0f;
    constexpr mpQuadFaceArray arr = {
    vec3{-scale, -scale,  scale},
    vec3{ scale, -scale,  scale},
    vec3{ scale,  scale,  scale},
    vec3{-scale,  scale,  scale},
    vec3{0.0f, 0.0f, 1.0f}};
    return arr;
}

constexpr static mpQuadFaceArray mpQuadFaceBottom()
{
    constexpr float scale = 1.0f;
    constexpr mpQuadFaceArray arr = {
    vec3{-scale,  scale, -scale},
    vec3{ scale,  scale, -scale},
    vec3{ scale, -scale, -scale},
    vec3{-scale, -scale, -scale},
    vec3{0.0f, 0.0f, -1.0f}};
    return arr;
}

static mpQuad mpCreateQuad(mpQuadFaceArray (*getQuadFace)(), vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad;
    const mpQuadFaceArray quadFace = getQuadFace();
    quad.vertices[0] = {(quadFace.data[0] * scale) + offset, quadFace.normal, colour};
    quad.vertices[1] = {(quadFace.data[1] * scale) + offset, quadFace.normal, colour};
    quad.vertices[2] = {(quadFace.data[2] * scale) + offset, quadFace.normal, colour};
    quad.vertices[3] = {(quadFace.data[3] * scale) + offset, quadFace.normal, colour};
    constexpr uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;

    return quad;
}

static size_t largestChunkSize = 0;
// mpCreateMesh resets tempMemory after use.
static mpMesh mpCreateMesh(mpChunk *chunk, mpMemoryRegion *meshRegion, mpMemoryRegion *tempMemory, mpVoxelTypeDictionary *typeDict)
{
    constexpr float VOXEL_SCALE = 0.5f;
    constexpr uint16_t vertexStride = 4;
    constexpr uint16_t indexStride = 6;
    uint16_t vertexCount = 0, indexCount = 0;

    constexpr size_t TEMP_VERT_BLOCK_SIZE = sizeof(mpVertex) * 12 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    mpVertex *tempBlockVertices = static_cast<mpVertex*>(mpAllocateIntoRegion(tempMemory, TEMP_VERT_BLOCK_SIZE));
    mpVertex *tempBlockVertIncrementer = tempBlockVertices;

    constexpr size_t TEMP_INDEX_BLOCK_SIZE = sizeof(uint16_t) * 18 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    uint16_t *tempBlockIndices = static_cast<uint16_t*>(mpAllocateIntoRegion(tempMemory, TEMP_INDEX_BLOCK_SIZE));
    uint16_t *tempBlockIndexIncrementer = tempBlockIndices;

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                uint32_t drawFlags = chunk->voxels[x][y][z].flags;
                if((drawFlags & VOXEL_FLAG_ACTIVE) == false)
                    continue;

                const vec4 colour = mpVoxelDictionaryGetValue(typeDict, chunk->voxels[x][y][z].type);
                const vec3 positionOffset = {
                    static_cast<float>(x) + chunk->position.x,
                    static_cast<float>(y) + chunk->position.y,
                    static_cast<float>(z) + chunk->position.z,
                };

                if(drawFlags & VOXEL_FLAG_DRAW_NORTH)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceNorth, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_SOUTH)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceSouth, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_EAST)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceEast, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_WEST)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceWest, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_TOP)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceTop, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, sizeof(mpVertex) * vertexStride);
                    memcpy(tempBlockIndexIncrementer, quad.indices, sizeof(uint16_t) * indexStride);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(drawFlags & VOXEL_FLAG_DRAW_BOTTOM)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceBottom, positionOffset, colour, VOXEL_SCALE, vertexCount);

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
    if(mesh.memReg->dataSize > largestChunkSize)
        largestChunkSize += mesh.memReg->dataSize;

    memcpy(mesh.vertices, tempBlockVertices, verticesSize);
    memcpy(mesh.indices, tempBlockIndices, indicesSize);
    mpResetMemoryRegion(tempMemory);

    return mesh;
}
// TODO: function should take a seed to generate multiple worlds as well as regenerate the same world from a seed
static mpChunk mpGenerateChunkTerrain(mpChunk chunk)
{
    /* 3D Noise constants
    const float threshold = -0.2f;
    const float typeThreshold = -1.0f;
    const float noiseFactor = 10.0f;
    */
    constexpr float surfaceLevel = 40.0f;
    constexpr float granularity = 0.05f;
    constexpr float moistureFactor = 0.4f;
    constexpr float heightMult = 7.0f;

    vec3 globalPos = {};

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                globalPos.x = chunk.position.x + static_cast<float>(x);
                globalPos.y = chunk.position.y + static_cast<float>(y);
                globalPos.z = chunk.position.z + static_cast<float>(z);

                const float moistureThreshold = perlin(globalPos.x * moistureFactor, globalPos.y * moistureFactor);
                const float surfaceThreshold = moistureThreshold + surfaceLevel + heightMult * perlin(globalPos.x * granularity, globalPos.y * granularity);

                if(globalPos.z > surfaceThreshold)
                    continue;
                chunk.voxels[x][y][z].flags = VOXEL_FLAG_ACTIVE;
                chunk.voxels[x][y][z].type = VOXEL_TYPE_GRASS;
                /*
                const float noiseValue = noiseFactor * perlin3Dmap(globalPos / noiseFactor);

                if(noiseValue < threshold)
                    chunk.voxels[x][y][z].flags = VOXEL_FLAG_ACTIVE;
                chunk.voxels[x][y][z].type = noiseValue < typeThreshold ? VOXEL_TYPE_GRASS : VOXEL_TYPE_DIRT;
                */
            }
        }
    }
    return chunk;
}
// Loops through all blocks in a chunk and decides which faces need to be drawn based on neighbours active status
static mpChunk mpSetDrawFlags(mpChunk chunk)
{
    constexpr uint32_t max = MP_CHUNK_SIZE - 1;
    bool32 localCheck = 0;
    uint32_t voxelFlags = 0;

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                voxelFlags = chunk.voxels[x][y][z].flags;
                if((voxelFlags & VOXEL_FLAG_ACTIVE) == false)
                    continue;

                if(x > 0)
                    localCheck = chunk.voxels[x - 1][y][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_SOUTH)
                    localCheck = chunk.southNeighbour->voxels[max][y][z].flags & VOXEL_FLAG_ACTIVE;
                if(localCheck == false)
                    voxelFlags |= VOXEL_FLAG_DRAW_SOUTH;

                if(x < max)
                    localCheck = chunk.voxels[x + 1][y][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_NORTH)
                    localCheck = chunk.northNeighbour->voxels[0][y][z].flags & VOXEL_FLAG_ACTIVE;
                if(localCheck == false)
                    voxelFlags |= VOXEL_FLAG_DRAW_NORTH;

                if(y > 0)
                    localCheck = chunk.voxels[x][y - 1][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_WEST)
                    localCheck = chunk.westNeighbour->voxels[x][max][z].flags & VOXEL_FLAG_ACTIVE;
                if(localCheck == false)
                    voxelFlags |= VOXEL_FLAG_DRAW_WEST;

                if(y < max)
                    localCheck = chunk.voxels[x][y + 1][z].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_EAST)
                    localCheck = chunk.eastNeighbour->voxels[x][0][z].flags & VOXEL_FLAG_ACTIVE;
                if(localCheck == false)
                    voxelFlags |= VOXEL_FLAG_DRAW_EAST;

                if(z > 0)
                    localCheck = chunk.voxels[x][y][z - 1].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_BOTTOM)
                    localCheck = chunk.bottomNeighbour->voxels[x][y][max].flags & VOXEL_FLAG_ACTIVE;
                if(localCheck == false)
                    voxelFlags |= VOXEL_FLAG_DRAW_BOTTOM;

                if(z < max)
                    localCheck = chunk.voxels[x][y][z + 1].flags & VOXEL_FLAG_ACTIVE;
                else if(chunk.flags & CHUNK_FLAG_NEIGHBOUR_TOP)
                    localCheck = chunk.topNeighbour->voxels[x][y][0].flags & VOXEL_FLAG_ACTIVE;
                if(localCheck == false)
                    voxelFlags |= VOXEL_FLAG_DRAW_TOP;

                chunk.voxels[x][y][z].flags = voxelFlags;
            }
        }
    }

    return chunk;
}

static mpWorldData mpGenerateWorldData(const grid3 worldSize, mpMemoryRegion *chunkMemory)
{
    mpWorldData worldData = {};
    worldData.chunkCount = worldSize.x * worldSize.y * worldSize.z;
    worldData.chunks = static_cast<mpChunk*>(mpAllocateIntoRegion(chunkMemory, sizeof(mpChunk) * worldData.chunkCount));

    vec3 pos = {};

    for(uint32_t i = 0; i < worldData.chunkCount; i++)
    {
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

static mpRenderData mpGenerateRenderData(const mpWorldData *worldData, mpMemoryPool *meshPool, mpMemoryRegion *meshHeaderMemory, mpMemoryRegion *tempMemory, mpVoxelTypeDictionary *typeDict)
{
    mpRenderData renderData = {};

    renderData.meshCount = worldData->chunkCount;
    renderData.meshes = static_cast<mpMesh*>(mpAllocateIntoRegion(meshHeaderMemory, sizeof(mpMesh) * renderData.meshCount));

    for(uint32_t i = 0; i < renderData.meshCount; i++)
    {
        mpMemoryRegion *meshRegion = mpGetMemoryRegion(meshPool);
        renderData.meshes[i] = mpCreateMesh(&worldData->chunks[i], meshRegion, tempMemory, typeDict);
    }
    return renderData;
}

static void mpUpdateCameraControlState(const mpEventReceiver *eventReceiver, mpCameraControls *cameraControls)
{
    constexpr mpKeyEvent keyList[] = {
        MP_KEY_UP, MP_KEY_DOWN,
        MP_KEY_LEFT, MP_KEY_RIGHT,
        MP_KEY_W, MP_KEY_S,
        MP_KEY_A, MP_KEY_D,
    };
    bool32 *controlList[] = {
        &cameraControls->rUp, &cameraControls->rDown,
        &cameraControls->rLeft, &cameraControls->rRight,
        &cameraControls->tForward, &cameraControls->tBackward,
        &cameraControls->tLeft, &cameraControls->tRight,
    };
    constexpr uint32_t listSize = arraysize(keyList);

    for(uint32_t i = 0; i < listSize; i++)
    {
        if(eventReceiver->keyPressedEvents & keyList[i])
            *controlList[i] = true;
        else if(eventReceiver->keyReleasedEvents & keyList[i])
            *controlList[i] = false;
    }
}
// Returns the chunk that contains position -- 39, 34, 40
inline static mpChunk* mpChunkBoundsCheck(const mpWorldData *worldData, vec3 position)
{
    constexpr vec3 diagonal = {MP_CHUNK_SIZE, MP_CHUNK_SIZE, MP_CHUNK_SIZE};
    mpChunk *result = nullptr;
    for(uint32_t i = 0; i < worldData->chunkCount; i++)
    {
        const vec3 chunkCorner = worldData->chunks[i].position + diagonal;
        // Simple bounding box search to find which chunk the destination is located within
        if(position.x <= worldData->chunks[i].position.x)
            continue;
        if(position.x > chunkCorner.x)
            continue;

        if(position.y <= worldData->chunks[i].position.y)
            continue;
        if(position.y > chunkCorner.y)
            continue;

        if(position.z <= worldData->chunks[i].position.z)
            continue;
        if(position.z > chunkCorner.z)
            continue;

        result = &worldData->chunks[i];
        break;
    }
    return result;
}
// NOTE: could perhaps return raycasthit data or smth
static mpVoxel* mpRaycast(mpWorldData *worldData, const vec3 origin, const vec3 direction)
{
    constexpr uint32_t stepCount = 10;

    mpVoxel *result = nullptr, *previous = nullptr;
    for(uint32_t step = 1; step <= stepCount; step++)
    {
        vec3 raycastHit = origin + direction * static_cast<float>(step);

        mpChunk *chunk = mpChunkBoundsCheck(worldData, raycastHit);
        if(chunk != nullptr)
        {
            // Convert raycastHit to local chunk space
            raycastHit -= chunk->position;
            const uint32_t x = static_cast<uint32_t>(raycastHit.x);
            const uint32_t y = static_cast<uint32_t>(raycastHit.y);
            const uint32_t z = static_cast<uint32_t>(raycastHit.z);

            previous = result;
            result = &chunk->voxels[x][y][z];
            chunk->flags |= CHUNK_FLAG_IS_DIRTY;
            if(result->flags & VOXEL_FLAG_ACTIVE)
            {
                result = previous;
                break;
            }
        }
    }
    return result;
}

static bool32 mpEntityIsGrounded(const mpWorldData *worldData, vec3 position)
{
    bool32 result = false;
    mpChunk *chunk = mpChunkBoundsCheck(worldData, position);
    if(chunk != nullptr)
    {
        position -= chunk->position;

        if(position.x < 0.0f)
            position.x = 0.0f;
        if(position.y < 0.0f)
            position.y = 0.0f;
        if(position.z < 0.0f)
            position.z = 0.0f;
        const uint32_t x = static_cast<uint32_t>(position.x);
        const uint32_t y = static_cast<uint32_t>(position.y);
        const uint32_t z = static_cast<uint32_t>(position.z);
        result = chunk->voxels[x][y][z].flags & VOXEL_FLAG_ACTIVE;
    }
    else
    {
        puts("GROUND CHECK ERROR: Entity out of world bounds");
    }
    return result;
}

inline static void mpPrintMemoryInfo(mpMemoryRegion *region, const char *name)
{
    MP_LOG_TRACE("%s uses %zu out of %zu kB\n", name, (region->dataSize / 1000), (region->regionSize) / 1000)
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
    mpMemoryPool smallPool = mpCreateMemoryPool(6, MegaBytes(40), 2);

    mpMemoryRegion *chunkMemory = mpGetMemoryRegion(&smallPool);
    mpMemoryRegion *meshHeaderMemory = mpGetMemoryRegion(&smallPool);
    mpMemoryRegion *tempMemory = mpGetMemoryRegion(&smallPool);

    mpVoxelTypeDictionary typeDict = mpCreateVoxelTypeDictionary();
    typeDict.data[VOXEL_TYPE_GRASS] = {0.0f, 0.3f, 0.05f, 1.0f};
    typeDict.data[VOXEL_TYPE_DARKGRASS] = {0.0f, 0.2f, 0.0f, 1.0f};
    typeDict.data[VOXEL_TYPE_DIRT] = {0.3f, 0.2f, 0.0f, 1.0f};
    typeDict.data[VOXEL_TYPE_STONE] = {0.15f, 0.2f, 0.2f, 1.0f};
    typeDict.data[VOXEL_TYPE_WOOD] = {0.6f, 0.2f, 0.2f, 1.0f};

    const grid3 worldSize = {10, 10, 5};
    core.worldData = mpGenerateWorldData(worldSize, chunkMemory);

    mpMemoryPool meshPool = mpCreateMemoryPool(core.worldData.chunkCount, MegaBytes(1), 42);
    core.renderData = mpGenerateRenderData(&core.worldData, &meshPool, meshHeaderMemory, tempMemory, &typeDict);

    mpMemoryRegion *vulkanMemory = mpGetMemoryRegion(&smallPool);
    mpVulkanInit(&core, vulkanMemory);

    mpPrintMemoryInfo(chunkMemory, "chunkmemory");
    mpPrintMemoryInfo(meshHeaderMemory, "meshHeaderMemory");
    mpPrintMemoryInfo(vulkanMemory, "vulkanMemory");
    printf("Largest chunk size: %zu\n", largestChunkSize);

    core.camera.speed = 10.0f;
    core.camera.sensitivity = 2.0f;
    core.camera.fov = PI32 / 3.0f;
    core.camera.model = Mat4x4Identity();
    core.camera.pitchClamp = (PI32 / 2.0f) - 0.01f;

    core.globalLight.ambient = 0.4f;
    core.globalLight.position = {100.0f, 100.0f, 100.0f};
    core.globalLight.colour = {0.9f, 0.9f, 1.0f};

    // TODO: Entity Component System?
    mpEntity player = {};
    player.position = {20.0f, 20.0f, 52.0f};
    player.velocity = {};
    player.mass = 1.0f;
    player.speed = 10.0f;
    core.camera.position = player.position;

    float timestep = 0.0f;
    int64_t lastCounter = 0, perfCountFrequency = 0;
    PlatformPrepareClock(&lastCounter, &perfCountFrequency);

    core.windowInfo.running = true;
    core.windowInfo.hasResized = false; // Windows likes to set this to true at startup
    while(core.windowInfo.running)
    {
        PlatformPollEvents(&core.eventReceiver);

        mpUpdateCameraControlState(&core.eventReceiver, &core.camControls);

        // Player physics update
        constexpr float playerHeight = 3.0f;
        float playerGravityForce = player.mass * MP_GRAVITY_CONSTANT;
        float jumpForce = 0.0f;

        if(mpEntityIsGrounded(&core.worldData, player.position))
        {
            playerGravityForce = 0.0f;
            player.velocity.z = 0.0f;
            if(core.eventReceiver.keyPressedEvents & MP_KEY_SPACE)
            {
                player.position.z += 0.2f;
                jumpForce = 200.0f;
            }
        }
        player.zAccel = 2.0f * (playerGravityForce + 10.0f * jumpForce) / player.mass;
        player.velocity.z += player.zAccel * timestep;
        player.position.z += player.velocity.z * timestep;

        // Clamp rotation values
        if(core.camera.pitch > core.camera.pitchClamp)
            core.camera.pitch = core.camera.pitchClamp;
        else if(core.camera.pitch < -core.camera.pitchClamp)
            core.camera.pitch = -core.camera.pitchClamp;
        // Update camera rotation values
        if(core.camControls.rUp)
            core.camera.pitch += core.camera.sensitivity * timestep;
        else if(core.camControls.rDown)
            core.camera.pitch -= core.camera.sensitivity * timestep;
        if(core.camControls.rLeft)
            core.camera.yaw += core.camera.sensitivity * timestep;
        else if(core.camControls.rRight)
            core.camera.yaw -= core.camera.sensitivity * timestep;
        // Get camera vectors
        const float yawCos = cosf(core.camera.yaw);
        const float yawSin = sinf(core.camera.yaw);
        const float pitchCos = cosf(core.camera.pitch);
        const vec3 front = {pitchCos * yawCos, pitchCos * yawSin, sinf(core.camera.pitch)};
        const vec3 xyFront = {yawCos, yawSin, 0.0f};
        const vec3 left = {yawSin, -yawCos, 0.0f};
        constexpr vec3 up = {0.0f, 0.0f, 1.0f};
        // Update player position state
        if(core.camControls.tForward)
            player.position += xyFront * core.camera.speed * timestep;
        else if(core.camControls.tBackward)
            player.position -= xyFront * core.camera.speed * timestep;
        if(core.camControls.tLeft)
            player.position -= left * core.camera.speed * timestep;
        else if(core.camControls.tRight)
            player.position += left * core.camera.speed * timestep;
        // Update camera matrices
        core.camera.position = {player.position.x, player.position.y, player.position.z + playerHeight};
        core.camera.view = LookAt(core.camera.position, core.camera.position + front, up);
        core.camera.projection = Perspective(core.camera.fov, static_cast<float>(core.windowInfo.width) / static_cast<float>(core.windowInfo.height), 0.1f, 100.0f);

        // Game events : raycasthits
        if(core.eventReceiver.keyPressedEvents & MP_KEY_F)
        {
            mpVoxel *raycastHit = mpRaycast(&core.worldData, core.camera.position, front);
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
                core.renderData.meshes[i] = mpCreateMesh(&core.worldData.chunks[i], core.renderData.meshes[i].memReg, tempMemory, &typeDict);
                core.worldData.chunks[i].flags ^= CHUNK_FLAG_IS_DIRTY;
                mpVulkanRecreateGeometryBuffer(core.rendererHandle, &core.renderData.meshes[i], i);
                core.renderFlags |= MP_RENDER_FLAG_REDRAW_MESHES;
            }
        }

        mpVulkanUpdate(&core, vulkanMemory);

        core.windowInfo.hasResized = false;
        ResetEventReceiver(&core.eventReceiver);
        timestep = PlatformUpdateClock(&lastCounter, perfCountFrequency);
        mpDbgProcessSampledRecords(2000);
    }

    mpVulkanCleanup(&core.rendererHandle, core.renderData.meshCount);

    mpDestroyMemoryPool(&meshPool);
    mpDestroyMemoryPool(&smallPool);

    return 0;
}
