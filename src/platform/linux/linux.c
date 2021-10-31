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

#define SOGL_IMPLEMENTATION_X11
#include "../../../lib/simple-opengl-loader.h"
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "linux-audio.h"

#define INITIAL_WINDOW_WIDTH 1200
#define INITIAL_WINDOW_HEIGHT 600

static struct {
    bool left;
    bool right;
    bool up;
    bool down;
    bool space;
    bool lastSpace;
    bool ctrl;
} keyboard;

typedef GLXContext (*glXCreateContextAttribsARBFUNC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void (*glXSwapIntervalEXTFUNC)(Display*, GLXDrawable, int);

int main(int argc, char const *argv[]) {
    Display* display;
    Window window;
    XEvent event;
    XWindowAttributes xWinAtt;

    // X Windows stuff
    display = XOpenDisplay(NULL);

    if (display == NULL) {
        printf("Unable to connect to X Server\n");
        return 1;
    }

    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 20, 20, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, 0, 0, 0);
    

    XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask);
    XStoreName(display, window, "space-shooter.c (x11)");
    XMapWindow(display, window);
    
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
        fprintf(stderr, "Unable to get framebuffer\n");
        return -1;        
    }

    glXCreateContextAttribsARBFUNC glXCreateContextAttribsARB = (glXCreateContextAttribsARBFUNC) glXGetProcAddress((const GLubyte *) "glXCreateContextAttribsARB");
    glXSwapIntervalEXTFUNC glXSwapIntervalEXT = (glXSwapIntervalEXTFUNC) glXGetProcAddress((const GLubyte *) "glXSwapIntervalEXT");

    if (!glXCreateContextAttribsARB) {
        fprintf(stderr, "Unable to get proc glXCreateContextAttribsARB\n");
        XFree(fbc);
        return -1; 
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
        fprintf(stderr, "Unable to create OpenGL context\n");
        return -1;
    }

    glXMakeCurrent(display, window, ctx);

    if (glXSwapIntervalEXT) {
        glXSwapIntervalEXT(display, window, 1);
    }

    if (!sogl_loadOpenGL()) {
        const char **failures = sogl_getFailures();
        while (*failures) {
            fprintf(stderr, "space-shooter.c x11: Failed to load function %s\n", *failures);
            failures++;
        }
    }

    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    if (!linux_initAudio()) {
        return 1;
    }

    game_init();
    game_resize(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);

    uint64_t lastTime;
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    lastTime = timeSpec.tv_sec * 1000000000ll + timeSpec.tv_nsec;

    bool running = true;

    // Animation loop
    while (running) {
        while (XCheckTypedWindowEvent(display, window, ClientMessage, &event) == True) {
            if (event.xclient.data.l[0] == wmDeleteMessage) {
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
                case XK_Up: keyboard.up = down; break;
                case XK_Down: keyboard.down = down; break;
                case XK_Left: keyboard.left = down; break;
                case XK_Right: keyboard.right = down; break;
                case XK_space: keyboard.space = down; break;
                case XK_Control_L: keyboard.ctrl = down; break;
                case XK_Control_R: keyboard.ctrl = down; break;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &timeSpec);
        uint64_t time = timeSpec.tv_sec * 1000000000ll + timeSpec.tv_nsec;

        uint64_t elapsedTime = time - lastTime;

        game_update(elapsedTime / 1000000.0f);
        game_draw();

        glXSwapBuffers(display, window);

        lastTime = time;
    };

    // Teardown
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}




void platform_getInput(GameInput* input) {
    input->velocity[0] = 0.0f;
    input->velocity[1] = 0.0f;
    input->shoot = false;

    if (keyboard.left) {
        input->velocity[0] = -1.0f;
    } else if (keyboard.right) {
        input->velocity[0] = 1.0f;
    }

    if (keyboard.up) {
        input->velocity[1] = 1.0f;
    } else if (keyboard.down) {
        input->velocity[1] = -1.0f;
    }

    if (keyboard.space && !keyboard.lastSpace) {
        input->shoot = true;
    }
    keyboard.lastSpace = keyboard.space;
}

void platform_debugLog(const char* message) {
    int32_t length = 0;
    while(message[length]) {
        ++length;
    }

    write(STDERR_FILENO, message, length);
    write(STDERR_FILENO, "\n", 1);
}

bool platform_loadBinFile(const char* fileName, DataBuffer* buffer) {
    int32_t fd = open(fileName, O_RDONLY);

    if (fd == -1) {
        platform_debugLog("platform_loadBinFile: Unable to open file.");
        return false;
    }

    int32_t size = lseek(fd, 0, SEEK_END);

    if (size == -1) {
        platform_debugLog("platform_loadBinFile: Unable to get file size.");
        return false;  
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        platform_debugLog("platform_loadBinFile: Unable to reset file cursor.");
        return false;  
    }

    uint8_t* data = (uint8_t*) malloc(size);

    if (!data) {
        platform_debugLog("platform_loadBinFile: Unable to allocate data.");
        return false;  
    }

    if (read(fd, data, size) == -1) {
        platform_debugLog("platform_loadBinFile: Unable to read data.");
        free(data);
        return false; 
    }

    buffer->data = data;
    buffer->size = size;

    return true;
}
