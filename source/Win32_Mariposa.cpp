#include "Win32_Mariposa.h"
#include "..\Vulkan\Include\vulkan\vulkan_win32.h"

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
            //MP_ASSERT(!"Keyboard input came in through a non-dispatch message!");
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

void Win32CloseFile(mpFile *file)
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

void PlatformCreateWindow(mpWindowData *windowData)
{
    gWin32WindowData = &(*windowData);
    gHInstance = GetModuleHandleA(0);
    
    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = gHInstance;
    //WindowClass.hIcon;
    windowClass.lpszClassName = "MariposaWindowClass";
    
    if(RegisterClassA(&windowClass))
    {
        gWindow = CreateWindowExA(0, windowClass.lpszClassName, "Mariposa",
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
    callbacks.mpCloseFile = Win32CloseFile;
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

static void ProcessKeyEvents(mpEventReceiver *pReceiver, uint32_t keyCode, mpKeyState state, bool32 altKeyDown)
{
    switch (keyCode) {
    case 'W':
        DispatchKeyEvent(pReceiver, MP_KEY_W, state);
        break;
    case 'A':
        DispatchKeyEvent(pReceiver, MP_KEY_A, state);
        break;
    case 'S':
        DispatchKeyEvent(pReceiver, MP_KEY_S, state);
        break;
    case 'D':
        DispatchKeyEvent(pReceiver, MP_KEY_D, state);
        break;
    case 'Q':
        DispatchKeyEvent(pReceiver, MP_KEY_Q, state);
        break;
    case 'E':
        DispatchKeyEvent(pReceiver, MP_KEY_E, state);
        break;
    case VK_UP:
        DispatchKeyEvent(pReceiver, MP_KEY_UP, state);
        break;
    case VK_DOWN:
        DispatchKeyEvent(pReceiver, MP_KEY_DOWN, state);
        break;
    case VK_LEFT:
        DispatchKeyEvent(pReceiver, MP_KEY_LEFT, state);
        break;
    case VK_RIGHT:
        DispatchKeyEvent(pReceiver, MP_KEY_RIGHT, state);
        break;
    case VK_SPACE:
        DispatchKeyEvent(pReceiver, MP_KEY_SPACE, state);
        break;
    case VK_ESCAPE:
        DispatchKeyEvent(pReceiver, MP_KEY_ESCAPE, state);
        break;
    case VK_CONTROL:
        DispatchKeyEvent(pReceiver, MP_KEY_CONTROL, state);
        break;
    case VK_SHIFT:
        DispatchKeyEvent(pReceiver, MP_KEY_SHIFT, state);
        break;
    default: break;
    }
    if(altKeyDown && (state == MP_KEY_PRESS))
    {
        if(keyCode == VK_F4)
        {
            gWin32WindowData->running = false;
        }
        else if(keyCode == VK_RETURN)
        {
            Win32ToggleFullScreen();
        }
    }
}

void PlatformPollEvents(mpEventReceiver *pReceiver)
{
    MSG message;
    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
            case WM_MOUSEMOVE:
            {
                
            } break;
            case WM_LBUTTONDOWN:
            {
                
            } break;
            case WM_LBUTTONUP:
            {
                
            } break;
            case WM_RBUTTONDOWN:
            {
                
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
                uint32_t keyCode = (uint32_t)message.wParam;
                bool32 altKeyDown = (message.lParam & (1 << 29));
                ProcessKeyEvents(pReceiver, keyCode, MP_KEY_RELEASE, altKeyDown);
            } break;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                uint32_t keyCode = (uint32_t)message.wParam;
                bool32 altKeyDown = (message.lParam & (1 << 29));
                ProcessKeyEvents(pReceiver, keyCode, MP_KEY_PRESS, altKeyDown);
            } break;
            
            default:
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } break;
        }
    }
}

void PlatformPrepareClock(int64_t *lastCounter, int64_t *perfCountFrequency)
{
    LARGE_INTEGER lastCounter_Large = {}, perfCountFrequencyResult = {};
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    *perfCountFrequency = perfCountFrequencyResult.QuadPart;
    QueryPerformanceCounter(&lastCounter_Large);
    *lastCounter = lastCounter_Large.QuadPart;
}

float PlatformUpdateClock(int64_t *lastCounter, int64_t perfCountFrequency)
{
    LARGE_INTEGER endCounter = {};
    QueryPerformanceCounter(&endCounter);
    float result = (float)(endCounter.QuadPart - (*lastCounter)) / (float)perfCountFrequency;
    *lastCounter = endCounter.QuadPart;
    
    return result;
}
