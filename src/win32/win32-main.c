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

#define WIN32_LEAN_AND_MEAN
#define SOGL_IMPLEMENTATION_WIN32
#define CREATE_OPENGL_WINDOW_IMPLEMENTATION
#include <windows.h>
#include <windowsx.h>
#include <xinput.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <profileapi.h>
#include "../../lib/create-opengl-window.h"
#include "../../lib/simple-opengl-loader.h"
#include "../shared/platform-interface.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

GameKeyboard inputKeys;
GameController controllerInput;

bool gamepadEquals(XINPUT_GAMEPAD* gp1, XINPUT_GAMEPAD* gp2) {
    return gp1->wButtons == gp2->wButtons &&
        gp1->bLeftTrigger == gp2->bLeftTrigger &&
        gp1->bRightTrigger == gp2->bRightTrigger &&
        gp1->sThumbLX == gp2->sThumbLX &&
        gp1->sThumbLY == gp2->sThumbLY &&
        gp1->sThumbRX == gp2->sThumbRX &&
        gp1->sThumbRY == gp2->sThumbRY;
}

LRESULT CALLBACK winProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_SIZE: {
            RECT clientRect;
            GetClientRect(window, &clientRect); 
            game_resize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
            return 0;
        };
        case WM_SIZING: {
            RECT clientRect;
            GetClientRect(window, &clientRect); 
            game_resize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
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
        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_LEFT:    inputKeys.left  = true; break;
                case VK_RIGHT:   inputKeys.right = true; break;
                case VK_UP:      inputKeys.up    = true; break;
                case VK_DOWN:    inputKeys.down  = true; break;
                case VK_SPACE:   inputKeys.space = true; break;
                case VK_CONTROL: inputKeys.ctrl  = true; break;
            }
            inputKeys.changed = true;
            return 0;
        } break;
        case WM_KEYUP: {
            switch (wParam) {
                case VK_LEFT:    inputKeys.left  = false; break;
                case VK_RIGHT:   inputKeys.right = false; break;
                case VK_UP:      inputKeys.up    = false; break;
                case VK_DOWN:    inputKeys.down  = false; break;
                case VK_SPACE:   inputKeys.space = false; break;
                case VK_CONTROL: inputKeys.ctrl  = false; break;
            }
            inputKeys.changed = true;
            return 0;
        } break;
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        } break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showWindow) {
    HWND window = createOpenGLWindow( &(CreateOpenGLWindowArgs) {
        .title = "Win32 Shooter", 
        .majorVersion = SOGL_MAJOR_VERSION, 
        .minorVersion = SOGL_MINOR_VERSION,
        .winCallback = winProc,
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT
    });
    
    if (!sogl_loadOpenGL()) {
        const char **failures = sogl_getFailures();
        while (*failures) {
            char debugMessage[256];
            snprintf(debugMessage, 256, "SOGL: Failed to load function %s\n", *failures);
            OutputDebugStringA(debugMessage);
            failures++;
        }
    }

    XINPUT_STATE controllerState;
    XINPUT_GAMEPAD lastGamePadState;

    int controllerIndex = -1;

    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (XInputGetState(i, &controllerState) == ERROR_SUCCESS) {
            controllerIndex = i;
            break;
        }
    }

    if (!platform_initAudio()) {
        return 1;
    }

    RECT clientRect;
    GetClientRect(window, &clientRect); 
    game_resize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    game_init();
    
    ///////////////////
    // Display window
    ///////////////////

    ShowWindow(window, showWindow);
    HDC deviceContext = GetDC(window);

    //////////////////////////////////
    // Start render and message loop
    //////////////////////////////////

    MSG message;
    LARGE_INTEGER startTicks, endTicks, elapsedTime, tickFrequency;
    QueryPerformanceFrequency(&tickFrequency);
    uint32_t ticks = 0;
    float frameTime = 0.0f;
    float totalTime = 0.0f;
    float averageTime = 0.0f;
    float minTime = 10000.0f;
    float maxTime = -10000.0f;
    bool running = true;
    while (running) {
        QueryPerformanceCounter(&startTicks);
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);

            if (message.message == WM_QUIT) {
                running = false; 
                break;
            }
        }

        if (ticks % 200 == 0 && controllerIndex == -1) {
            for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
                if (XInputGetState(i, &controllerState) == ERROR_SUCCESS) {
                    controllerIndex = i;
                    break;
                }
            }
        } else if (controllerIndex > -1) {
            if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS) {
                if (!gamepadEquals(&controllerState.Gamepad, &lastGamePadState)) {
                    float x = controllerState.Gamepad.sThumbLX;
                    float y = controllerState.Gamepad.sThumbLY;

                    float mag = (float) sqrt(x * x + y * y);
                    x /= mag;
                    y /= mag;

                    if (mag > 32767.0f) {
                        mag = 32767.0f;
                    }

                    if (mag > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
                        mag -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
                        mag /= 32767.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
                        controllerInput.leftStickX = x * mag;
                        controllerInput.leftStickY = y * mag;
                    } else {
                        controllerInput.leftStickX = 0.0f;
                        controllerInput.leftStickY = 0.0f;
                    }
                    controllerInput.aButton = controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_A;
                    game_controller(&controllerInput);
                    lastGamePadState = controllerState.Gamepad;
                }
            } else {
                controllerIndex = -1;
            }
        }

        if (inputKeys.changed) {
            game_keyboard(&inputKeys);
            inputKeys.changed = false;
        }

        game_update();
        game_draw();

        SwapBuffers(deviceContext);
        QueryPerformanceCounter(&endTicks);

        elapsedTime.QuadPart = (endTicks.QuadPart - startTicks.QuadPart) * 1000000;
        elapsedTime.QuadPart /= tickFrequency.QuadPart;

        frameTime = elapsedTime.QuadPart / 1000.0f;
        ++ticks;

        totalTime += frameTime;
        averageTime = totalTime / ticks;
        minTime = frameTime < minTime ? frameTime : minTime;
        maxTime = frameTime > maxTime ? frameTime : maxTime;

        if (ticks == 600) {
            char buffer[1024];
            snprintf(buffer, 1024, "Win32 Shooter: Frame Time Average: %.2fms, Min: %.2fms, Max: %.2fms", averageTime, minTime, maxTime);
            SetWindowTextA(window, buffer);
            totalTime = 0.0f;
            ticks = 0;
        }
    }

    return (int) message.wParam;
}
