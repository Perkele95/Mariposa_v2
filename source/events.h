#pragma once
#include <stdint.h>
#include <stdlib.h>

enum mpKeyEvent
{
    MP_KEY_W = 0x00000001,
    MP_KEY_S = 0x00000002,
    MP_KEY_A = 0x00000004,
    MP_KEY_D = 0x00000008,
    MP_KEY_Q = 0x00000010,
    MP_KEY_E = 0x00000020,
    MP_KEY_UP = 0x00000040,
    MP_KEY_DOWN = 0x00000080,
    MP_KEY_LEFT = 0x00000100,
    MP_KEY_RIGHT = 0x00000200,
    MP_KEY_SPACE = 0x00000400,
    MP_KEY_ESCAPE = 0x00000800,
    MP_KEY_CONTROL = 0x00001000,
    MP_KEY_SHIFT = 0x00002000,
};

enum mpMouseEvent
{
    MP_MOUSE_MOVE = 0x00000001,
    MP_MOUSE_CLICK_LEFT = 0x00000002,
    MP_MOUSE_CLICK_RIGHT = 0x00000004,
    MP_MOUSE_CLICK_MIDDLE = 0x00000008,
};

enum mpEventState { MP_EVENT_STATE_RELEASE = 0, MP_EVENT_STATE_PRESS = 1 };

struct mpEventReceiver
{
    uint64_t keyPressedEvents;
    uint64_t keyReleasedEvents;
    uint32_t mousePressedEvents;
    uint32_t mouseReleasedEvents;
    int32_t mouseX, mouseY, mouseWheel;
};

inline void DispatchKeyEvent(mpEventReceiver *const pReceiver, mpKeyEvent event, mpEventState state)
{
    if(state)
        pReceiver->keyPressedEvents |= event;
    else
        pReceiver->keyReleasedEvents |= event;
}

inline void DispatchMouseEvent(mpEventReceiver *const pReceiver, mpMouseEvent event, mpEventState state)
{
    if(state)
        pReceiver->mousePressedEvents |= event;
    else
        pReceiver->mouseReleasedEvents |= event;
}

inline void DispatchMouseMove(mpEventReceiver *const pReceiver, int32_t x, int32_t y)
{
    pReceiver->mouseX = x;
    pReceiver->mouseX = y;
}

inline void ResetEventReceiver(mpEventReceiver *const pReceiver)
{
    pReceiver->keyPressedEvents = 0;
    pReceiver->keyReleasedEvents = 0;
    pReceiver->mousePressedEvents = 0;
    pReceiver->mouseReleasedEvents = 0;
}