#pragma once

#include "core.h"
#include "events.h"
#include <windows.h>

void PlatformCreateWindow(mpWindowData *windowData, const char *name);
mpCallbacks PlatformGetCallbacks();
void PlatformPollEvents(mpEventReceiver *pReceiver);
void PlatformPrepareClock();
// Returns timestep
float PlatformUpdateClock();
