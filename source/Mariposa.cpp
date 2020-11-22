#include "Win32_Mariposa.h"
#include "VulkanLayer.h"
#include "events.h"

#include <stdlib.h>
#include <stdio.h>

static void PrepareRenderData(mpMemory *memory, mpVoxelData *voxelData)
{
    mp_assert(voxelData->voxelScale > 0.0f);
    // Voxels points to the structure containing info about vertices and indices for each voxel.
    // All voxeldata is allcated on the heap, through PermanentStorage
    voxelData->vertices = static_cast<mpVertex*>(PushBackPermanentStorage(memory, sizeof(mpVertex) * 24 *  voxelData->voxelCount));
    voxelData->indices = static_cast<uint16_t*>(PushBackPermanentStorage(memory, sizeof(uint16_t) * 36 * voxelData->voxelCount));
    
    vec3 vertex_0 = {-voxelData->voxelScale, -voxelData->voxelScale,  voxelData->voxelScale};
    vec3 vertex_1 = { voxelData->voxelScale, -voxelData->voxelScale,  voxelData->voxelScale};
    vec3 vertex_2 = { voxelData->voxelScale,  voxelData->voxelScale,  voxelData->voxelScale};
    vec3 vertex_3 = {-voxelData->voxelScale,  voxelData->voxelScale,  voxelData->voxelScale};
    vec3 vertex_4 = {-voxelData->voxelScale,  voxelData->voxelScale, -voxelData->voxelScale};
    vec3 vertex_5 = { voxelData->voxelScale,  voxelData->voxelScale, -voxelData->voxelScale};
    vec3 vertex_6 = { voxelData->voxelScale, -voxelData->voxelScale, -voxelData->voxelScale};
    vec3 vertex_7 = {-voxelData->voxelScale, -voxelData->voxelScale, -voxelData->voxelScale};
    
    uint32_t gridX = 0, gridY = 0;
    for(uint32_t i = 0; i < voxelData->voxelCount; i++)
    {
        if(gridX == voxelData->gridXWidth)
        {
            gridY++;
            gridX = 0;
        }
        float xPos = 2.0f * voxelData->voxelScale * static_cast<float>(gridX);
        float yPos = 2.0f * voxelData->voxelScale * static_cast<float>(gridY);
        float heightMap = Perlin(xPos, yPos);
        vec3 offset = {xPos, yPos, (heightMap)};
        float colourMap = 0.5f + (0.5f * heightMap);
        vec3 colour = {0.0f, colourMap, colourMap};
        
        voxelData->vertices[0 + i * 24]  = {vertex_0 + offset, colour}; // TOP
        voxelData->vertices[1 + i * 24]  = {vertex_1 + offset, colour};
        voxelData->vertices[2 + i * 24]  = {vertex_2 + offset, colour};
        voxelData->vertices[3 + i * 24]  = {vertex_3 + offset, colour};
        voxelData->vertices[4 + i * 24]  = {vertex_4 + offset, colour}; // BOTTOM
        voxelData->vertices[5 + i * 24]  = {vertex_5 + offset, colour};
        voxelData->vertices[6 + i * 24]  = {vertex_6 + offset, colour};
        voxelData->vertices[7 + i * 24]  = {vertex_7 + offset, colour};
        voxelData->vertices[8 + i * 24]  = {vertex_1 + offset, colour}; // NORTH
        voxelData->vertices[9 + i * 24]  = {vertex_6 + offset, colour};
        voxelData->vertices[10 + i * 24] = {vertex_5 + offset, colour};
        voxelData->vertices[11 + i * 24] = {vertex_2 + offset, colour};
        voxelData->vertices[12 + i * 24] = {vertex_3 + offset, colour}; // SOUTH
        voxelData->vertices[13 + i * 24] = {vertex_4 + offset, colour};
        voxelData->vertices[14 + i * 24] = {vertex_7 + offset, colour};
        voxelData->vertices[15 + i * 24] = {vertex_0 + offset, colour};
        voxelData->vertices[16 + i * 24] = {vertex_2 + offset, colour}; // EAST
        voxelData->vertices[17 + i * 24] = {vertex_5 + offset, colour};
        voxelData->vertices[18 + i * 24] = {vertex_4 + offset, colour};
        voxelData->vertices[19 + i * 24] = {vertex_3 + offset, colour};
        voxelData->vertices[20 + i * 24] = {vertex_0 + offset, colour}; // WEST
        voxelData->vertices[21 + i * 24] = {vertex_7 + offset, colour};
        voxelData->vertices[22 + i * 24] = {vertex_6 + offset, colour};
        voxelData->vertices[23 + i * 24] = {vertex_1 + offset, colour};
        
        for(uint16_t row = 0; row < 6; row++)
        {
            uint16_t rowOffset = row * 6 + 36 * static_cast<uint16_t>(i);
            uint16_t indexOffset = 4 * row + 24 * static_cast<uint16_t>(i);
            voxelData->indices[rowOffset]     = 0 + indexOffset;
            voxelData->indices[rowOffset + 1] = 1 + indexOffset;
            voxelData->indices[rowOffset + 2] = 2 + indexOffset;
            voxelData->indices[rowOffset + 3] = 2 + indexOffset;
            voxelData->indices[rowOffset + 4] = 3 + indexOffset;
            voxelData->indices[rowOffset + 5] = 0 + indexOffset;
        }
        gridX++;
    }
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
    
    // TODO: clamp pitch
    vec3 front = {cosf(camera->pitch) * cosf(camera->yaw), cosf(camera->pitch) * sinf(camera->yaw), sinf(camera->pitch)};
    printf("pitch: %f, yaw: %f, front.Z: %f\n", camera->pitch, camera->yaw, front.Z);
    if(cameraControls->tForward)
        camera->position += front * camera->speed * timestep;
    else if(cameraControls->tBackward)
        camera->position -= front * camera->speed * timestep;
    if(cameraControls->tLeft)
        camera->position += front * camera->speed * timestep;
    else if(cameraControls->tRight)
        camera->position -= front * camera->speed * timestep;
    
    camera->view = LookAt(camera->position, camera->position + front, {0.0f, 0.0f, 1.0f});// * Mat4RotateY(camera->yaw) * Mat4RotateX(camera->pitch);
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
    Win32CreateWindow(&windowData, &callbacks);
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
    voxelData.voxelCount = 1000;
    voxelData.gridXWidth = 50;
    voxelData.voxelScale = 0.2f;
    PrepareRenderData(&memory, &voxelData);
    
    float timestep = 0.0f;
    int64_t lastCounter = 0, perfCountFrequency = 0;
    Win32PrepareClock(&lastCounter, &perfCountFrequency);
    
    mpCamera camera = {};
    camera.speed = 2.0f;
    camera.sensitivity = 2.0f;
    camera.fov = PI32 / 3.0f;
    camera.model = Mat4x4Identity();
    camera.position = {2.0f, 2.0f, 2.0f};
    
    mpCameraControls cameraControls = {};
    
    mpRenderer renderer = nullptr;
    mpVulkanInit(&renderer, &memory, &windowData, &voxelData, &callbacks);
    
    mpEventReceiver eventReceiver = {};
    
    mpFPSsampler fpsSampler = {1000,0,0};
    
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
    
    mpVulkanCleanup(&renderer);
    
    return 0;
}