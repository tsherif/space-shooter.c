////////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
// 
// Copyright (c) 2021 Tarek Sherif
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////////

#ifndef CREATE_OPENGL_WINDOW_H
#define CREATE_OPENGL_WINDOW_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////////////////
// Uses WGL extensions:
// - https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
// - https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CreateOpenGLWindowLoggingFunc)(const char*);

typedef struct {
    WNDPROC winCallback;
    CreateOpenGLWindowLoggingFunc errorLogFunction;
    char* title;
    int32_t majorVersion;
    int32_t minorVersion;
    int32_t width;
    int32_t height;
    int32_t msaaSamples;
    bool depth;
    bool stencil;
    bool vsync;
} CreateOpenGLWindowArgs;

HWND createOpenGLWindow(CreateOpenGLWindowArgs* args);
void destroyOpenGLWindow(HWND window);

#ifdef CREATE_OPENGL_WINDOW_IMPLEMENTATION

#define CREATE_OPENGL_WINDOW_MAX_PIXEL_FORMATS 128

/////////////////////////////////////
// OpenGL constants
/////////////////////////////////////

#define GL_TRUE                           1

/////////////////////////////////////
// WGL constants
/////////////////////////////////////

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_ACCELERATION_ARB              0x2003
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_RED_BITS_ARB                  0x2015
#define WGL_GREEN_BITS_ARB                0x2017
#define WGL_BLUE_BITS_ARB                 0x2019
#define WGL_ALPHA_BITS_ARB                0x201B
#define WGL_FULL_ACCELERATION_ARB         0x2027
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001

/////////////////////////////////////
// WGL loading helper functions 
/////////////////////////////////////

#define DECLARE_WGL_EXT_FUNC(returnType, name, ...) typedef returnType (WINAPI *name##FUNC)(__VA_ARGS__);\
    static name##FUNC name = (name##FUNC)0;
#define LOAD_WGL_EXT_FUNC(name) name = (name##FUNC) wglGetProcAddress(#name)

/////////////////////////////////////
// Set up WGL function pointers
/////////////////////////////////////

DECLARE_WGL_EXT_FUNC(BOOL, wglChoosePixelFormatARB, HDC hdc, const int32_t *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int32_t *piFormats, UINT *nNumFormats);
DECLARE_WGL_EXT_FUNC(HGLRC, wglCreateContextAttribsARB, HDC hDC, HGLRC hshareContext, const int32_t *attribList);
DECLARE_WGL_EXT_FUNC(BOOL, wglSwapIntervalEXT, int32_t interval);
DECLARE_WGL_EXT_FUNC(BOOL, wglGetPixelFormatAttribivARB, HDC hdc, int32_t iPixelFormat, int32_t iLayerPlane, UINT nAttributes, const int32_t *piAttributes, int32_t *piValues);

static const char WIN_CLASS_NAME[] = "CREATE_OPENGL_WINDOW_CLASS"; 

static void createOpenGLWindowDefaultErrorLog(const char* message) {
    MessageBoxA(NULL, message, "FAILURE", MB_OK);   
}

HWND createOpenGLWindow(CreateOpenGLWindowArgs* args) {
    HINSTANCE instance = GetModuleHandle(NULL);
    int32_t windowX      = CW_USEDEFAULT;
    int32_t windowY      = CW_USEDEFAULT;
    int32_t windowWidth  = args->width;
    int32_t windowHeight = args->height;
    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    CreateOpenGLWindowLoggingFunc errorLog = args->errorLogFunction ? args->errorLogFunction : createOpenGLWindowDefaultErrorLog;

    WNDCLASSEXA winClass = {
        .cbSize = sizeof(winClass),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = args->winCallback,
        .hInstance = instance,
        .hIcon = LoadIcon(instance, IDI_APPLICATION),
        .hIconSm = LoadIcon(instance, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = NULL,
        .lpszClassName = WIN_CLASS_NAME
    };

    if (!RegisterClassExA(&winClass)) {
        errorLog("Failed to register window class.");

        return NULL;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a dummy window so we can get WGL extension functions.
    // The reason we can't use the same window later is that SetPixelFormat can only be 
    // called once on a window:
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat#remarks
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    HWND dummyWindow = CreateWindowA(WIN_CLASS_NAME, "DUMMY", WS_POPUP, 0, 0, 1, 1, NULL,  NULL, instance, NULL);

    if (!dummyWindow) {
        errorLog("Failed to create window.");

        return NULL;
    }

    if (windowWidth == 0 || windowHeight == 0) {
        HMONITOR monitor = MonitorFromWindow(dummyWindow, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo = { .cbSize = sizeof(MONITORINFO) };
        GetMonitorInfo(monitor, &monitorInfo);
        windowX      = monitorInfo.rcMonitor.left;
        windowY      = monitorInfo.rcMonitor.top;
        windowWidth  = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        windowHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        windowStyle = WS_POPUP;
    }

    HDC dummyContext = GetDC(dummyWindow);

    PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_SUPPORT_OPENGL,
        .iPixelType = PFD_TYPE_RGBA,        
        .cColorBits = 32,                   
        .cDepthBits = 24,           
        .cStencilBits = 8,                 
        .iLayerType = PFD_MAIN_PLANE
    };
    
    
    int32_t dummyPixelFormat = ChoosePixelFormat(dummyContext, &pixelFormatDescriptor);
    SetPixelFormat(dummyContext, dummyPixelFormat, &pixelFormatDescriptor);
    HGLRC dummyGL = wglCreateContext(dummyContext);
    wglMakeCurrent(dummyContext, dummyGL);

    LOAD_WGL_EXT_FUNC(wglChoosePixelFormatARB);
    LOAD_WGL_EXT_FUNC(wglCreateContextAttribsARB);
    LOAD_WGL_EXT_FUNC(wglSwapIntervalEXT);
    LOAD_WGL_EXT_FUNC(wglGetPixelFormatAttribivARB);

    if (!wglCreateContextAttribsARB || !wglCreateContextAttribsARB) {
        errorLog("Failed to load WGL functions.");
        return NULL;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummyGL);
    ReleaseDC(dummyWindow, dummyContext);
    DestroyWindow(dummyWindow);

    /////////////////////////////////////////////
    // Create real window and rendering context
    /////////////////////////////////////////////

    HWND window = CreateWindow(
        WIN_CLASS_NAME,
        args->title,
        windowStyle,
        windowX, windowY,
        windowWidth, windowHeight,
        NULL, 
        NULL,
        instance,
        NULL
    );

    if (!window) {
        errorLog("Failed to create window.");

        return NULL;
    }

    HDC deviceContext = GetDC(window);

    UINT formatCount = 0;
    int32_t pixelFormats[CREATE_OPENGL_WINDOW_MAX_PIXEL_FORMATS] = { 0 };
    BOOL success = wglChoosePixelFormatARB(
        deviceContext, 
        (int32_t[]) {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_RED_BITS_ARB, 8,
            WGL_BLUE_BITS_ARB, 8,
            WGL_RED_BITS_ARB, 8,
            WGL_ALPHA_BITS_ARB, 8,
            0
        }, 
        NULL,
        CREATE_OPENGL_WINDOW_MAX_PIXEL_FORMATS,
        pixelFormats,
        &formatCount
    );

    if (!success || formatCount == 0) {
        errorLog("Failed to find valid pixel format.");
        return NULL;
    }

    // Find format with most samples but at most the number requested
    int32_t formatIndex = 0;
    int32_t bestSamples = 0;
    int32_t samples = 0;
    wglGetPixelFormatAttribivARB(deviceContext, pixelFormats[0], 0, 1, (int32_t []) { WGL_SAMPLES_ARB } , &samples);

    for (UINT i = 1; i < formatCount; ++i) {
        wglGetPixelFormatAttribivARB(deviceContext, pixelFormats[i], 0, 1, (int32_t []) { WGL_SAMPLES_ARB } , &samples);

        if (samples <= args->msaaSamples && samples > bestSamples) {
            bestSamples = samples;
            formatIndex = i;
        }
    }

    int32_t pixelFormat = pixelFormats[formatIndex];
    DescribePixelFormat(deviceContext, pixelFormat, sizeof(pixelFormatDescriptor), &pixelFormatDescriptor);
    SetPixelFormat(deviceContext, pixelFormat, &pixelFormatDescriptor);

    HGLRC gl = wglCreateContextAttribsARB(deviceContext, NULL,  (int32_t[]) {
        WGL_CONTEXT_MAJOR_VERSION_ARB, args->majorVersion,
        WGL_CONTEXT_MINOR_VERSION_ARB, args->minorVersion,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    });

    if (!gl) {
        errorLog("Failed to load OpenGL.");
        return NULL;
    }

    wglMakeCurrent(deviceContext, gl);

    if (args->vsync && wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1);
    }

    return window;
}

void destroyOpenGLWindow(HWND window) {
    HGLRC gl = wglGetCurrentContext();
    HDC deviceContext = GetDC(window);
    
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(gl);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);
}

#endif // CREATE_OPENGL_WINDOW_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // CREATE_OPENGL_WINDOW_H
