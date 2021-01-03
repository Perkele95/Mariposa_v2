#pragma once

#include "core.h"

#ifdef _WIN32
#include "vulkan\vulkan_win32.h"
#endif

void mpVulkanInit(mpCore *core, mpMemoryRegion vulkanMemory, mpMemoryRegion tempMemory, bool32 enableValidation);
void mpVulkanUpdate(mpCore *core, mpMemoryRegion vulkanMemory, mpMemoryRegion tempMemory);
void mpVulkanCleanup(mpHandle *rendererHandle);

void mpVkRecreateGUIBuffers(mpHandle rendererHandle, const mpGuiData &guiData);
void mpVkRecreateGeometryBuffer(mpHandle rendererHandle, mpMesh &mesh, const vec3Int index);
