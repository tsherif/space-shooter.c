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
#include "../../shared/constants.h"
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "../../shared/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "linux-audio.h"
#include "linux-gamepad.h"

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2

static LinuxGamepad gamepad;

typedef GLXContext (*glXCreateContextAttribsARBFUNC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void (*glXSwapIntervalEXTFUNC)(Display*, GLXDrawable, int);

int main(int argc, char const *argv[]) {

    ////////////////////////////////
    // Create window
    ////////////////////////////////

    Display* display = XOpenDisplay(NULL);

    if (display == NULL) {
        platform_userMessage("Unable to create window.");
        return 1;
    }

    XSetWindowAttributes windowAttributes = { 0 };
    windowAttributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask;

    int screen = XDefaultScreen(display);
    Window rootWindow = XRootWindow(display, screen);
    Window window = XCreateWindow(
        display,
        rootWindow,
        20, 20, 
        SPACE_SHOOTER_DEFAULT_WINDOWED_WIDTH, SPACE_SHOOTER_DEFAULT_WINDOWED_HEIGHT,
        0,
        CopyFromParent,
        CopyFromParent,
        CopyFromParent,
        CWEventMask,
        &windowAttributes
    );

    XStoreName(display, window, "space-shooter.c (Linux)");
    
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

    int numFBC = 0;
    GLint visualAtt[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT, 
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, 
        GLX_DOUBLEBUFFER, True, 
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DEPTH_SIZE, 1,
        GLX_STENCIL_SIZE, 1,
        GLX_SAMPLE_BUFFERS, 1,
        GLX_SAMPLES, 4,
        None
    };

    GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), visualAtt, &numFBC);

    if (!fbc) {
        platform_userMessage("Unable to load OpenGL.");
        return 1;        
    }

    glXCreateContextAttribsARBFUNC glXCreateContextAttribsARB = (glXCreateContextAttribsARBFUNC) glXGetProcAddress((const GLubyte *) "glXCreateContextAttribsARB");
    glXSwapIntervalEXTFUNC glXSwapIntervalEXT = (glXSwapIntervalEXTFUNC) glXGetProcAddress((const GLubyte *) "glXSwapIntervalEXT");

    if (!glXCreateContextAttribsARB) {
        platform_userMessage("Unable to load OpenGL.");
        XFree(fbc);
        return 1; 
    }

    static int contextAttribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, SOGL_MAJOR_VERSION,
        GLX_CONTEXT_MINOR_VERSION_ARB, SOGL_MINOR_VERSION,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    GLXContext ctx = glXCreateContextAttribsARB(display, *fbc, NULL, True, contextAttribs);

    XFree(fbc);

    if (!ctx) {
        platform_userMessage("Unable to load OpenGL.");
        return 1;
    }

    glXMakeCurrent(display, window, ctx);

    if (glXSwapIntervalEXT) {
        glXSwapIntervalEXT(display, window, 1);
    }

    if (!sogl_loadOpenGL()) {
        const char **failures = sogl_getFailures();
        while (*failures) {
            fprintf(stderr, "space-shooter.c Linux: Failed to load function %s\n", *failures);
            failures++;
        }
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
                    _NET_WM_STATE_ADD,
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
                    _NET_WM_STATE_REMOVE,
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
        platform_userMessage("Unable to initialize audio.");
    }

    /////////////////////
    // Start game
    /////////////////////

    if (!game_init()) {
        return 1;
    }

    game_resize(SPACE_SHOOTER_DEFAULT_WINDOWED_WIDTH, SPACE_SHOOTER_DEFAULT_WINDOWED_HEIGHT);
    linux_detectGamepad();

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
    uint64_t ticks = 0;
    uint64_t lastTime;
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    lastTime = timeSpec.tv_sec * 1000000000ll + timeSpec.tv_nsec;
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

        if (ticks % 200 == 0) {
            linux_pingGamepad();
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
        uint64_t time = timeSpec.tv_sec * 1000000000ll + timeSpec.tv_nsec;

        uint64_t elapsedTime = time - lastTime;

        game_update(elapsedTime / 1000000.0f);
        game_draw();

        glXSwapBuffers(display, window);

        lastTime = time;
    };

    /////////////////////
    // Cleanup
    /////////////////////

    linux_closeGamepad();
    linux_closeAudio();
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

void platform_getInput(GameInput* input) {
    input->lastShoot = input->shoot;
    input->velocity[0] = gamepad.stickX;
    input->velocity[1] = gamepad.stickY;
    input->shoot = gamepad.aButton;
    input->keyboard = gamepad.keyboard;
}

void platform_debugLog(const char* message) {
    int32_t length = 0;
    while(message[length]) {
        ++length;
    }

    write(STDERR_FILENO, message, length);
    write(STDERR_FILENO, "\n", 1);
}

void platform_userMessage(const char* message) {
    platform_debugLog(message);
}


bool platform_loadFile(const char* fileName, DataBuffer* buffer, bool nullTerminate) {
    int32_t fd = open(fileName, O_RDONLY);
    uint8_t* data = 0;

    if (fd == -1) {
        DEBUG_LOG("platform_loadFile: Unable to open file.");
        goto ERROR_NO_RESOURCES;
    }

    int32_t size = lseek(fd, 0, SEEK_END);
    int32_t allocation = size;

    if (nullTerminate) {
        allocation += 1;
    }

    if (size == -1) {
        DEBUG_LOG("platform_loadFile: Unable to get file size.");
        goto ERROR_FILE_OPENED;
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        DEBUG_LOG("platform_loadFile: Unable to reset file cursor.");
        goto ERROR_FILE_OPENED;
    }

    data = (uint8_t*) malloc(allocation);

    if (!data) {
        DEBUG_LOG("platform_loadFile: Unable to allocate data.");
        goto ERROR_FILE_OPENED;
    }

    if (read(fd, data, size) == -1) {
        DEBUG_LOG("platform_loadFile: Unable to read data.");
        goto ERROR_DATA_ALLOCATED;
    }

    if (nullTerminate) {
        data[allocation - 1] = 0;
    }

    buffer->data = data;
    buffer->size = allocation;
    close(fd);

    return true;

    ERROR_DATA_ALLOCATED:
    free(data);
    
    ERROR_FILE_OPENED:
    close(fd);
    
    ERROR_NO_RESOURCES:
    return false;
}
