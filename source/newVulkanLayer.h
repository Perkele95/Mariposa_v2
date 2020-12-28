#pragma once

#include "core.h"

void mpVkInit(mpCore *core, mpMemoryRegion *vulkanMemory, mpMemoryRegion *tempMemory);
void mpVkUpdate(mpCore &core);
void mpVkCleanup(mpCore &core);

void mpVkCreateGeometryBuffer(mpCore &core);
void mpVkDrawMesh(mpCore &core);