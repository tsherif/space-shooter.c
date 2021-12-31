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

////////////////////////////////////////////////////////////////////////////////////////
// Uses Win32 and XInput
// - https://docs.microsoft.com/en-us/windows/win32/
// - https://docs.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput
////////////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#define SOGL_IMPLEMENTATION_WIN32
#define CREATE_OPENGL_WINDOW_IMPLEMENTATION
#include "../../../lib/create-opengl-window.h"
#include "../../../lib/simple-opengl-loader.h"
#include <windows.h>
#include <windowsx.h>
#include <xinput.h>
#include <timeapi.h>
#include <stdbool.h>
#include <malloc.h>
#include <math.h>
#include <profileapi.h>
#include "../../shared/constants.h"
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "../../shared/debug.h"
#include "windows-audio.h"

static struct {
    int32_t width;
    int32_t height;
    int32_t preFullscreenWidth;
    int32_t preFullscreenHeight;
    bool fullscreen;
    bool mouseHovering;
} windowState = {
    .preFullscreenWidth = SPACE_SHOOTER_DEFAULT_WINDOWED_WIDTH,
    .preFullscreenHeight = SPACE_SHOOTER_DEFAULT_WINDOWED_HEIGHT,
    .fullscreen = true,
    .mouseHovering = false
};

static struct {
    float stickX;
    float stickY;
    bool aButton;
    bool startButton;
    bool backButton;
    bool keyboard;
    bool fast_shoot;
} gamepad;

static struct {
    bool left;
    bool right;
    bool up;
    bool down;
} keyboardDirections;

static bool running = false;

static void toggleFullscreen(HWND window) {
    windowState.fullscreen = !windowState.fullscreen;
    int32_t  x = 100;
    int32_t  y = 100;
    int32_t  width = windowState.preFullscreenWidth;
    int32_t  height = windowState.preFullscreenHeight;
    uint32_t flags = SWP_NOCOPYBITS | SWP_FRAMECHANGED;
    DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    if (windowState.fullscreen) {
        windowState.preFullscreenWidth = windowState.width;
        windowState.preFullscreenHeight = windowState.height;
        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo = { .cbSize = sizeof(MONITORINFO) };
        GetMonitorInfo(monitor, &monitorInfo);
        x = monitorInfo.rcMonitor.left;
        y = monitorInfo.rcMonitor.top;
        width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        windowStyle = WS_POPUP | WS_VISIBLE;
    }

    SetWindowLong(window, GWL_STYLE, windowStyle);
    SetWindowPos(
        window,
        HWND_TOP,
        x,
        y,
        width,
        height,
        flags
    );
}

static void processXInputState(XINPUT_STATE* xInputState) {
    bool gamepadHasInput = false;
    float stickX = 0.0f;
    float stickY = 0.0f;
    bool aButton = false;
    bool startButton = false;
    bool backButton = false;

    float x = xInputState->Gamepad.sThumbLX;
    float y = xInputState->Gamepad.sThumbLY;

    float mag = (float) sqrt(x * x + y * y);
    x /= mag;
    y /= mag;

    if (mag > 32767.0f) {
        mag = 32767.0f;
    }

    if (mag > SPACE_SHOOTER_GAMEPAD_STICK_DEADZONE) {
        mag -= SPACE_SHOOTER_GAMEPAD_STICK_DEADZONE;
        mag /= 32767.0f - SPACE_SHOOTER_GAMEPAD_STICK_DEADZONE;
        stickX = x * mag;
        stickY = y * mag;
        gamepadHasInput = true;
    }
    
    aButton = (xInputState->Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    startButton = (xInputState->Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
    backButton = (xInputState->Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;

    if (aButton || startButton || backButton) {
        gamepadHasInput = true;
    }

    if (gamepadHasInput || !gamepad.keyboard) {
        gamepad.stickX = stickX;
        gamepad.stickY = stickY;
        gamepad.aButton = aButton;
        gamepad.startButton = startButton;
        gamepad.backButton = backButton;
        gamepad.keyboard = false;
    }
}

int64_t getElapsedTime(LARGE_INTEGER currentCount, LARGE_INTEGER lastCount, LARGE_INTEGER frequency) {
    int64_t elapsedCount = (currentCount.QuadPart - lastCount.QuadPart) * SPACE_SHOOTER_SECOND;
    return elapsedCount / frequency.QuadPart;
}

static LRESULT CALLBACK messageHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (!running) {
        return DefWindowProc(window, message, wParam, lParam);
    }

    switch (message) {
        case WM_SIZE: {
            RECT clientRect;
            GetClientRect(window, &clientRect);
            windowState.width = clientRect.right - clientRect.left;
            windowState.height = clientRect.bottom - clientRect.top;
            game_resize(windowState.width, windowState.height);
            return 0;
        };
        case WM_MOUSEMOVE: {
            if (!windowState.mouseHovering) {
                ShowCursor(false);
                TRACKMOUSEEVENT trackMouse = {
                    .cbSize = sizeof(TRACKMOUSEEVENT),
                    .dwFlags = TME_LEAVE,
                    .hwndTrack = window
                };
                TrackMouseEvent(&trackMouse);
                windowState.mouseHovering = true;
            }
            return 0;
        } break;
        case WM_MOUSELEAVE: {
            ShowCursor(true);
            windowState.mouseHovering = false;    
            return 0;
        } break;
        case WM_SIZING: {
            RECT clientRect;
            GetClientRect(window, &clientRect); 
            windowState.width = clientRect.right - clientRect.left;
            windowState.height = clientRect.bottom - clientRect.top;
            game_resize(windowState.width, windowState.height);
            return 0;
        } break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC deviceContext = BeginPaint(window, &ps);
            game_draw();
            SwapBuffers(deviceContext);
            EndPaint(window, &ps);
            return 0;
        } break;
        case WM_KEYDOWN:
        case WM_KEYUP: {
            bool down = (lParam & (1 << 31)) == 0;
            switch (wParam) {
                case VK_LEFT: keyboardDirections.left = down; break;
                case VK_RIGHT: keyboardDirections.right = down; break;
                case VK_DOWN: keyboardDirections.down = down; break;
                case VK_UP: keyboardDirections.up = down; break; 
                case VK_SPACE: gamepad.aButton = down; break;
                case VK_ESCAPE: gamepad.backButton = down; break;
                case 'F': gamepad.startButton = down; break;
                case 'A': gamepad.fast_shoot = down; break;
            }

            if (keyboardDirections.left) {
                gamepad.stickX = -1.0f;
            } else if (keyboardDirections.right) {
                gamepad.stickX = 1.0f;
            } else {
                gamepad.stickX = 0.0f;
            }

            if (keyboardDirections.down) {
                gamepad.stickY = -1.0f;
            } else if (keyboardDirections.up) {
                gamepad.stickY = 1.0f;
            } else {
                gamepad.stickY = 0.0f;
            }

            gamepad.keyboard = true;
            return 0;
        } break;
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        } break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

int32_t WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int32_t showWindow) {
    int32_t exitStatus = 1;

    DWORD fileAttributes = GetFileAttributesA("./assets");
    if (fileAttributes == INVALID_FILE_ATTRIBUTES || !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        platform_userMessage("Asset directory not found.\nDid you move the game executable without moving the assets?");
        goto EXIT_NO_RESOURCES;
    };

    HWND window = createOpenGLWindow( &(CreateOpenGLWindowArgs) {
        .title = "space-shooter.c (Windows)", 
        .majorVersion = SOGL_MAJOR_VERSION, 
        .minorVersion = SOGL_MINOR_VERSION,
        .winCallback = messageHandler,
        .msaaSamples = SPACE_SHOOTER_MSAA_SAMPLES,
        .vsync = true,
        .errorLogFunction = platform_userMessage
    });

    if (!window) {
        platform_userMessage("Unable to create window.");
        goto EXIT_NO_RESOURCES;
    }

    if (!sogl_loadOpenGL()) {
#ifdef SPACE_SHOOTER_DEBUG
        DEBUG_LOG("The following OpenGL functions could not be loaded:");
        const char **failures = sogl_getFailures();
        while (*failures) {
            DEBUG_LOG(*failures);
            failures++;
        }
#endif        
        platform_userMessage("Failed to load OpenGL functions.");
        goto EXIT_WINDOW;
    }

    XINPUT_STATE xInputState;
    int32_t gamepadIndex = -1;

    for (int32_t i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (XInputGetState(i, &xInputState) == ERROR_SUCCESS) {
            gamepadIndex = i;
            break;
        }
    }

    if (!windows_initAudio()) {
        platform_userMessage("Unable to initialize audio.");
    }


    if (!game_init()) {
        goto EXIT_GAME;
    }

    RECT clientRect;
    GetClientRect(window, &clientRect); 
    windowState.width = clientRect.right - clientRect.left;
    windowState.height = clientRect.bottom - clientRect.top;
    game_resize(windowState.width, windowState.height);

    ///////////////////
    // Display window
    ///////////////////

    ShowWindow(window, showWindow);
    HDC deviceContext = GetDC(window);

    //////////////////////////////////
    // Start render and message loop
    //////////////////////////////////

    struct {
        bool toggleFullscreen;
        bool quit;
        bool lastToggleFullscreen;
        bool lastQuit;
    } systemInput = { 0 };
    MSG message = { 0 };
    LARGE_INTEGER lastPerfCount, tickFrequency;
    int64_t gamePadPollTime = 0;
    QueryPerformanceFrequency(&tickFrequency);
    QueryPerformanceCounter(&lastPerfCount);
    
    // Use 1ms schedular frequency for sleeps if available.
    bool useSleep = timeBeginPeriod(1) == TIMERR_NOERROR;
    running = true;

    while (running) {
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);

            if (message.message == WM_QUIT) {
                exitStatus = (int32_t) message.wParam;
                running = false; 
                break;
            }
        }

        if (gamepadIndex > -1 && XInputGetState(gamepadIndex, &xInputState) == ERROR_SUCCESS) {
            processXInputState(&xInputState);
        } else {
            gamepadIndex = -1;
            gamepad.keyboard = true;
        }

        systemInput.toggleFullscreen = gamepad.startButton;
        if (systemInput.toggleFullscreen && !systemInput.lastToggleFullscreen) {
            toggleFullscreen(window);
        }
        systemInput.lastToggleFullscreen = systemInput.toggleFullscreen;

        systemInput.quit = gamepad.backButton;
        if (systemInput.quit && !systemInput.lastQuit) {
            exitStatus = 0;
            running = false;
        }
        systemInput.lastQuit = systemInput.quit;

        LARGE_INTEGER perfCount;
        QueryPerformanceCounter(&perfCount);
        int64_t elapsedTime = getElapsedTime(perfCount, lastPerfCount, tickFrequency);

        if (useSleep && SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime > SPACE_SHOOTER_MILLISECOND) {
            DWORD sleepMs = (DWORD) ((SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime) / SPACE_SHOOTER_MILLISECOND);
            Sleep(sleepMs);

            QueryPerformanceCounter(&perfCount);
            elapsedTime = getElapsedTime(perfCount, lastPerfCount, tickFrequency);
        }


        if (gamepadIndex == -1) {
            gamePadPollTime += elapsedTime;
            if (gamePadPollTime > SPACE_SHOOTER_GAMEPAD_POLL_TIME) {
                for (int32_t i = 0; i < XUSER_MAX_COUNT; ++i) {
                    if (XInputGetState(i, &xInputState) == ERROR_SUCCESS) {
                        gamepadIndex = i;
                        break;
                    }
                }
                gamePadPollTime = 0;
            }
        }

        game_update((float) elapsedTime / SPACE_SHOOTER_MILLISECOND);
        game_draw();
        SwapBuffers(deviceContext);    
        
        lastPerfCount = perfCount;
    }

    if (useSleep) {
        timeEndPeriod(1);
    }

    EXIT_GAME:
    windows_closeAudio();
    game_close();  // NOTE(Tarek): After closeAudio so audio buffers don't get freed while playing.

    EXIT_WINDOW:
    destroyOpenGLWindow(window);

    EXIT_NO_RESOURCES:
    return exitStatus;
}

void platform_getInput(Game_Input* input) {
    input->lastShoot = input->shoot;
    input->velocity[0] = gamepad.stickX;
    input->velocity[1] = gamepad.stickY;
    input->shoot = gamepad.aButton;
    input->fast_shoot = gamepad.fast_shoot;
    input->keyboard = gamepad.keyboard;
}

void platform_debugMessage(const char* message) {
    OutputDebugStringA(message);  
    OutputDebugStringA("\n");  
}

void platform_userMessage(const char* message) {
    MessageBoxA(NULL, message, "space-shooter.c (Windows)", MB_OK);
}

bool platform_loadFile(const char* fileName, Data_Buffer* buffer, bool nullTerminate) {
    HANDLE file = CreateFileA(
      fileName,
      GENERIC_READ,
      FILE_SHARE_READ,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL
    );

    uint8_t* data = 0;
    
    if (file == INVALID_HANDLE_VALUE) {        
        DEBUG_LOG("platform_loadFile: Unable to open file.");
        goto ERROR_NO_RESOURCES;
    }

    LARGE_INTEGER fileSize = { 0 }; 
    if (!GetFileSizeEx(file, &fileSize)) {
        DEBUG_LOG("platform_loadFile: Unable to get file size.");
        goto ERROR_FILE_OPENED;
    }

    DEBUG_ASSERT(fileSize.HighPart == 0, "platform_loadFile: File too large!");

    int32_t allocation = fileSize.LowPart;

    if (nullTerminate) {
        allocation += 1;
    }

    DWORD bytesRead = 0;
    data = (uint8_t*) malloc(allocation);

    if (!data) {
        DEBUG_LOG("platform_loadFile: Unable to allocate data.");
        goto ERROR_FILE_OPENED;
    }

    if (!ReadFile(file, data, fileSize.LowPart, &bytesRead, NULL)) {
        DEBUG_LOG("platform_loadFile: Unable to read data.");
        goto ERROR_DATA_ALLOCATED;
    }

    if (nullTerminate) {
        data[allocation - 1] = 0;
    }

    buffer->data = data;
    buffer->size = allocation;
    CloseHandle(file);

    /////////////
    // Success!
    /////////////

    return true;

    ///////////////////
    // Error handling
    ///////////////////

    ERROR_DATA_ALLOCATED:
    free(data);

    ERROR_FILE_OPENED:
    CloseHandle(file);
    
    ERROR_NO_RESOURCES:
    return false;
}

