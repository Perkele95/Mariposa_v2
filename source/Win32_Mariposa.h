#pragma once

#include "core.h"
#include "events.h"
#include <windows.h>

void PlatformCreateWindow(mpWindowData *windowData, const char *name);
mpCallbacks PlatformGetCallbacks();
bool32 PlatformIsKeyDown(mpKeyCode key);
void PlatformPollEvents(mpEventHandler &eventHandler);
void PlatformPrepareClock();
// Returns timestep
float PlatformUpdateClock();
