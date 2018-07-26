#if _WIN32
#include "platform.h"
#include "main.h"
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>
#include <stdio.h>

using namespace MRT;

static HDC DC;
static BITMAPINFO bmpInfo;
static HWND mainWindow;
static uint32 G_windowWidth;
static uint32 G_windowHeight;
static uint32 G_bufferWidth;
static uint32 G_bufferHeight;
static double freq;

uint64_t MRT_GetTime() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

float MRT_TimeDelta(uint64_t start, uint64_t stop) {
    return float((stop - start) / freq);
}

void MRT_PlatformInit() {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    freq = double(f.QuadPart);

    DWORD dwProcessList[2];
    DWORD count = GetConsoleProcessList(dwProcessList, 2);
    
    if (count == 1) { // we are the only process using this console, just close it
        FreeConsole();
    }
}

LRESULT CALLBACK MainWndProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;
    int32 mouseX = ((short*)&lParam)[0];
    int32 mouseY = ((short*)&lParam)[1];

    switch (uMsg) {
    case WM_KEYDOWN: {
        KeyState prevState = (lParam & (1 << 30)) ? MRT_DOWN : MRT_UP;
        KeyboardCallback(int(wParam), MRT_DOWN, prevState);
    } break;
    case WM_KEYUP:
        KeyboardCallback(int(wParam), MRT_UP, MRT_DOWN);
        break;
    case WM_LBUTTONUP:
        MouseCallback(mouseX, mouseY, MRT_UP, MRT_NONE);
        break;
    case WM_LBUTTONDOWN:
        MouseCallback(mouseX, mouseY, MRT_DOWN, MRT_NONE);
        break;
    case WM_RBUTTONUP:
        MouseCallback(mouseX, mouseY, MRT_NONE, MRT_UP);
        break;
    case WM_RBUTTONDOWN:
        MouseCallback(mouseX, mouseY, MRT_NONE, MRT_DOWN);
        break;
    case WM_DESTROY:
    case WM_CLOSE:
        WindowCallback(MRT_CLOSE);
        break;

    default:
        result = DefWindowProc(hWindow, uMsg, wParam, lParam);
    }

    return result;
}

void MRT_SetWindowTitle(const char *str) {
    SetWindowTextA(mainWindow, str);
}

void MRT_CreateWindow(uint32_t windowWidth, uint32_t windowHeight, uint32_t bufferWidth, uint32_t bufferHeight) {

    HMODULE hInstance = GetModuleHandle(NULL);

    WNDCLASSEX windowClass = { sizeof(windowClass) };
    windowClass.lpfnWndProc = MainWndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "MiniRayTracerMain";

    if (!RegisterClassEx(&windowClass)) {
        DebugBreak();
        exit(1);
    }

    DWORD windowStyle = WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
    DWORD windowStyleEx = 0;

    // adjust window so that drawable area is exactly the size we want
    RECT r = { 0, 0, LONG(windowWidth), LONG(windowHeight) };
    AdjustWindowRectEx(&r, windowStyle, FALSE, windowStyleEx);

    if ((r.right - r.left) == LONG(windowWidth)) { // AdjustWindowRect doesn't seem to work for some window styles
        int32 x_adjust = GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
        int32 y_adjust = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME) * 2;
        r.right += x_adjust;
        r.bottom += y_adjust;
    }

    mainWindow = CreateWindowEx(windowStyleEx, windowClass.lpszClassName, "MiniRayTracer", windowStyle, 5, 35,
                                (r.right - r.left), (r.bottom - r.top), NULL, NULL, hInstance, 0);

    DC = GetDC(mainWindow);

    // set stretch mode in case buffer size != window size
    // NOTE: stretch mode is reset if we release the DC
    //SetStretchBltMode(DC, HALFTONE); // bicubic-like, blurry. comment out if undesired
    //SetBrushOrgEx(DC, 0, 0, NULL);   // required after setting HALFTONE according to MSDN

    // tell Windows how to interpret our backbuffer
    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
    bmpInfo.bmiHeader.biWidth = bufferWidth;
    bmpInfo.bmiHeader.biHeight = bufferHeight; // y axis up
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    G_windowWidth  = windowWidth;
    G_windowHeight = windowHeight;
    G_bufferWidth  = bufferWidth;
    G_bufferHeight = bufferHeight;
}

void MRT_HandleMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void MRT_DrawToWindow(const uint32_t* backBuffer) {
    StretchDIBits(DC, 0, 0, G_windowWidth, G_windowHeight, 0, 0, G_bufferWidth, G_bufferHeight, backBuffer, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
}

void MRT_DebugPrint(const char *format, ...) {
    static char buffer[16384];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    fputs(buffer, stderr);
    OutputDebugStringA(buffer);
}

void MRT_Assert(bool cond) {
    if (!cond) DebugBreak();
}

void MRT_Assert(bool cond, const char *msg) {
    if (!cond) {
        MRT_DebugPrint(msg);
        DebugBreak();
    }
}

void MRT_LowerThreadPriority() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
}

void MRT_PlatformDestroy() {
    ReleaseDC(mainWindow, DC);
    FreeConsole();
}

void MRT_Sleep(uint32_t ms) {
    Sleep(ms);
}


#endif
