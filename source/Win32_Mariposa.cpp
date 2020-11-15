#include "Win32_Mariposa.h"
#include "..\Vulkan\Include\vulkan\vulkan_win32.h"

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

void Win32CloseFile(mpFileHandle *handle)
{
    free(*handle);
}

mpFile Win32ReadFile(const char *filename)
{
    mpFile result = {};
    mpFileHandle fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    LARGE_INTEGER fileSize;
    
    if((fileHandle != INVALID_HANDLE_VALUE) && GetFileSizeEx(fileHandle, &fileSize))
    {
        mp_assert(fileSize.QuadPart < 0xFFFFFFFF);
        uint32_t fileSize32 = (uint32_t) fileSize.QuadPart;
        result.contents = malloc((size_t)fileSize.QuadPart);
        if(result.contents)
        {
            DWORD bytesRead;
            if(ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead)
            {
                result.size = fileSize32;
            }
            else
            {
                free(result.contents);
                result.contents = 0;
            }
            
        }
        CloseHandle(fileHandle);
    }
    return result;
}

bool32 Win32WriteFile(const char *filename, mpFile *file)
{
    bool32 result = false;
    mpFileHandle fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if(WriteFile(fileHandle, file->contents, file->size, &bytesWritten, 0))
            result = (file->size == bytesWritten);
        
        CloseHandle(fileHandle);
    }
    return result;
}

void Win32CreateWindow(mpWindowData *windowData, mpCallbacks* callbacks)
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
    callbacks->GetSurface = CreateWin32Surface;
    callbacks->mpCloseFile = Win32CloseFile;
    callbacks->mpReadFile = Win32ReadFile;
    callbacks->mpWriteFile = Win32WriteFile;
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
        DispatchKeyEvent(pReceiver, MP_KEY_W, state);
        break;
    case VK_RIGHT:
        DispatchKeyEvent(pReceiver, MP_KEY_W, state);
        break;
    case VK_SPACE:
        DispatchKeyEvent(pReceiver, MP_KEY_W, state);
        break;
    case VK_ESCAPE:
        DispatchKeyEvent(pReceiver, MP_KEY_W, state);
        break;
    case VK_CONTROL:
        DispatchKeyEvent(pReceiver, MP_KEY_W, state);
        break;
    case VK_SHIFT:
        DispatchKeyEvent(pReceiver, MP_KEY_W, state);
        break;
    default: break;
    }
    if(keyCode == VK_F4 && altKeyDown)
    {
        gWin32WindowData->running = false;
    }
    else if(keyCode == VK_RETURN && altKeyDown)
    {
        //Win32ToggleFullScreen(message.hwnd);
    }
}

void Win32PollEvents(mpEventReceiver *pReceiver)
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

void Win32PrepareClock(int64_t *lastCounter, int64_t *perfCountFrequency)
{
    LARGE_INTEGER lastCounter_Large = {}, perfCountFrequencyResult = {};
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    *perfCountFrequency = perfCountFrequencyResult.QuadPart;
    QueryPerformanceCounter(&lastCounter_Large);
    *lastCounter = lastCounter_Large.QuadPart;
}

float Win32UpdateClock(int64_t *lastCounter, int64_t perfCountFrequency)
{
    LARGE_INTEGER endCounter = {};
    QueryPerformanceCounter(&endCounter);
    float result = (float)(endCounter.QuadPart - (*lastCounter)) / (float)perfCountFrequency;
    *lastCounter = endCounter.QuadPart;
    
    return result;
}