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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char* title;
	int majorVersion;
	int minorVersion;
	WNDPROC winCallback;
	int width;
	int height;
} CreateOpenGLWindowArgs;

HWND createOpenGLWindow(CreateOpenGLWindowArgs* args);

#ifdef CREATE_OPENGL_WINDOW_IMPLEMENTATION

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
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
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

DECLARE_WGL_EXT_FUNC(BOOL, wglChoosePixelFormatARB, HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
DECLARE_WGL_EXT_FUNC(HGLRC, wglCreateContextAttribsARB, HDC hDC, HGLRC hshareContext, const int *attribList);
DECLARE_WGL_EXT_FUNC(BOOL, wglSwapIntervalEXT, int interval);

static const char WIN_CLASS_NAME[] = "CREATE_OPENGL_WINDOW_CLASS"; 

HWND createOpenGLWindow(CreateOpenGLWindowArgs* args) {
    HINSTANCE instance = GetModuleHandle(NULL);

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
        MessageBoxA(NULL, "Failed to register window class!", "FAILURE", MB_OK);

        return NULL;
    }

    ////////////////////////////////////////////////////////////////////
    // Create a dummy window so we can get WGL extension functions
    ////////////////////////////////////////////////////////////////////

    HWND dummyWindow = CreateWindowA(WIN_CLASS_NAME, "DUMMY", WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, NULL,  NULL, instance, NULL);

    if (!dummyWindow) {
        MessageBoxA(NULL, "Failed to create window!", "FAILURE", MB_OK);

        return NULL;
    }

    HDC dummyContext = GetDC(dummyWindow);

    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,        
        .cColorBits = 32,                   
        .cDepthBits = 24,           
        .cStencilBits = 8,                 
        .iLayerType = PFD_MAIN_PLANE
    };
    
    
    int pixelFormat = ChoosePixelFormat(dummyContext, &pfd);
    SetPixelFormat(dummyContext, pixelFormat, &pfd);
    HGLRC dummyGL = wglCreateContext(dummyContext);
    wglMakeCurrent(dummyContext, dummyGL);

    LOAD_WGL_EXT_FUNC(wglChoosePixelFormatARB);
    LOAD_WGL_EXT_FUNC(wglCreateContextAttribsARB);
    LOAD_WGL_EXT_FUNC(wglSwapIntervalEXT);

    if (!wglCreateContextAttribsARB || !wglCreateContextAttribsARB) {
        MessageBoxA(NULL, "Didn't get wgl ARB functions!", "FAILURE", MB_OK);
        return NULL;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummyGL);
    DestroyWindow(dummyWindow);

    /////////////////////////////////////////////
    // Create real window and rendering context
    /////////////////////////////////////////////

    HWND window = CreateWindow(
        WIN_CLASS_NAME,
        args->title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        args->width, args->height,
        NULL, 
        NULL,
        instance,
        NULL
    );

    if (!window) {
        MessageBoxA(NULL, "Failed to create window!", "FAILURE", MB_OK);

        return NULL;
    }

    HDC deviceContext = GetDC(window);

    const int pixelAttribList[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        // TODO(Tarek): Make these optional 
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        WGL_SAMPLES_ARB, 4,
        0
    };

    UINT numFormats;
    BOOL success;
    success = wglChoosePixelFormatARB(deviceContext, pixelAttribList, NULL, 1, &pixelFormat, &numFormats);

    if (!success || numFormats == 0) {
        MessageBoxA(NULL, "Didn't get ARB pixel format!", "FAILURE", MB_OK);
        return NULL;
    }
    
    DescribePixelFormat(deviceContext, pixelFormat, sizeof(pfd), &pfd);
    SetPixelFormat(deviceContext, pixelFormat, &pfd);

    const int contextAttribList[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, args->majorVersion,
        WGL_CONTEXT_MINOR_VERSION_ARB, args->minorVersion,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    HGLRC gl = wglCreateContextAttribsARB(deviceContext, NULL, contextAttribList);

    if (!gl) {
        MessageBoxA(NULL, "Didn't get ARB GL context!", "FAILURE", MB_OK);
        return NULL;
    }

    wglMakeCurrent(deviceContext, gl);

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1);
    }

    return window;
}

#endif // CREATE_OPENGL_WINDOW_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // CREATE_OPENGL_WINDOW_H