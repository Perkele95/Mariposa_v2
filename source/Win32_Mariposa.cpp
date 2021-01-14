#include "Win32_Mariposa.h"
#include "vulkan\vulkan_win32.h"

static WINDOWPLACEMENT gWindowPlacement = { sizeof(WINDOWPLACEMENT) };
static mpWindowData *gWin32WindowData = nullptr;
static HINSTANCE gHInstance = 0;
static HWND gWindow = 0;

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            gWin32WindowData->width = (int32_t)(lParam & 0x0000FFFF);
            gWin32WindowData->height = (int32_t)(lParam >> 16);
            gWin32WindowData->hasResized = true;
        } break;

        case WM_CLOSE:
        {
            gWin32WindowData->running = false;
        } break;

        case WM_DESTROY:
        {
            gWin32WindowData->running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            mp_assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

void CreateWin32Surface(VkInstance instance, VkSurfaceKHR *surface)
{
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = gWindow;
    createInfo.hinstance = gHInstance;

    VkResult error = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, surface);
    mp_assert(!error);
}

void Win32FreeFileMemory(mpFile *file)
{
    free(file->handle);
}

mpFile Win32ReadFile(const char *filename)
{
    mpFile result = {};
    mpHandle fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    LARGE_INTEGER fileSize;

    if((fileHandle != INVALID_HANDLE_VALUE) && GetFileSizeEx(fileHandle, &fileSize))
    {
        mp_assert(fileSize.QuadPart < 0xFFFFFFFF);
        uint32_t fileSize32 = (uint32_t) fileSize.QuadPart;
        result.handle = malloc((size_t)fileSize.QuadPart);
        if(result.handle)
        {
            DWORD bytesRead;
            if(ReadFile(fileHandle, result.handle, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead)
            {
                result.size = fileSize32;
            }
            else
            {
                free(result.handle);
                result.handle = 0;
            }

        }
        CloseHandle(fileHandle);
    }
    return result;
}

bool32 Win32WriteFile(const char *filename, mpFile *file)
{
    bool32 result = false;
    mpHandle fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if(WriteFile(fileHandle, file->handle, file->size, &bytesWritten, 0))
            result = (file->size == bytesWritten);

        CloseHandle(fileHandle);
    }
    return result;
}

void PlatformCreateWindow(mpWindowData *windowData, const char *name)
{
    gWin32WindowData = windowData;
    gWin32WindowData->fullscreen = false;
    gHInstance = GetModuleHandleA(0);

    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = gHInstance;
    //WindowClass.hIcon;
    windowClass.lpszClassName = "MariposaWindowClass";

    if(RegisterClassA(&windowClass))
    {
        gWindow = CreateWindowExA(0, windowClass.lpszClassName, name,
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, gHInstance, NULL);
    }
}

mpCallbacks PlatformGetCallbacks()
{
    mpCallbacks callbacks = {};
    callbacks.GetSurface = CreateWin32Surface;
    callbacks.mpFreeFileMemory = Win32FreeFileMemory;
    callbacks.mpReadFile = Win32ReadFile;
    callbacks.mpWriteFile = Win32WriteFile;
    return callbacks;
}

static void Win32ToggleFullScreen()
{
    // Thanks to Raymond Chan we can do this:
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    DWORD style = GetWindowLong(gWindow, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {
        HMONITOR monitor = MonitorFromWindow(gWindow, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(gWindow, &gWindowPlacement) && GetMonitorInfoA(monitor, &mi))
        {
            SetWindowLong(gWindow, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(gWindow, HWND_TOP,
                        mi.rcMonitor.left, mi.rcMonitor.top,
                        mi.rcMonitor.right - mi.rcMonitor.left,
                        mi.rcMonitor.bottom - mi.rcMonitor.top,
                        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(gWindow, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(gWindow, &gWindowPlacement);
        SetWindowPos(gWindow, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

static void ProcessKeyEvents(mpEventHandler &eventHandler, uint32_t keyCode, bool32 altKeyDown)
{
    switch (keyCode) {
    case 'W':
        eventHandler.keyPressEvents |= MP_KEY_EVENT_W;
        break;
    case 'A':
        eventHandler.keyPressEvents |= MP_KEY_EVENT_A;
        break;
    case 'S':
        eventHandler.keyPressEvents |= MP_KEY_EVENT_S;
        break;
    case 'D':
        eventHandler.keyPressEvents |= MP_KEY_EVENT_D;
        break;
    case 'Q':
        eventHandler.keyPressEvents |= MP_KEY_EVENT_Q;
        break;
    case 'E':
        eventHandler.keyPressEvents |= MP_KEY_EVENT_E;
        break;
    case 'F':
        eventHandler.keyPressEvents |= MP_KEY_EVENT_F;
        break;
    case VK_UP:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_UP;
        break;
    case VK_DOWN:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_DOWN;
        break;
    case VK_LEFT:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_LEFT;
        break;
    case VK_RIGHT:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_RIGHT;
        break;
    case VK_SPACE:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_SPACE;
        break;
    case VK_ESCAPE:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_ESCAPE;
        break;
    case VK_CONTROL:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_CONTROL;
        break;
    case VK_SHIFT:
        eventHandler.keyPressEvents |= MP_KEY_EVENT_SHIFT;
        break;
    default: break;
    }
    if(altKeyDown){
        if(keyCode == VK_F4){
            gWin32WindowData->running = false;
        }
        else if(keyCode == VK_RETURN){
            Win32ToggleFullScreen();
            gWin32WindowData->fullscreen = !gWin32WindowData->fullscreen;
        }
    }
}

void PlatformSetMousePos(int32_t localX, int32_t localY)
{
    // TODO: only check getwindowrect when window size/position updates
    RECT windowRect;
    GetWindowRect(gWindow, &windowRect);
    const int32_t x = windowRect.left + localX;
    const int32_t y = windowRect.top + localY;
    SetCursorPos(x, y);
}

void PlatformSetCursorVisbility(bool32 show)
{
    ShowCursor(show);
}

bool32 PlatformIsKeyDown(mpKeyCode key)
{
    // Bitmask ensures checking of high order bits according to win32 docs
    return static_cast<bool32>(GetKeyState(key) & 0xFF00);
}

void PlatformPollEvents(mpEventHandler &eventHandler)
{
    MSG message;
    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE | PM_QS_INPUT))
    {
        switch (message.message)
        {
            case WM_MOUSEMOVE:
            {
                eventHandler.mouseX = message.lParam & 0x0000FFFF;
                eventHandler.mouseY = static_cast<int32_t>(static_cast<uint32_t>(message.lParam) >> 16);
                eventHandler.mouseEvents |= MP_MOUSE_MOVE;
            } break;
            case WM_LBUTTONDOWN:
            {
                eventHandler.mouseEvents |= MP_MOUSE_CLICK_LEFT;
            } break;
            case WM_LBUTTONUP:
            {

            } break;
            case WM_RBUTTONDOWN:
            {
                eventHandler.mouseEvents |= MP_MOUSE_CLICK_RIGHT;
            } break;
            case WM_RBUTTONUP:
            {

            } break;
            case WM_MBUTTONDOWN:
            {

            } break;
            case WM_MBUTTONUP:
            {

            } break;
            case WM_MOUSEWHEEL:
            {

            } break;

            case WM_SYSKEYUP:
            case WM_KEYUP:
            {

            } break;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                uint32_t keyCode = (uint32_t)message.wParam;
                bool32 altKeyDown = (message.lParam & (1 << 29));
                ProcessKeyEvents(eventHandler, keyCode, altKeyDown);
            } break;

            default:
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } break;
        }
    }
}

static int64_t perfCountFrequency = 0, lastCounter = 0;

void PlatformPrepareClock()
{
    LARGE_INTEGER lastCounter_Large = {}, perfCountFrequencyResult = {};
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    perfCountFrequency = perfCountFrequencyResult.QuadPart;
    QueryPerformanceCounter(&lastCounter_Large);
    lastCounter = lastCounter_Large.QuadPart;
}
float PlatformUpdateClock()
{
    LARGE_INTEGER counter = {};
    QueryPerformanceCounter(&counter);
    float timestep = static_cast<float>(counter.QuadPart - lastCounter) / static_cast<float>(perfCountFrequency);
    lastCounter = counter.QuadPart;

    return timestep;
}
