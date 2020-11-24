#include "Win32_Mariposa.h"
#include "VulkanLayer.h"
#include "events.h"

#include <stdlib.h>
#include <stdio.h>

// Amount = amount of cubes/voxels. Scale = half of width/height.
static mpBatch CreateBatch(mpMemory *memory, float scale, vec2 bottomLeft, vec2 topRight)
{
    mpBatch batch = {};
    const vec2 localArea = {topRight.X - bottomLeft.X, topRight.Y - bottomLeft.Y};
    const uint32_t batchSize = static_cast<uint32_t>(localArea.X * localArea.Y);
    batch.vertices = static_cast<mpVertex*>(PushBackPermanentStorage(memory, sizeof(mpVertex) * 24 *  batchSize));
    batch.indices = static_cast<uint16_t*>(PushBackPermanentStorage(memory, sizeof(uint16_t) * 36 * batchSize));
    
    const vec3 voxVerts[8] = {
        {-scale, -scale,  scale},
        { scale, -scale,  scale},
        { scale,  scale,  scale},
        {-scale,  scale,  scale},
        {-scale,  scale, -scale},
        { scale,  scale, -scale},
        { scale, -scale, -scale},
        {-scale, -scale, -scale},
    };
    // TODO: Create vertex list
    
    for(float y = 0; y < localArea.Y; y++)
    {
        for(float x = 0; x < localArea.X; x++)
        {
            uint32_t i = static_cast<uint32_t>(x + (localArea.X * y));
            uint32_t iOffset = i * 24;
            
            vec2 position = {2.0f * scale * x, 2.0f * scale * y};
            float heightMap = Perlin(position.X, position.Y);
            vec3 voxelPos = {position.X, position.Y, (heightMap)};
            
            float colourMap = 0.5f + (0.5f * heightMap);
            vec3 colour = {0.0f, colourMap, 0.2f};
            
            batch.vertices[0 + iOffset]  = {voxVerts[0] + voxelPos, colour}; // TOP
            batch.vertices[1 + iOffset]  = {voxVerts[1] + voxelPos, colour};
            batch.vertices[2 + iOffset]  = {voxVerts[2] + voxelPos, colour};
            batch.vertices[3 + iOffset]  = {voxVerts[3] + voxelPos, colour};
            batch.vertices[4 + iOffset]  = {voxVerts[4] + voxelPos, colour}; // BOTTOM
            batch.vertices[5 + iOffset]  = {voxVerts[5] + voxelPos, colour};
            batch.vertices[6 + iOffset]  = {voxVerts[6] + voxelPos, colour};
            batch.vertices[7 + iOffset]  = {voxVerts[7] + voxelPos, colour};
            batch.vertices[8 + iOffset]  = {voxVerts[1] + voxelPos, colour}; // NORTH
            batch.vertices[9 + iOffset]  = {voxVerts[6] + voxelPos, colour};
            batch.vertices[10 + iOffset] = {voxVerts[5] + voxelPos, colour};
            batch.vertices[11 + iOffset] = {voxVerts[2] + voxelPos, colour};
            batch.vertices[12 + iOffset] = {voxVerts[3] + voxelPos, colour}; // SOUTH
            batch.vertices[13 + iOffset] = {voxVerts[4] + voxelPos, colour};
            batch.vertices[14 + iOffset] = {voxVerts[7] + voxelPos, colour};
            batch.vertices[15 + iOffset] = {voxVerts[0] + voxelPos, colour};
            batch.vertices[16 + iOffset] = {voxVerts[2] + voxelPos, colour}; // EAST
            batch.vertices[17 + iOffset] = {voxVerts[5] + voxelPos, colour};
            batch.vertices[18 + iOffset] = {voxVerts[4] + voxelPos, colour};
            batch.vertices[19 + iOffset] = {voxVerts[3] + voxelPos, colour};
            batch.vertices[20 + iOffset] = {voxVerts[0] + voxelPos, colour}; // WEST
            batch.vertices[21 + iOffset] = {voxVerts[7] + voxelPos, colour};
            batch.vertices[22 + iOffset] = {voxVerts[6] + voxelPos, colour};
            batch.vertices[23 + iOffset] = {voxVerts[1] + voxelPos, colour};
            
            for(uint16_t row = 0; row < 6; row++)
            {
                uint16_t rowOffset = row * 6 + 36 * static_cast<uint16_t>(i);
                uint16_t indexOffset = 4 * row + 24 * static_cast<uint16_t>(i);
                batch.indices[rowOffset]     = 0 + indexOffset;
                batch.indices[rowOffset + 1] = 1 + indexOffset;
                batch.indices[rowOffset + 2] = 2 + indexOffset;
                batch.indices[rowOffset + 3] = 2 + indexOffset;
                batch.indices[rowOffset + 4] = 3 + indexOffset;
                batch.indices[rowOffset + 5] = 0 + indexOffset;
            }
        }
    }
    return batch;
}

static void PrepareRenderData(mpMemory *memory, mpVoxelData *voxelData)
{
    voxelData->pBatches = static_cast<mpBatch*>(PushBackPermanentStorage(memory, sizeof(mpBatch) * voxelData->batchCount));
    
    vec2 area = {30.0f, 30.0f};
    voxelData->voxelsPerBatch = static_cast<uint32_t>(area.X * area.Y);
    
    for(uint32_t batchIndex = 0; batchIndex < voxelData->batchCount; batchIndex++)
        voxelData->pBatches[batchIndex] = CreateBatch(memory, voxelData->voxelScale, {0.0f, 0.0f}, area);
}

inline static void ProcessKeyToCameraControl(const mpEventReceiver *const receiver, mpKeyEvent key, bool32 *controlValue)
{
    if(receiver->keyPressedEvents & key)
        (*controlValue) = true;
    else if(receiver->keyReleasedEvents & key)
        (*controlValue) = false;
}

static void UpdateCamera(const mpEventReceiver *const receiver, mpCamera *const camera, mpCameraControls *const cameraControls, float aspectRatio, float timestep)
{
    ProcessKeyToCameraControl(receiver, MP_KEY_W, &cameraControls->rUp);
    ProcessKeyToCameraControl(receiver, MP_KEY_S, &cameraControls->rDown);
    ProcessKeyToCameraControl(receiver, MP_KEY_A, &cameraControls->rLeft);
    ProcessKeyToCameraControl(receiver, MP_KEY_D, &cameraControls->rRight);
    ProcessKeyToCameraControl(receiver, MP_KEY_UP, &cameraControls->tForward);
    ProcessKeyToCameraControl(receiver, MP_KEY_DOWN, &cameraControls->tBackward);
    ProcessKeyToCameraControl(receiver, MP_KEY_LEFT, &cameraControls->tLeft);
    ProcessKeyToCameraControl(receiver, MP_KEY_RIGHT, &cameraControls->tRight);
    
    if(cameraControls->rUp)
        camera->pitch += camera->sensitivity * timestep;
    else if(cameraControls->rDown)
        camera->pitch -= camera->sensitivity * timestep;
    if(cameraControls->rLeft)
        camera->yaw += camera->sensitivity * timestep;
    else if(cameraControls->rRight)
        camera->yaw -= camera->sensitivity * timestep;
    
    if(camera->pitch > camera->pitchClamp)
        camera->pitch = camera->pitchClamp;
    else if(camera->pitch < -camera->pitchClamp)
        camera->pitch = -camera->pitchClamp;
    
    // TODO: clamp pitch
    vec3 front = {cosf(camera->pitch) * cosf(camera->yaw), cosf(camera->pitch) * sinf(camera->yaw), sinf(camera->pitch)};
    vec3 left = {sinf(camera->yaw), -cosf(camera->yaw), 0.0f};
    if(cameraControls->tForward)
        camera->position += front * camera->speed * timestep;
    else if(cameraControls->tBackward)
        camera->position -= front * camera->speed * timestep;
    if(cameraControls->tLeft)
        camera->position -= left * camera->speed * timestep;
    else if(cameraControls->tRight)
        camera->position += left * camera->speed * timestep;
    
    camera->view = LookAt(camera->position, camera->position + front, {0.0f, 0.0f, 1.0f});
    camera->projection = Perspective(camera->fov, aspectRatio, 0.1f, 20.0f);
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
    mpCallbacks callbacks = {};
    mpWindowData windowData = {};
    Win32CreateWindow(&windowData, &callbacks); // TODO: Change Win32CreateWindow() to CreateWindow()
    // TODO: Prepare win32 sound
    
    mpMemory memory = {};
    memory.PermanentStorageSize = MegaBytes(64);
    memory.TransientStorageSize = GigaBytes(1);
    uint64_t totalSize = memory.PermanentStorageSize + memory.TransientStorageSize;
    memory.CombinedStorage = malloc(totalSize);
    memset(memory.CombinedStorage, 0, totalSize);
    memory.PermanentStorage = memory.CombinedStorage;
    memory.TransientStorage = static_cast<uint8_t*>(memory.CombinedStorage) + memory.PermanentStorageSize;
    
    mpVoxelData voxelData = {};
    voxelData.gridXWidth = 50;
    voxelData.voxelScale = 0.2f;
    voxelData.batchCount = 1;
    PrepareRenderData(&memory, &voxelData);
    
    mpCamera camera = {};
    camera.speed = 2.0f;
    camera.sensitivity = 2.0f;
    camera.fov = PI32 / 3.0f;
    camera.model = Mat4x4Identity();
    camera.position = {2.0f, 2.0f, 2.0f};
    camera.pitchClamp = (PI32 / 2.0f) - 0.01f;
    
    mpRenderer renderer = nullptr;
    mpVulkanInit(&renderer, &memory, &windowData, &voxelData, &callbacks);
    
    mpEventReceiver eventReceiver = {};
    mpCameraControls cameraControls = {};
    mpFPSsampler fpsSampler = {1000,0,0};
    
    float timestep = 0.0f;
    int64_t lastCounter = 0, perfCountFrequency = 0;
    Win32PrepareClock(&lastCounter, &perfCountFrequency);
    
    windowData.running = true;
    windowData.hasResized = false; // WM_SIZE is triggered at startup, so we need to reset hasResized before the loop
    while(windowData.running)
    {
        Win32PollEvents(&eventReceiver);
        
        UpdateCamera(&eventReceiver, &camera, &cameraControls, static_cast<float>(windowData.width) / static_cast<float>(windowData.height), timestep);
        
        mpVulkanUpdate(&renderer, &voxelData, &camera, &windowData);
        
        windowData.hasResized = false;
        ResetEventReceiver(&eventReceiver);
        timestep = Win32UpdateClock(&lastCounter, perfCountFrequency);
        UpdateFpsSampler(&fpsSampler, timestep);
    }
    
    mpVulkanCleanup(&renderer, voxelData.batchCount);
    
    return 0;
}