#pragma once
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t mpKeyCode;

enum mpVirtualKeyCodes : mpKeyCode
{
    MP_KEY_LMBUTTON = 0x01,
    MP_KEY_RMBUTTON = 0x02,
    MP_KEY_SHIFT = 0x10,
    MP_KEY_CTRL = 0x11,
    MP_KEY_ALT = 0x12,
    MP_KEY_NUM0 = 0x30,
    MP_KEY_NUM1 = 0x31,
    MP_KEY_NUM2 = 0x32,
    MP_KEY_NUM3 = 0x33,
    MP_KEY_NUM4 = 0x34,
    MP_KEY_NUM5 = 0x35,
    MP_KEY_NUM6 = 0x36,
    MP_KEY_NUM7 = 0x37,
    MP_KEY_NUM8 = 0x38,
    MP_KEY_NUM9 = 0x39,
    MP_KEY_A = 0x41,
    MP_KEY_D = 0x44,
    MP_KEY_S = 0x53,
    MP_KEY_W = 0x57,
};

enum mpKeyEvent
{
    MP_KEY_EVENT_W = 0x00000001,
    MP_KEY_EVENT_S = 0x00000002,
    MP_KEY_EVENT_A = 0x00000004,
    MP_KEY_EVENT_D = 0x00000008,
    MP_KEY_EVENT_Q = 0x00000010,
    MP_KEY_EVENT_E = 0x00000020,
    MP_KEY_EVENT_UP = 0x00000040,
    MP_KEY_EVENT_DOWN = 0x00000080,
    MP_KEY_EVENT_LEFT = 0x00000100,
    MP_KEY_EVENT_RIGHT = 0x00000200,
    MP_KEY_EVENT_SPACE = 0x00000400,
    MP_KEY_EVENT_ESCAPE = 0x00000800,
    MP_KEY_EVENT_CONTROL = 0x00001000,
    MP_KEY_EVENT_SHIFT = 0x00002000,
    MP_KEY_EVENT_F = 0x00004000,
};

enum mpMouseEvent
{
    MP_MOUSE_MOVE = 0x00000001,
    MP_MOUSE_CLICK_LEFT = 0x00000002,
    MP_MOUSE_CLICK_RIGHT = 0x00000004,
    MP_MOUSE_CLICK_MIDDLE = 0x00000008,
};

struct mpEventHandler
{
    uint64_t keyPressEvents;
    uint32_t mouseEvents;
    int32_t mouseWheel, mouseX, mouseY, mouseDeltaX, mouseDeltaY, prevMouseX, prevMouseY;
};

inline void mpEventHandlerBegin(mpEventHandler &eventHandler)
{
    eventHandler.mouseDeltaX = eventHandler.mouseX - eventHandler.prevMouseX;
    eventHandler.mouseDeltaY = eventHandler.mouseY - eventHandler.prevMouseY;
}

inline void mpEventHandlerEnd(mpEventHandler &eventHandler)
{
    eventHandler.prevMouseX = eventHandler.mouseDeltaX;
    eventHandler.prevMouseY = eventHandler.mouseDeltaY;
    eventHandler.keyPressEvents = 0;
    eventHandler.mouseEvents = 0;
}