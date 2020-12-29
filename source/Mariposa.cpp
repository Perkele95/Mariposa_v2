#include "Mariposa.h"
#include "Win32_Mariposa.h"
#include "VulkanLayer.h"

#include <stdlib.h>
#include <stdio.h>

constexpr mpQuadFaceArray mpQuadFaceNorth = {
    vec3{ vScale, -vScale,  vScale},
    vec3{ vScale, -vScale, -vScale},
    vec3{ vScale,  vScale, -vScale},
    vec3{ vScale,  vScale,  vScale},
    vec3{1.0f, 0.0f, 0.0f}
};
constexpr mpQuadFaceArray mpQuadFaceSouth = {
    vec3{-vScale,  vScale,  vScale},
    vec3{-vScale,  vScale, -vScale},
    vec3{-vScale, -vScale, -vScale},
    vec3{-vScale, -vScale,  vScale},
    vec3{-1.0f, 0.0f, 0.0f}
};
constexpr mpQuadFaceArray mpQuadFaceEast = {
    vec3{ vScale,  vScale,  vScale},
    vec3{ vScale,  vScale, -vScale},
    vec3{-vScale,  vScale, -vScale},
    vec3{-vScale,  vScale,  vScale},
    vec3{0.0f, 1.0f, 0.0f}
};
constexpr mpQuadFaceArray mpQuadFaceWest = {
    vec3{-vScale, -vScale,  vScale},
    vec3{-vScale, -vScale, -vScale},
    vec3{ vScale, -vScale, -vScale},
    vec3{ vScale, -vScale,  vScale},
    vec3{0.0f, -1.0f, 0.0f}
};
constexpr mpQuadFaceArray mpQuadFaceTop = {
    vec3{-vScale, -vScale,  vScale},
    vec3{ vScale, -vScale,  vScale},
    vec3{ vScale,  vScale,  vScale},
    vec3{-vScale,  vScale,  vScale},
    vec3{0.0f, 0.0f, 1.0f}
};
constexpr mpQuadFaceArray mpQuadFaceBottom = {
    vec3{-vScale,  vScale, -vScale},
    vec3{ vScale,  vScale, -vScale},
    vec3{ vScale, -vScale, -vScale},
    vec3{-vScale, -vScale, -vScale},
    vec3{0.0f, 0.0f, -1.0f}
};

static mpQuad mpCreateQuad(const mpQuadFaceArray &quadFace, vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
{
    mpQuad quad;
    for(uint32_t i = 0; i < 4; i++)
        quad.vertices[i] = {(quadFace.data[i] * scale) + offset, quadFace.normal, colour};

    constexpr uint16_t quadIndices[] = {0, 1, 2, 2, 3, 0};
    for(uint16_t index = 0; index < 6; index++)
        quad.indices[index] = quadIndices[index] + indexOffset;

    return quad;
}
// mpCreateMesh resets tempMemory after use.
static mpMesh mpCreateMesh(mpChunk *chunk, mpMemoryRegion *meshRegion, mpMemoryRegion *tempMemory, mpVoxelTypeTable *typeTable)
{
    constexpr float VOXEL_SCALE = 0.5f;
    constexpr uint16_t vertexStride = 4;
    constexpr uint16_t indexStride = 6;
    uint16_t vertexCount = 0, indexCount = 0;

    constexpr size_t tempVertBlockSize = sizeof(mpVertex) * 12 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    mpVertex *tempBlockVertices = static_cast<mpVertex*>(mpAllocateIntoRegion(tempMemory, tempVertBlockSize));
    mpVertex *tempBlockVertIncrementer = tempBlockVertices;

    constexpr size_t tempIndexBlockSize = sizeof(uint16_t) * 18 * MP_CHUNK_SIZE * MP_CHUNK_SIZE * MP_CHUNK_SIZE;
    uint16_t *tempBlockIndices = static_cast<uint16_t*>(mpAllocateIntoRegion(tempMemory, tempIndexBlockSize));
    uint16_t *tempBlockIndexIncrementer = tempBlockIndices;

    constexpr size_t vertexCopySize = sizeof(mpVertex) * vertexStride;
    constexpr size_t indexCopySize = sizeof(uint16_t) * indexStride;

    for(uint32_t z = 0; z < MP_CHUNK_SIZE; z++){
        for(uint32_t y = 0; y < MP_CHUNK_SIZE; y++){
            for(uint32_t x = 0; x < MP_CHUNK_SIZE; x++)
            {
                mpVoxel &voxel = chunk->voxels[x][y][z];
                if((voxel.flags & VOXEL_FLAG_ACTIVE) == false)
                    continue;

                const vec4 colour = mpGetVoxelColour(typeTable, voxel.type, voxel.modifier);
                const vec3 positionOffset = chunk->position + mpGridU32ToVec3({x, y, z});

                if(voxel.flags & VOXEL_FLAG_DRAW_NORTH)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceNorth, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & VOXEL_FLAG_DRAW_SOUTH)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceSouth, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & VOXEL_FLAG_DRAW_EAST)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceEast, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & VOXEL_FLAG_DRAW_WEST)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceWest, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & VOXEL_FLAG_DRAW_TOP)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceTop, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & VOXEL_FLAG_DRAW_BOTTOM)
                {
                    const mpQuad quad = mpCreateQuad(mpQuadFaceBottom, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

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
// TODO: function should take a seed to generate multiple worlds as well as regenerate the same world from a seed
static mpChunk mpGenerateChunkTerrain(mpChunk chunk)
{
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
            }
        }
    }
    return chunk;
}

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

static mpWorldData mpGenerateWorldData(const gridU32 bounds, mpMemoryRegion *chunkMemory)
{
    mpWorldData worldData = {};
    worldData.chunkCount = bounds.x * bounds.y * bounds.z;
    worldData.bounds = bounds;
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
        if(pos.x < (bounds.x - 1))
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_NORTH;
            worldData.chunks[i].northNeighbour = &worldData.chunks[i + 1];
        }
        if(pos.y > 0)
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_WEST;
            worldData.chunks[i].westNeighbour = &worldData.chunks[i - bounds.x];
        }
        if(pos.y < (bounds.y - 1))
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_EAST;
            worldData.chunks[i].eastNeighbour = &worldData.chunks[i + bounds.x];
        }
        if(pos.z > 0)
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_BOTTOM;
            worldData.chunks[i].bottomNeighbour = &worldData.chunks[i - (bounds.x * bounds.y)];
        }
        if(pos.z < (bounds.z - 1))
        {
            worldData.chunks[i].flags |= CHUNK_FLAG_NEIGHBOUR_TOP;
            worldData.chunks[i].topNeighbour = &worldData.chunks[i + (bounds.x * bounds.y)];
        }
        // This bit increments x,y and z as if the for loop was iterating over a 3D array
        pos.x++;
        pos = pos.x == bounds.x ? vec3{0, ++pos.y, pos.z} : pos;
        pos = pos.y == bounds.y ? vec3{pos.x, 0, ++pos.z} : pos;
    }

    pos = {};
    for(uint32_t i = 0; i < worldData.chunkCount; i++)
    {
        worldData.chunks[i] = mpSetDrawFlags(worldData.chunks[i]);
        // This bit increments x,y and z as if the for loop was iterating over a 3D array
        pos.x++;
        pos = pos.x == bounds.x ? vec3{0, ++pos.y, pos.z} : pos;
        pos = pos.y == bounds.y ? vec3{pos.x, 0, ++pos.z} : pos;
    }

    return worldData;
}

static mpRenderData mpGenerateRenderData(const mpWorldData *worldData, mpMemoryPool *meshPool, mpMemoryRegion *meshHeaderMemory, mpMemoryRegion *tempMemory, mpVoxelTypeTable *typeTable)
{
    mpRenderData renderData = {};
    renderData.meshCount = worldData->chunkCount;
    renderData.meshes = static_cast<mpMesh*>(mpAllocateIntoRegion(meshHeaderMemory, sizeof(mpMesh) * renderData.meshCount));

    for(uint32_t i = 0; i < renderData.meshCount; i++)
    {
        mpMemoryRegion *meshRegion = mpGetMemoryRegion(meshPool);
        renderData.meshes[i] = mpCreateMesh(&worldData->chunks[i], meshRegion, tempMemory, typeTable);
    }
    return renderData;
}

static mpGUI::renderData mpCreateGuiData(mpMemoryRegion *staticMemory, uint32_t screenWidth, uint32_t screenHeight)
{
    constexpr size_t vertexSize = sizeof(mpGUI::vertex);
    constexpr size_t indexSize = sizeof(uint16_t);
    // Create list of gui elements
    // Format: { topLeft, bottomRight }
    constexpr mpGUI::rect2D rect2DList[] = {
        {{100, 100}, {500, 200}},
        {{100, 250}, {500, 350}},
        {{100, 400}, {500, 500}},
    };
    constexpr uint32_t rect2DListCount = arraysize(rect2DList);
    constexpr uint32_t quadVertexCount = arraysize(mpGUI::quad::vertices);
    constexpr uint32_t quadIndexCount = arraysize(mpGUI::quad::indices);

    constexpr size_t vertexStride = sizeof(mpGUI::vertex) * quadVertexCount;
    constexpr size_t vertexAllocSize = vertexStride * rect2DListCount;
    constexpr size_t indexStride = sizeof(uint16_t) * quadIndexCount;
    constexpr size_t indexAllocSize = indexStride * rect2DListCount;
    // Allocate necessary data
    mpGUI::renderData result = {};
    result.vertices = static_cast<mpGUI::vertex*>(mpAllocateIntoRegion(staticMemory, vertexAllocSize));
    result.indices = static_cast<uint16_t*>(mpAllocateIntoRegion(staticMemory, indexAllocSize));
    result.vertexCount = rect2DListCount * quadVertexCount;
    result.indexCount = rect2DListCount * quadIndexCount;
    // Merge gui elements into big data block
    for(uint32_t rect = 0; rect < rect2DListCount; rect++)
    {
        mpGUI::quad newQuad = mpGUI::quadFromRect(rect2DList[rect], {1.0f, 1.0f, 1.0f, 0.2f}, {screenWidth, screenHeight});
        for(uint32_t i = 0; i < quadIndexCount; i++)
            newQuad.indices[i] += static_cast<uint16_t>(quadVertexCount * rect);

        memcpy(result.vertices + quadVertexCount * rect, newQuad.vertices, vertexStride);
        memcpy(result.indices + quadIndexCount * rect, newQuad.indices, indexStride);
    }
    return result;
}

static void mpUpdateCameraControlState(const mpEventReceiver &eventReceiver, mpCameraControls *cameraControls)
{
    constexpr mpKeyEvent keyList[] = {
        MP_KEY_UP, MP_KEY_DOWN,
        MP_KEY_LEFT, MP_KEY_RIGHT,
        MP_KEY_W, MP_KEY_S,
        MP_KEY_A, MP_KEY_D,
    };
    constexpr uint32_t listSize = arraysize(keyList);
    for(uint32_t key = 0; key < listSize; key++)
    {
        if(eventReceiver.keyPressedEvents & keyList[key])
            cameraControls->flags |= keyList[key];
        else if(eventReceiver.keyReleasedEvents & keyList[key])
            cameraControls->flags &= ~keyList[key];
    }
}

static vec3 mpRayCast(const mpWorldData &worldData, const vec3 origin, const vec3 direction)
{
    vec3 rayCastHit = origin + direction;
    for(uint32_t step = 0; step < 20; step++)
    {
        rayCastHit += direction;
        mpChunk *chunk = mpGetContainingChunk(worldData, rayCastHit);
        if(chunk != nullptr)
        {
            const vec3 localHit = rayCastHit - chunk->position;
            gridU32 index = mpVec3ToGridU32(localHit);

            if(chunk->voxels[index.x][index.y][index.z].flags & VOXEL_FLAG_ACTIVE)
                break;
        }
    }
    return rayCastHit;
}
// TODO: optimise
static void mpCreateVoxelSphere(const mpWorldData &worldData, const vec3 origin, const vec3 direction, mpBitFieldShort type)
{
    vec3 rayCastHit = mpRayCast(worldData, origin, direction);
    // Paint sphere
    constexpr int32_t size = 8;
    constexpr int32_t start = -(size/2);
    for(int32_t z = start; z < size; z++)
    {
        for(int32_t y = start; y < size; y++)
        {
            for(int32_t x = start; x < size; x++)
            {
                const vec3 target = rayCastHit + mpGrid32ToVec3(grid32{x,y,z});
                mpChunk *chunk = mpGetContainingChunk(worldData, target);
                if(chunk != nullptr)
                {
                    const vec3 localHit = target - chunk->position;
                    if(vec3Length(target - rayCastHit) < 4.0f)
                    {
                        gridU32 index = mpVec3ToGridU32(localHit);

                        mpVoxel &result = chunk->voxels[index.x][index.y][index.z];
                        result.flags = VOXEL_FLAG_ACTIVE;
                        result.type = type;
                        chunk->flags |= CHUNK_FLAG_DIRTY;
                    }
                }
            }
        }
    }
}

static void mpCreateVoxelBlock(const mpWorldData &worldData, const vec3 origin, const vec3 direction, mpBitFieldShort type)
{
    vec3 rayCastHit = mpRayCast(worldData, origin, direction);
    // Fetch each block around target and set to active
    constexpr uint32_t size = 4;
    for(uint32_t z = 0; z < size; z++)
    {
        for(uint32_t y = 0; y < size; y++)
        {
            for(uint32_t x = 0; x < size; x++)
            {
                const vec3 target = rayCastHit + mpGridU32ToVec3(gridU32{x,y,z});
                mpChunk *chunk = mpGetContainingChunk(worldData, target);
                if(chunk != nullptr)
                {
                    const vec3 localHit = target - chunk->position;
                    gridU32 index = mpVec3ToGridU32(localHit);

                    mpVoxel &result = chunk->voxels[index.x][index.y][index.z];
                    result.flags = VOXEL_FLAG_ACTIVE;
                    result.type = type;
                    chunk->flags |= CHUNK_FLAG_DIRTY;
                }
            }
        }
    }
}

inline static void mpPaintVoxel(const mpWorldData &worldData, const vec3 origin, const vec3 direction, mpBitFieldShort type, mpBitFieldShort mod = VOXEL_TYPE_MOD_DEFAULT)
{
    for(uint32_t step = 1; step <= 20; step++)
    {
        vec3 raycastHit = origin + direction * static_cast<float>(step);

        mpChunk *chunk = mpGetContainingChunk(worldData, raycastHit);
        if(chunk != nullptr)
        {
            raycastHit -= chunk->position;
            gridU32 index = mpVec3ToGridU32(raycastHit);

            mpVoxel &result = chunk->voxels[index.x][index.y][index.z];
            if(result.flags & VOXEL_FLAG_ACTIVE)
            {
                result.type = type;
                result.modifier = mod;
                chunk->flags |= CHUNK_FLAG_DIRTY;
                break;
            }
        }
    }
}

static void mpEntityPhysics(const mpWorldData &worldData, mpEntity &entity, float timestep)
{
    // Check for collision
    const mpChunk *chunk = mpGetContainingChunk(worldData, entity.position);
    if(chunk != nullptr)
    {
        const vec3 localPosition = entity.position - chunk->position;
        const gridU32 index = mpVec3ToGridU32(localPosition);
        const bool32 isColliding = chunk->voxels[index.x][index.y][index.z].flags & VOXEL_FLAG_ACTIVE;
        if(isColliding)
        {
            // Collision response
            entity.force.z += MP_GRAVITY_CONSTANT;
            entity.velocity.z = 0.2f;
        }
    }
    // F = ma => a = F/m
    const vec3 force = gravityVec3 + entity.force;
    const vec3 acceleration = force / entity.mass;
    entity.velocity += acceleration * timestep;
    entity.position += entity.velocity * timestep;
    entity.force = vec3{};
}

inline static void mpPrintMemoryInfo(mpMemoryRegion *region, const char *name)
{
    MP_LOG_INFO("%s uses %zu out of %zu kB\n", name, (region->dataSize / 1000), (region->regionSize) / 1000)
}

int main(int argc, char *argv[])
{
    mpCore core;
    memset(&core, 0, sizeof(mpCore));
    core.name = "Mariposa 3D Voxel Engine";
    core.renderFlags |= MP_RENDER_FLAG_ENABLE_VK_VALIDATION;

    PlatformCreateWindow(&core.windowInfo, core.name);
    // TODO: Prepare win32 sound
    core.callbacks = PlatformGetCallbacks();
    // TODO: allocate pool header on heap as well
    mpMemoryPool chunkPool = mpCreateMemoryPool(2, MegaBytes(40), 1);
    mpMemoryPool smallPool = mpCreateMemoryPool(6, MegaBytes(1), 2);

    mpMemoryRegion *chunkMemory = mpGetMemoryRegion(&chunkPool);
    mpMemoryRegion *staticMemory = mpGetMemoryRegion(&smallPool);
    mpMemoryRegion *tempMemory = mpGetMemoryRegion(&chunkPool);

    mpVoxelTypeTable* table = mpCreateVoxelTypeTable(
        mpAllocateIntoRegion(staticMemory, sizeof(mpVoxelTypeTable))
    );
    mpRegisterVoxelType(table, {0.1f, 0.3f, 0.15f, 1.0f}, VOXEL_TYPE_GRASS);
    mpRegisterVoxelType(table, {0.3f, 0.2f, 0.2f, 1.0f}, VOXEL_TYPE_DIRT);
    mpRegisterVoxelType(table, {1.0f, 1.0f, 1.0f, 1.0f}, VOXEL_TYPE_STONE);
    mpRegisterVoxelType(table, {0.1f, 0.1f, 0.1f, 1.0f}, VOXEL_TYPE_STONE, VOXEL_TYPE_MOD_DARK);
    mpRegisterVoxelType(table, {0.3f, 0.1f, 0.1f, 1.0f}, VOXEL_TYPE_WOOD);
    mpRegisterVoxelType(table, {0.0f, 0.2f, 0.0f, 1.0f}, VOXEL_TYPE_GRASS, VOXEL_TYPE_MOD_DARK);

    core.worldData.bounds = {10, 10, 5};
    core.worldData = mpGenerateWorldData(core.worldData.bounds, chunkMemory);

    mpMemoryPool meshPool = mpCreateMemoryPool(core.worldData.chunkCount, MegaBytes(1), 42);
    core.renderData = mpGenerateRenderData(&core.worldData, &meshPool, staticMemory, tempMemory, table);

    core.guiData = mpCreateGuiData(staticMemory, core.windowInfo.width, core.windowInfo.height);

    mpMemoryRegion *vulkanMemory = mpGetMemoryRegion(&smallPool);
    mpVulkanInit(&core, vulkanMemory, tempMemory, core.renderFlags & MP_RENDER_FLAG_ENABLE_VK_VALIDATION);

    mpPrintMemoryInfo(chunkMemory, "staticMemory");
    mpPrintMemoryInfo(staticMemory, "meshHeaderMemory");
    mpPrintMemoryInfo(vulkanMemory, "vulkanMemory");
    MP_LOG_INFO("Meshpool consumes: %zu memory\n", core.worldData.chunkCount * sizeof(mpChunk));

    core.camera.speed = 10.0f;
    core.camera.sensitivity = 2.0f;
    core.camera.fov = PI32 / 3.0f;
    core.camera.model = Mat4x4Identity();
    core.camera.pitchClamp = (PI32 / 2.0f) - 0.01f;

    core.globalLight.ambient = 0.4f;
    core.globalLight.position = {80.0f, 80.0f, 80.0f};
    core.globalLight.colour = {0.6f, 0.6f, 1.0f};

    // TODO: Entity Component System?
    mpEntity player = {};
    player.position = {19.0f, 19.0f, 45.0f};
    player.velocity = {};
    player.mass = 2.0f;
    player.speed = 10.0f;
    core.camera.position = player.position;

    float timestep = 0.0f;
    PlatformPrepareClock();

    core.windowInfo.running = true;
    core.windowInfo.hasResized = false; // Windows likes to set this to true at startup
    while(core.windowInfo.running)
    {
        PlatformPollEvents(&core.eventReceiver);

        mpUpdateCameraControlState(core.eventReceiver, &core.camControls);

        // Core :: Clamp rotation values
        if(core.camera.pitch > core.camera.pitchClamp)
            core.camera.pitch = core.camera.pitchClamp;
        else if(core.camera.pitch < -core.camera.pitchClamp)
            core.camera.pitch = -core.camera.pitchClamp;
        // Core :: Update camera rotation values
        if(core.camControls.flags & MP_KEY_UP)
            core.camera.pitch += core.camera.sensitivity * timestep;
        else if(core.camControls.flags & MP_KEY_DOWN)
            core.camera.pitch -= core.camera.sensitivity * timestep;
        if(core.camControls.flags & MP_KEY_LEFT)
            core.camera.yaw += core.camera.sensitivity * timestep;
        else if(core.camControls.flags & MP_KEY_RIGHT)
            core.camera.yaw -= core.camera.sensitivity * timestep;
        // Core :: Get camera vectors
        const float yawCos = cosf(core.camera.yaw);
        const float yawSin = sinf(core.camera.yaw);
        const float pitchCos = cosf(core.camera.pitch);
        const vec3 front = {pitchCos * yawCos, pitchCos * yawSin, sinf(core.camera.pitch)};
        const vec3 xyFront = {yawCos, yawSin, 0.0f};
        const vec3 left = {yawSin, -yawCos, 0.0f};
        constexpr vec3 up = {0.0f, 0.0f, 1.0f};
        // Core :: Update player position state
        if(core.camControls.flags & MP_KEY_W)
            player.position += xyFront * core.camera.speed * timestep;
        else if(core.camControls.flags & MP_KEY_S)
            player.position -= xyFront * core.camera.speed * timestep;
        if(core.camControls.flags & MP_KEY_A)
            player.position -= left * core.camera.speed * timestep;
        else if(core.camControls.flags & MP_KEY_D)
            player.position += left * core.camera.speed * timestep;
        // Core :: Update camera matrices
        core.camera.position = {player.position.x, player.position.y, player.position.z + 8.0f};
        core.camera.view = LookAt(core.camera.position, core.camera.position + front, up);
        core.camera.projection = Perspective(core.camera.fov, static_cast<float>(core.windowInfo.width) / static_cast<float>(core.windowInfo.height), 0.1f, 100.0f);

        // Game events :: Physics update
        if(core.eventReceiver.keyPressedEvents & MP_KEY_SPACE)
            player.force = vec3{0.0f, 0.0f, 5000.0f};
        mpEntityPhysics(core.worldData, player, timestep);

        // Game events :: raycasthits
        if(core.eventReceiver.keyPressedEvents & MP_KEY_F)
            mpCreateVoxelSphere(core.worldData, core.camera.position, front, VOXEL_TYPE_STONE);
        if(core.eventReceiver.keyPressedEvents & MP_KEY_E)
            mpCreateVoxelBlock(core.worldData, core.camera.position, front, VOXEL_TYPE_STONE);

        // Core :: Recreate dirty meshes & vulkan buffers // TODO: manage a list of chunks needing updating rather than a for loop
        for(uint32_t i = 0; i < core.worldData.chunkCount; i++)
        {
            if(core.worldData.chunks[i].flags & CHUNK_FLAG_DIRTY)
            {
                core.worldData.chunks[i] = mpSetDrawFlags(core.worldData.chunks[i]);
                core.renderData.meshes[i] = mpCreateMesh(&core.worldData.chunks[i], core.renderData.meshes[i].memReg, tempMemory, table);
                core.worldData.chunks[i].flags ^= CHUNK_FLAG_DIRTY;
                mpVkRecreateGeometryBuffer(core.rendererHandle, &core.renderData.meshes[i], i);
                core.renderFlags |= MP_RENDER_FLAG_REDRAW_REQUIRED;
            }
        }
        // Renderer :: Render
        mpVulkanUpdate(&core, vulkanMemory, tempMemory);
        mpResetMemoryRegion(tempMemory);
        // Core :: Resets
        core.windowInfo.hasResized = false;
        ResetEventReceiver(&core.eventReceiver);
        MP_PROCESS_PROFILER
        timestep = PlatformUpdateClock();
    }

    mpVulkanCleanup(&core.rendererHandle, core.renderData.meshCount);

    mpDestroyMemoryPool(&meshPool);
    mpDestroyMemoryPool(&smallPool);

    return 0;
}
