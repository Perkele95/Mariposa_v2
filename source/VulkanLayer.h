#pragma once

#include "core.h"

#ifdef _WIN32
#include "vulkan\vulkan_win32.h"
#endif

void mpVulkanInit(mpCore *core, mpMemoryRegion *vulkanMemory, mpMemoryRegion *tempMemory, bool32 enableValidation);
void mpVulkanUpdate(mpCore *core, mpMemoryRegion *vulkanMemory, mpMemoryRegion *tempMemory);
void mpVulkanCleanup(mpHandle *rendererHandle, uint32_t meshCount);

void mpVkRecreateGeometryBuffer(mpHandle rendererHandle, mpMesh *mesh, uint32_t index);
