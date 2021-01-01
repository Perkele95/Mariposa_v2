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

inline static mpQuad mpCreateQuad(const mpQuadFaceArray &quadFace, vec3 offset, vec4 colour, float scale, uint16_t indexOffset)
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
static mpMesh mpCreateMesh(mpVoxelSubRegion &subRegion, mpMemoryRegion *meshRegion, mpMemoryRegion *tempMemory)
{
    constexpr float VOXEL_SCALE = 0.5f;
    constexpr uint16_t vertexStride = 4;
    constexpr uint16_t indexStride = 6;
    uint16_t vertexCount = 0, indexCount = 0;

    constexpr size_t tempVertBlockSize = sizeof(mpVertex) * 12 * MP_SUBREGION_SIZE * MP_SUBREGION_SIZE * MP_SUBREGION_SIZE;
    mpVertex *tempBlockVertices = static_cast<mpVertex*>(mpAllocateIntoRegion(tempMemory, tempVertBlockSize));
    mpVertex *tempBlockVertIncrementer = tempBlockVertices;

    constexpr size_t tempIndexBlockSize = sizeof(uint16_t) * 18 * MP_SUBREGION_SIZE * MP_SUBREGION_SIZE * MP_SUBREGION_SIZE;
    uint16_t *tempBlockIndices = static_cast<uint16_t*>(mpAllocateIntoRegion(tempMemory, tempIndexBlockSize));
    uint16_t *tempBlockIndexIncrementer = tempBlockIndices;

    constexpr size_t vertexCopySize = sizeof(mpVertex) * vertexStride;
    constexpr size_t indexCopySize = sizeof(uint16_t) * indexStride;

    for(int32_t z = 0; z < MP_SUBREGION_SIZE; z++){
        for(int32_t y = 0; y < MP_SUBREGION_SIZE; y++){
            for(int32_t x = 0; x < MP_SUBREGION_SIZE; x++){
                mpVoxel &voxel = subRegion.voxels[x][y][z];
                if((voxel.flags & MP_VOXEL_FLAG_ACTIVE) == false)
                    continue;

                const vec4 colour = mpConvertToDenseColour(voxel.colour);
                const vec3 positionOffset = subRegion.position + mpVec3IntToVec3(vec3Int{x, y, z});

                if(voxel.flags & MP_VOXEL_FLAG_DRAW_NORTH){
                    const mpQuad quad = mpCreateQuad(mpQuadFaceNorth, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & MP_VOXEL_FLAG_DRAW_SOUTH){
                    const mpQuad quad = mpCreateQuad(mpQuadFaceSouth, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & MP_VOXEL_FLAG_DRAW_EAST){
                    const mpQuad quad = mpCreateQuad(mpQuadFaceEast, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & MP_VOXEL_FLAG_DRAW_WEST){
                    const mpQuad quad = mpCreateQuad(mpQuadFaceWest, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & MP_VOXEL_FLAG_DRAW_TOP){
                    const mpQuad quad = mpCreateQuad(mpQuadFaceTop, positionOffset, colour, VOXEL_SCALE, vertexCount);

                    memcpy(tempBlockVertIncrementer, quad.vertices, vertexCopySize);
                    memcpy(tempBlockIndexIncrementer, quad.indices, indexCopySize);

                    tempBlockVertIncrementer += vertexStride;
                    tempBlockIndexIncrementer += indexStride;
                    vertexCount += vertexStride;
                    indexCount += indexStride;
                }
                if(voxel.flags & MP_VOXEL_FLAG_DRAW_BOTTOM){
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
static mpVoxelSubRegion mpGenerateTerrain(const vec3 subRegionPosition)
{
    constexpr float noiseOffset = 40.0f;
    constexpr float noiseHeight = 8.0f;
    constexpr float granularity = 0.09f;
    mpVoxelSubRegion subRegion = {};
    subRegion.position = subRegionPosition;

    for(int32_t z = 0; z < MP_SUBREGION_SIZE; z++){
        for(int32_t y = 0; y < MP_SUBREGION_SIZE; y++){
            for(int32_t x = 0; x < MP_SUBREGION_SIZE; x++){
                const vec3 globalPos = subRegionPosition + mpVec3IntToVec3(vec3Int{x, y, z});
                const float noise = noiseHeight * perlin(globalPos.x * granularity, globalPos.y * granularity);
                const float surfaceThreshold = noiseOffset + noise;
                float moistureLevel = (noiseOffset + noise * 3.5f) * 4.0f;
                if(moistureLevel > 255.0f)
                    moistureLevel = 250.0f;

                if(globalPos.z > surfaceThreshold)
                    continue;

                mpVoxel &voxel = subRegion.voxels[x][y][z];
                voxel.flags = MP_VOXEL_FLAG_ACTIVE;
                voxel.colour.r = 0x20;
                voxel.colour.g = static_cast<uint8_t>(moistureLevel);
                voxel.colour.b = 0x4A;
                voxel.colour.a = 0xFF;
            }
        }
    }
    return subRegion;
}

static void mpSetDrawFlags(mpVoxelSubRegion &subRegion, const mpVoxelRegion *region, const vec3Int index)
{
    constexpr int32_t subRegMax = MP_SUBREGION_SIZE - 1;
    for(int32_t z = 0; z < MP_SUBREGION_SIZE; z++){
        for(int32_t y = 0; y < MP_SUBREGION_SIZE; y++){
            for(int32_t x = 0; x < MP_SUBREGION_SIZE; x++)
            {
                mpVoxel &voxel = subRegion.voxels[x][y][z];
                if((voxel.flags & MP_VOXEL_FLAG_ACTIVE) == false)
                    continue;
                // TODO: Figure out how to do this more elegantly
                bool32 flagCheck = 1;
                if(x > 0)
                    flagCheck = subRegion.voxels[x - 1][y][z].flags & MP_VOXEL_FLAG_ACTIVE;
                else if(subRegion.flags & MP_SUBREG_FLAG_NEIGHBOUR_SOUTH)
                    flagCheck = region->subRegions[index.x - 1][index.y][index.z].voxels[subRegMax][y][z].flags & MP_VOXEL_FLAG_ACTIVE;
                if(flagCheck == false)
                    voxel.flags |= MP_VOXEL_FLAG_DRAW_SOUTH;

                if(x < subRegMax)
                    flagCheck = subRegion.voxels[x + 1][y][z].flags & MP_VOXEL_FLAG_ACTIVE;
                else if(subRegion.flags & MP_SUBREG_FLAG_NEIGHBOUR_NORTH)
                    flagCheck = region->subRegions[index.x + 1][index.y][index.z].voxels[0][y][z].flags & MP_VOXEL_FLAG_ACTIVE;
                if(flagCheck == false)
                    voxel.flags |= MP_VOXEL_FLAG_DRAW_NORTH;

                if(y > 0)
                    flagCheck = subRegion.voxels[x][y - 1][z].flags & MP_VOXEL_FLAG_ACTIVE;
                else if(subRegion.flags & MP_SUBREG_FLAG_NEIGHBOUR_WEST)
                    flagCheck = region->subRegions[index.x][index.y - 1][index.z].voxels[x][subRegMax][z].flags & MP_VOXEL_FLAG_ACTIVE;
                if(flagCheck == false)
                    voxel.flags |= MP_VOXEL_FLAG_DRAW_WEST;

                if(y < subRegMax)
                    flagCheck = subRegion.voxels[x][y + 1][z].flags & MP_VOXEL_FLAG_ACTIVE;
                else if(subRegion.flags & MP_SUBREG_FLAG_NEIGHBOUR_EAST)
                    flagCheck = region->subRegions[index.x][index.y + 1][index.z].voxels[x][0][z].flags & MP_VOXEL_FLAG_ACTIVE;
                if(flagCheck == false)
                    voxel.flags |= MP_VOXEL_FLAG_DRAW_EAST;

                if(z > 0)
                    flagCheck = subRegion.voxels[x][y][z - 1].flags & MP_VOXEL_FLAG_ACTIVE;
                else if(subRegion.flags & MP_SUBREG_FLAG_NEIGHBOUR_BOTTOM)
                    flagCheck = region->subRegions[index.x][index.y][index.z - 1].voxels[x][y][subRegMax].flags & MP_VOXEL_FLAG_ACTIVE;
                if(flagCheck == false)
                    voxel.flags |= MP_VOXEL_FLAG_DRAW_BOTTOM;

                if(z < subRegMax)
                    flagCheck = subRegion.voxels[x][y][z + 1].flags & MP_VOXEL_FLAG_ACTIVE;
                else if(subRegion.flags & MP_SUBREG_FLAG_NEIGHBOUR_TOP)
                    flagCheck = region->subRegions[index.x][index.y][index.z + 1].voxels[x][y][0].flags & MP_VOXEL_FLAG_ACTIVE;
                if(flagCheck == false)
                    voxel.flags |= MP_VOXEL_FLAG_DRAW_TOP;
            }
        }
    }
}

static mpVoxelRegion *mpGenerateWorldData(mpMemoryRegion *regionMemory, mpMemoryPool *meshPool, mpMemoryRegion *tempMemory)
{
    mpVoxelRegion *region = static_cast<mpVoxelRegion*>(mpAllocateIntoRegion(regionMemory, sizeof(mpVoxelRegion)));
    // Set active flag on voxels
    constexpr int32_t regionMax = MP_REGION_SIZE - 1;
    for(int32_t z = 0; z < MP_REGION_SIZE; z++){
        for(int32_t y = 0; y < MP_REGION_SIZE; y++){
            for(int32_t x = 0; x < MP_REGION_SIZE; x++){
                // Set neighbour flags
                mpVoxelSubRegion &subRegion = region->subRegions[x][y][z];
                // Generate terrain
                const vec3 subRegionPosition = {
                    static_cast<float>(x * MP_SUBREGION_SIZE),
                    static_cast<float>(y * MP_SUBREGION_SIZE),
                    static_cast<float>(z * MP_SUBREGION_SIZE)
                };
                subRegion = mpGenerateTerrain(subRegionPosition);
                // Set neighbour flags
                if(x > 0)
                    subRegion.flags |= MP_SUBREG_FLAG_NEIGHBOUR_SOUTH;
                if(x < regionMax)
                    subRegion.flags |= MP_SUBREG_FLAG_NEIGHBOUR_NORTH;
                if(y > 0)
                    subRegion.flags |= MP_SUBREG_FLAG_NEIGHBOUR_WEST;
                if(y < regionMax)
                    subRegion.flags |= MP_SUBREG_FLAG_NEIGHBOUR_EAST;
                if(z > 0)
                    subRegion.flags |= MP_SUBREG_FLAG_NEIGHBOUR_BOTTOM;
                if(z < regionMax)
                    subRegion.flags |= MP_SUBREG_FLAG_NEIGHBOUR_TOP;
            }
        }
    }
    // Generate meshes
    for(int32_t z = 0; z < MP_REGION_SIZE; z++){
        for(int32_t y = 0; y < MP_REGION_SIZE; y++){
            for(int32_t x = 0; x < MP_REGION_SIZE; x++){
                // Set draw flags
                mpVoxelSubRegion &subRegion = region->subRegions[x][y][z];
                mpSetDrawFlags(subRegion, region, vec3Int{x, y, z});
                // Create mesh
                mpMemoryRegion *meshRegion = mpGetMemoryRegion(meshPool);
                region->meshArray[x][y][z] = mpCreateMesh(subRegion, meshRegion, tempMemory);
            }
        }
    }
    return region;
}

static mpGUI::renderData mpCreateGuiData(mpMemoryRegion *staticMemory, uint32_t screenWidth, uint32_t screenHeight)
{
    constexpr size_t vertexSize = sizeof(mpGUI::vertex);
    constexpr size_t indexSize = sizeof(uint16_t);
    // Create list of gui elements
    // Format: { topLeft, bottomRight }
    #if 0
    constexpr mpGUI::rect2D rect2DList[] = {
        {{100, 100}, {500, 200}},
        {{100, 250}, {500, 350}},
        {{100, 400}, {500, 500}},
    };
    #else
    constexpr mpGUI::rect2D rect2DList[] = {
        {}
    };
    #endif
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
    for(uint32_t key = 0; key < listSize; key++){
        if(eventReceiver.keyPressedEvents & keyList[key])
            cameraControls->flags |= keyList[key];
        else if(eventReceiver.keyReleasedEvents & keyList[key])
            cameraControls->flags &= ~keyList[key];
    }
}

static mpRayCastHitInfo mpRayCast(mpVoxelRegion &region, const vec3 origin, const vec3 direction)
{
    mpRayCastHitInfo hitInfo;
    memset(&hitInfo, 0, sizeof(mpRayCastHitInfo));
    vec3 rayCastHit = origin + direction;
    for(uint32_t i = 0; i < 20; i++){
        rayCastHit += direction;

        mpVoxelSubRegion *subRegion = mpGetContainintSubRegion(region, rayCastHit);
        if(subRegion != nullptr){
            const vec3 localPos = rayCastHit - subRegion->position;
            vec3Int index = mpVec3ToVec3Int(localPos);

            if(subRegion->voxels[index.x][index.y][index.z].flags & MP_VOXEL_FLAG_ACTIVE){
                hitInfo.position = rayCastHit;
                hitInfo.voxel = &subRegion->voxels[index.x][index.y][index.z];
                break;
            }
        }
    }
    return hitInfo;
}

static void mpSpawnTree(mpVoxelRegion *region, const vec3 origin)
{
    constexpr int32_t size = 6;
    constexpr float radius = size/2;
    constexpr int32_t start = -(size/2);
    for(int32_t layer = start; layer < 25; layer++){
        for(int32_t y = start; y < size; y++){
            for(int32_t x = start; x < size; x++){
                const vec3 target = origin + mpVec3IntToVec3(vec3Int{x, y, layer});
                mpVoxelQueryInfo info = mpQueryVoxelLocation(*region, target);
                if(info.voxel != nullptr){
                    const vec3 distanceCalc = {origin.x, origin.y, target.z};
                    const float distance = vec3Length(target - distanceCalc);
                    if(distance > radius)
                        continue;

                    info.voxel->flags = MP_VOXEL_FLAG_ACTIVE;
                    info.voxel->colour.r = x & 1 ? 0x25 : 0x2A;
                    info.voxel->colour.g = y & 1 ? 0x10 : 0x13;
                    info.voxel->colour.b = 0x10;
                    info.voxel->colour.a = 0xFF;
                    info.subRegion->flags |= MP_SUBREG_FLAG_DIRTY;
                }
            }
        }
    }
}

// TODO: optimise
static void mpCreateVoxelSphere(mpVoxelRegion &region, const vec3 origin, const vec3 direction)
{
    const mpRayCastHitInfo rayCastHit = mpRayCast(region, origin, direction);
    if(rayCastHit.voxel == nullptr)
        return;

    constexpr int32_t size = 12;
    constexpr uint8_t zOffset = size / 2;
    constexpr int32_t start = -(size/2);
    constexpr float radius = static_cast<float>(size / 2);

    for(int32_t z = start; z < size; z++){
        for(int32_t y = start; y < size; y++){
            for(int32_t x = start; x < size; x++){
                const vec3 target = rayCastHit.position + mpVec3IntToVec3(vec3Int{x,y,z});
                mpVoxelSubRegion *subRegion = mpGetContainintSubRegion(region, target);

                if(subRegion != nullptr){
                    const vec3 localHit = target - subRegion->position;

                    if(vec3Length(target - rayCastHit.position) < radius){
                        const vec3Int index = mpVec3ToVec3Int(localHit);
                        mpVoxel &result = subRegion->voxels[index.x][index.y][index.z];
                        result.flags = MP_VOXEL_FLAG_ACTIVE;
                        result.colour.r = static_cast<uint8_t>(z + zOffset) * 20;
                        result.colour.g = static_cast<uint8_t>(z + zOffset) * 12;
                        result.colour.b = static_cast<uint8_t>(z + zOffset) * 5;
                        result.colour.a = rayCastHit.voxel->colour.a;
                        subRegion->flags |= MP_SUBREG_FLAG_DIRTY;
                    }
                }
            }
        }
    }
}

inline static void mpCreateVoxelBlock(mpVoxelRegion &region, const vec3 origin, const vec3 direction, uint32_t rgba)
{
    const mpRayCastHitInfo rayCastHit = mpRayCast(region, origin, direction);
    if(rayCastHit.voxel == nullptr)
        return;

    constexpr int32_t size = 4;
    for(int32_t z = 0; z < size; z++){
        for(int32_t y = 0; y < size; y++){
            for(int32_t x = 0; x < size; x++){
                const vec3 target = rayCastHit.position + mpVec3IntToVec3(vec3Int{x,y,z});
                mpVoxelSubRegion *subRegion = mpGetContainintSubRegion(region, target);

                if(subRegion != nullptr){
                    const vec3 localHit = target - subRegion->position;
                    vec3Int index = mpVec3ToVec3Int(localHit);

                    mpVoxel &result = subRegion->voxels[index.x][index.y][index.z];
                    result.flags = MP_VOXEL_FLAG_ACTIVE;
                    result.colour.rgba = rgba;
                    subRegion->flags |= MP_SUBREG_FLAG_DIRTY;
                }
            }
        }
    }
}
// TODO: optimise
static void mpEntityPhysics(mpVoxelRegion &region, mpEntity &entity, float timestep)
{
    const mpVoxelSubRegion *subRegion = mpGetContainintSubRegion(region, entity.position);
    const vec3 posEx = vec3{entity.position.x, entity.position.y, entity.position.z - 1.0f};
    const mpVoxelSubRegion *subRegionEx = mpGetContainintSubRegion(region, posEx);
    if(subRegion != nullptr && subRegionEx != nullptr){
        const vec3Int posIndex = mpVec3ToVec3Int(entity.position - subRegion->position);
        const mpVoxel &voxel = subRegion->voxels[posIndex.x][posIndex.y][posIndex.z];

        const vec3Int posExIndex = mpVec3ToVec3Int(posEx - subRegionEx->position);
        const mpVoxel &voxelEx = subRegionEx->voxels[posExIndex.x][posExIndex.y][posExIndex.z];
        // Check collision
        if(voxelEx.flags & MP_VOXEL_FLAG_ACTIVE){
            if(voxel.flags & MP_VOXEL_FLAG_ACTIVE){
                // Entity is inside ground
                entity.position.z += 0.1f;
            }
            else{
                // Entity is on ground
            }
            entity.velocity.z = 0.0f;
        }
        else{
            // Entity is falling
            const vec3 force = gravityVec3 + entity.force;
            const vec3 acceleration = force / entity.mass;
            entity.velocity += acceleration * timestep;
            entity.position += entity.velocity * timestep;
            entity.force = vec3{};
        }
    }
    else{
        puts("PHYS ERROR: Entity out of bounds");
    }
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
    mpMemoryPool subRegionPool = mpCreateMemoryPool(2, MegaBytes(70), 1);
    mpMemoryPool smallPool = mpCreateMemoryPool(6, MegaBytes(1), 2);

    mpMemoryRegion *subRegionMemory = mpGetMemoryRegion(&subRegionPool);
    mpMemoryRegion *staticMemory = mpGetMemoryRegion(&smallPool);
    mpMemoryRegion *tempMemory = mpGetMemoryRegion(&subRegionPool);

    constexpr uint32_t subRegionCount = arraysize3D(mpVoxelRegion::subRegions);
    mpMemoryPool meshPool = mpCreateMemoryPool(subRegionCount, MegaBytes(1), 42);
    core.region = mpGenerateWorldData(subRegionMemory, &meshPool, tempMemory);

    core.guiData = mpCreateGuiData(staticMemory, core.windowInfo.width, core.windowInfo.height);

    mpMemoryRegion *vulkanMemory = mpGetMemoryRegion(&smallPool);
    mpVulkanInit(&core, vulkanMemory, tempMemory, core.renderFlags & MP_RENDER_FLAG_ENABLE_VK_VALIDATION);

    mpPrintMemoryInfo(subRegionMemory, "subRegionMemory");
    mpPrintMemoryInfo(staticMemory, "staticMemory");
    mpPrintMemoryInfo(vulkanMemory, "vulkanMemory");
    MP_LOG_INFO("Meshpool consumes: %zu kB\n", subRegionCount * sizeof(mpVoxelSubRegion) / 1000);

    core.camera.speed = 10.0f;
    core.camera.sensitivity = 2.0f;
    core.camera.fov = PI32 / 3.0f;
    core.camera.model = Mat4x4Identity();
    core.camera.pitchClamp = (PI32 / 2.0f) - 0.01f;

    core.pointLight.position = {100.0f, 100.0f, 60.0f};
    core.pointLight.constant = 1.0f;
    core.pointLight.linear = 0.07f;
    core.pointLight.quadratic = 0.017f;
    core.pointLight.diffuse = {2.0f, 1.3f, 1.0f};
    core.pointLight.ambient = 0.12f;

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
        mpEntityPhysics(*core.region, player, timestep);

        // Game events :: raycasthits
        if(core.eventReceiver.keyPressedEvents & MP_KEY_F)
            mpCreateVoxelSphere(*core.region, core.camera.position, front);
        if(core.eventReceiver.keyPressedEvents & MP_KEY_E){
            mpRayCastHitInfo hitInfo = mpRayCast(*core.region, core.camera.position, front);
            if(hitInfo.voxel != nullptr)
                mpSpawnTree(core.region, hitInfo.position);
        }

        // Core :: Recreate dirty meshes & vulkan buffers // TODO: manage a list of chunks needing updating rather than a for loop
        for(int32_t z = 0; z < MP_REGION_SIZE; z++){
            for(int32_t y = 0; y < MP_REGION_SIZE; y++){
                for(int32_t x = 0; x < MP_REGION_SIZE; x++){
                    mpVoxelSubRegion &subRegion = core.region->subRegions[x][y][z];
                    if(subRegion.flags & MP_SUBREG_FLAG_DIRTY){
                        mpSetDrawFlags(subRegion, core.region, vec3Int{x, y, z});

                        mpMesh &mesh = core.region->meshArray[x][y][z];
                        mesh = mpCreateMesh(subRegion, mesh.memReg, tempMemory);
                        subRegion.flags &= ~MP_SUBREG_FLAG_DIRTY;
                        constexpr uint32_t regionSize2x = MP_REGION_SIZE * MP_REGION_SIZE;
                        mpVkRecreateGeometryBuffer(core.rendererHandle, mesh, vec3Int{x, y, z});
                        core.renderFlags |= MP_RENDER_FLAG_REDRAW_REQUIRED;
                    }
                }
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

    mpVulkanCleanup(&core.rendererHandle);

    mpDestroyMemoryPool(&meshPool);
    mpDestroyMemoryPool(&smallPool);
    mpDestroyMemoryPool(&subRegionPool);

    return 0;
}
