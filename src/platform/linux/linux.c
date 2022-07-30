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

///////////////////////////////////////////////////////////////////
// Uses XLib and EWMH
// - https://tronche.com/gui/x/xlib/
// - https://specifications.freedesktop.org/wm-spec/1.3/index.html
///////////////////////////////////////////////////////////////////

#define SOGL_IMPLEMENTATION_X11
#include "../../../lib/simple-opengl-loader.h"
#include <stdlib.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <time.h>
#include <sys/stat.h>
#include "../../shared/constants.h"
#include "../../shared/platform-interface.h"
#include "../../shared/debug.h"
#include "linux-audio.h"
#include "linux-gamepad.h"

#define NET_WM_STATE_REMOVE 0
#define NET_WM_STATE_ADD    1

static Linux_Gamepad gamepad;

typedef GLXContext (*glXCreateContextAttribsARBFUNC)(Display* display, GLXFBConfig framebufferConfig, GLXContext shareContext, Bool direct, const int32_t* contextAttribs);
typedef void (*glXSwapIntervalEXTFUNC)(Display* display, GLXDrawable window, int32_t interval);

int xErrorHandler(Display* display, XErrorEvent* event) {
    platform_userMessage("An Xlib error occurred.");
    return 0;
}

int64_t nsFromTimeSpec(struct timespec timeSpec) {
    return timeSpec.tv_sec * SPACE_SHOOTER_SECOND + timeSpec.tv_nsec;
}

int32_t main(int32_t argc, char const *argv[]) {
    int32_t exitStatus = 1;

    struct stat assetsStat = { 0 };
    int statResult = stat("./assets", &assetsStat);
    if (statResult == -1 || !S_ISDIR(assetsStat.st_mode)) {
        platform_userMessage("Asset directory not found.\nDid you move the game executable without moving the assets?");
        goto EXIT_NO_RESOURCES;
    }

    XSetErrorHandler(xErrorHandler); 

    /////////////////////////
    // Connect to X server
    /////////////////////////

    Display* display = XOpenDisplay(NULL);

    if (display == NULL) {
        DEBUG_LOG("Failed to open X connection.");
        platform_userMessage("Failed to create window.");
        goto EXIT_ERROR_HANDLER;
    }


    //////////////////////////////////////////////////////
    // Find framebuffer configuration that satisfies
    // OpenGL requirements. Have to do this here,
    // so we can create window with that configuration.
    //////////////////////////////////////////////////////

    int32_t fbcCount = 0;
    GLXFBConfig *fbcList = glXChooseFBConfig(display, DefaultScreen(display), (int32_t[]) {
        GLX_RENDER_TYPE, GLX_RGBA_BIT, 
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, 
        GLX_DOUBLEBUFFER, True, 
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        None
    }, &fbcCount);

    if (!fbcList) {
        DEBUG_LOG("No framebuffer config found.");
        platform_userMessage("Failed to load OpenGL.");
        goto EXIT_DISPLAY;
    }

    // Find fbc with most samples but at most 4
    int32_t fbcIndex = 0;
    int32_t bestSamples = 0;
    int32_t samples = 0;
    glXGetFBConfigAttrib(display, fbcList[0], GLX_SAMPLES, &samples);

    for (int32_t i = 1; i < fbcCount; ++i) {
        glXGetFBConfigAttrib(display, fbcList[i], GLX_SAMPLES, &samples);

        if (samples <= SPACE_SHOOTER_MSAA_SAMPLES && samples > bestSamples) {
            bestSamples = samples;
            fbcIndex = i;
        }
    }

    GLXFBConfig framebufferConfig = fbcList[fbcIndex];
    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, framebufferConfig);
    XFree(fbcList);

    if (!visualInfo) {
        DEBUG_LOG("No visual info found.");
        platform_userMessage("Failed to load OpenGL.");
        goto EXIT_DISPLAY;
    }

    ///////////////////
    // Create window
    ///////////////////

    // NOTE(Tarek): border pixel is required in case depth doesn't match parent depth.
    // See: https://tronche.com/gui/x/xlib/window/attributes/border.html

    Window rootWindow = XRootWindow(display, visualInfo->screen);
    Colormap colorMap = XCreateColormap(display, rootWindow, visualInfo->visual, AllocNone);
    Window window = XCreateWindow(
        display,
        rootWindow,
        20, 20, 
        SPACE_SHOOTER_DEFAULT_WINDOWED_WIDTH, SPACE_SHOOTER_DEFAULT_WINDOWED_HEIGHT,
        0,
        visualInfo->depth,
        InputOutput,
        visualInfo->visual,
        CWColormap | CWEventMask | CWBorderPixel,
        &(XSetWindowAttributes) {
            .colormap = colorMap,
            .event_mask = ExposureMask | KeyPressMask | KeyReleaseMask,
            .border_pixel = 0
        }
    );

    XStoreName(display, window, "space-shooter.c (Linux)");
    XFree(visualInfo);
    
    /////////////////
    // Hide cursor
    /////////////////

    char hiddenCursorData = 0;
    XColor hiddenCursorColor = { 0 };
    Pixmap hiddenCursorPixmap = XCreatePixmapFromBitmapData(display, window, &hiddenCursorData, 1, 1, 1, 0, 1);
    Cursor hiddenCursor = XCreatePixmapCursor(display, hiddenCursorPixmap, hiddenCursorPixmap, &hiddenCursorColor, &hiddenCursorColor, 0, 0);
    XDefineCursor(display, window, hiddenCursor);
    XFreeCursor(display, hiddenCursor);
    XFreePixmap(display, hiddenCursorPixmap);
    
    ///////////////////////////
    // Create OpenGL context
    ///////////////////////////

    glXCreateContextAttribsARBFUNC glXCreateContextAttribsARB = (glXCreateContextAttribsARBFUNC) glXGetProcAddress((const GLubyte *) "glXCreateContextAttribsARB");
    glXSwapIntervalEXTFUNC glXSwapIntervalEXT = (glXSwapIntervalEXTFUNC) glXGetProcAddress((const GLubyte *) "glXSwapIntervalEXT");

    if (!glXCreateContextAttribsARB) {
        DEBUG_LOG("Failed to load GLX extension functions.");
        platform_userMessage("Failed to load OpenGL.");
        goto EXIT_WINDOW; 
    }

    GLXContext gl = glXCreateContextAttribsARB(display, framebufferConfig, NULL, True, (int32_t []) {
        GLX_CONTEXT_MAJOR_VERSION_ARB, SOGL_MAJOR_VERSION,
        GLX_CONTEXT_MINOR_VERSION_ARB, SOGL_MINOR_VERSION,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    });

    if (!gl) {
        DEBUG_LOG("Unable create OpenGL context.");
        platform_userMessage("Failed to load OpenGL.");
        goto EXIT_WINDOW;
    }

    glXMakeCurrent(display, window, gl);

    if (glXSwapIntervalEXT) {
        glXSwapIntervalEXT(display, window, 1);
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
        goto EXIT_GL;
    }

    /////////////////////////////////
    // Set up window manager events
    /////////////////////////////////

    Atom NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
    Atom NET_WM_STATE_FULLSCREEN = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);

    XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

    XEvent fullscreenEvent = {
        .xclient = {
            .type = ClientMessage,
            .window = window,
            .message_type = NET_WM_STATE,
            .format = 32,
            .data = {
                .l = {
                    NET_WM_STATE_ADD,
                    NET_WM_STATE_FULLSCREEN,
                    0,
                    1,
                    0
                }
            }
        }
    };

    XEvent windowedEvent = {
        .xclient = {
            .type = ClientMessage,
            .window = window,
            .message_type = NET_WM_STATE,
            .format = 32,
            .data = {
                .l = {
                    NET_WM_STATE_REMOVE,
                    NET_WM_STATE_FULLSCREEN,
                    0,
                    1,
                    0
                }
            }
        }
    };

    // Start in fullscreen
    XSendEvent(display, rootWindow, False, SubstructureNotifyMask | SubstructureRedirectMask, &fullscreenEvent);

    ////////////////////
    // Display window
    ////////////////////

    XMapWindow(display, window);


    /////////////////////
    // Initialize audio
    /////////////////////

    if (!linux_initAudio()) {
        platform_userMessage("Failed to initialize audio.");
    }

    /////////////////////
    // Start game
    /////////////////////

    linux_detectGamepad(); 

    if (!game_init(NULL)) {
        goto EXIT_GAME;
    }

    struct {
        bool left;
        bool right;
        bool up;
        bool down;
    } keyboardDirections = { 0 };
    struct {
        bool toggleFullscreen;
        bool quit;
        bool lastToggleFullscreen;
        bool lastQuit;
    } systemInput = { 0 };
    XEvent event = { 0 };
    XWindowAttributes xWinAtt = { 0 };
    
    struct timespec timeSpec = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    int64_t lastTime = nsFromTimeSpec(timeSpec);
    int64_t gamePadPollTime = 0;
    
    bool fullscreen = true;
    bool running = true;

    while (running) {
        while (XCheckTypedWindowEvent(display, window, ClientMessage, &event) == True) {
            if (event.xclient.data.l[0] == WM_DELETE_WINDOW) {
                running = false;
            }
        }

        while (XCheckTypedWindowEvent(display, window, Expose, &event) == True) {
            XGetWindowAttributes(display, window, &xWinAtt);
            game_resize(xWinAtt.width, xWinAtt.height);
        }

        while (XCheckWindowEvent(display, window, KeyPressMask | KeyReleaseMask, &event) == True) {
            bool down = event.type == KeyPress;

            KeySym key = XLookupKeysym(&event.xkey, 0);

            switch (key) {
                case XK_Left: keyboardDirections.left = down; break;
                case XK_Right: keyboardDirections.right = down; break;
                case XK_Down: keyboardDirections.down = down; break;
                case XK_Up: keyboardDirections.up = down; break;
                case XK_space: gamepad.aButton = down; break;
                case XK_Escape: gamepad.backButton = down; break;
                case XK_f: gamepad.startButton = down; break;
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
        }

        linux_updateGamepad(&gamepad);
        
        systemInput.toggleFullscreen = gamepad.startButton;
        if (systemInput.toggleFullscreen && !systemInput.lastToggleFullscreen) {
            if (fullscreen) {
                XSendEvent(display, rootWindow, False, SubstructureNotifyMask | SubstructureRedirectMask, &windowedEvent);
            } else {
                XSendEvent(display, rootWindow, False, SubstructureNotifyMask | SubstructureRedirectMask, &fullscreenEvent);
            }
            fullscreen = !fullscreen;
        }
        systemInput.lastToggleFullscreen = systemInput.toggleFullscreen;

        systemInput.quit = gamepad.backButton;
        if (systemInput.quit && !systemInput.lastQuit) {
            running = false;
        }
        systemInput.lastQuit = systemInput.quit;

        clock_gettime(CLOCK_MONOTONIC, &timeSpec);
        int64_t time = nsFromTimeSpec(timeSpec);
        int64_t elapsedTime = time - lastTime;

        // Sleep if at least 1ms less than frame min
        if (SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime > SPACE_SHOOTER_MILLISECOND) {
            struct timespec sleepTime = {
                .tv_nsec = SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime
            };
            nanosleep(&sleepTime, NULL);

            clock_gettime(CLOCK_MONOTONIC, &timeSpec);
            time = nsFromTimeSpec(timeSpec);
            elapsedTime = time - lastTime;
        }

        gamePadPollTime += elapsedTime;
        if (gamePadPollTime > SPACE_SHOOTER_GAMEPAD_POLL_TIME) {
            linux_detectGamepad();
            gamePadPollTime = 0;
        }

        game_update((float) elapsedTime / SPACE_SHOOTER_MILLISECOND);
        game_draw();

        glXSwapBuffers(display, window);

        lastTime = time;
    };

    EXIT_GAME:
    linux_closeGamepad();
    linux_closeAudio();
    game_close(); // NOTE(Tarek): After closeAudio so audio buffers don't get freed while playing.

    EXIT_GL:
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, gl);

    EXIT_WINDOW:
    XDestroyWindow(display, window);
    XFreeColormap(display, colorMap);

    EXIT_DISPLAY:
    XCloseDisplay(display);

    EXIT_ERROR_HANDLER:
    XSetErrorHandler(NULL);
    
    EXIT_NO_RESOURCES:
    return exitStatus;
}

void platform_getInput(Game_Input* input) {
    input->lastShoot = input->shoot;
    input->velocity[0] = gamepad.stickX;
    input->velocity[1] = gamepad.stickY;
    input->shoot = gamepad.aButton;
    input->keyboard = gamepad.keyboard;
}