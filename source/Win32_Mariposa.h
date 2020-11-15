#pragma once

#include "core.h"
#include "events.h"
#include <windows.h>

void Win32CreateWindow(mpWindowData *windowData, mpCallbacks* callbacks);
void Win32PollEvents(mpEventReceiver *pReceiver);
void Win32PrepareClock(int64_t *lastCounter, int64_t *perfCountFrequency);
float Win32UpdateClock(int64_t *lastCounter, int64_t perfCountFrequency);
