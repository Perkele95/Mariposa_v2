#pragma once

#include "core.h"
#include "events.h"
#include <windows.h>

void PlatformCreateWindow(mpWindowData *windowData, const char *name);
mpCallbacks PlatformGetCallbacks();
void PlatformSetMousePos(int32_t localX, int32_t localY);
void PlatformSetCursorVisbility(bool32 show);
bool32 PlatformIsKeyDown(mpKeyCode key);
void PlatformPollEvents(mpEventHandler &eventHandler);
void PlatformPrepareClock();
// Returns timestep
float PlatformUpdateClock();
