The Architecture of space-shooter.c
===================================

- [Introduction](#introduction)
- [Notes on Cross-platform Development](#notes-on-cross-platform-development)
- [Architectural Overview](#architectural-overview)
- [Data Model](#data-model)
- [The Platform Layer](#the-platform-layer)
- [The Game Layer](#the-game-layer)
- [The Rendering Layer](#the-rendering-layer)

Introduction
------------

In developing `space-shooter.c`, I iterated several times on how to organize the different parts and learned to use some more-or-less poorly-documented APIs on Windows, Linux and the Web. This document is intended as a record of that process and to hopefully serve as a reference for others doing similar work. I'll note that none of this is intended as a definitive description of how any of this **should** be done. I'm not a professional C programmer, nor a professional game programmer. I read about the APIs, wrote code and solved problems as they arose, and this is where I ended up. 

Throughout the text, I link to the references I used in building `space-shooter.c`, but I'll call out a few that were especially invaluable:
- [Handmade Hero](https://handmadehero.org/) is an incredibly generous resource on many levels, but I think its most important effect on me was simply demystifying low-level OS APIs.
- [pacman.c](https://github.com/floooh/pacman.c) is a goldmine of ideas for simplified game systems.
- The source code of [sokol](https://github.com/floooh/sokol), [GLFW](https://github.com/glfw/glfw) and [SDL](https://github.com/libsdl-org/SDL) were my encyclopedias for how get things done in the platform layer. This was especially helpful on Linux where functionality is spread across several APIs and documentation tends to be much worse.


### Notes on Cross-platform Development
---------------------------------------

Development on Windows and Linux is to a great extent similar. On Windows, I use [cl](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160) to compile, [Visual Studio 2019](https://visualstudio.microsoft.com/) to debug and simple batch files as the build system. On Linux, I use [gcc](https://gcc.gnu.org/) to compile, [gdb](https://sourceware.org/gdb/) to debug, [make](https://www.gnu.org/software/make/) to build. The development experience on Windows is generally smoother due to the thorough API documentation on [MSDN](https://docs.microsoft.com/en-us/windows/win32/desktop-programming) and a more pleasant debugging experience with Visual Studio, but I've never had to change any parts of the design of `space-shooter.c` to fit constraints on one platform or the other.

The Web, on the other hand, brings with it some challenges that weren't present on the native platforms. I use [emcc](https://emscripten.org/docs/tools_reference/emcc.html) to compile and [make](https://www.gnu.org/software/make/) again to build. I debug using Chrome DevTools as described in this [blog post](https://developer.chrome.com/blog/wasm-debugging-2020/), which takes a few steps to set up. The first is to install the [C/C++ DevTools Support Extension](https://chrome.google.com/webstore/detail/cc%20%20-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb) and to enable **WebAssembly Debugging: Enable DWARF support** in the DevTools settings. Additionally, to ensure the extension could find the files on my system, I had to set the compilation flag `-fdebug-compilation-dir=".."` (i.e. the root of the repository relative to the build directory) and remap the file paths as outlined in this [bug report](https://github.com/emscripten-core/emscripten/issues/13486#issuecomment-779117827). 

In addition to the debugging challenges, the Web involves constraints that required changes to the control flow of `space-shooter.c`, the most prominent of these being the requirement that certain operations only be performed in user input callbacks, namely playing audio and entering fullscreen mode. This challenge is further compounded by the fact that the Web Gamepad API uses a polling system rather than events and callbacks, meaning Gamepad input, as specced, isn't considered user input for the purpose of enabling the above-mentioned operations. Chrome implements [its own workaround](https://bugs.chromium.org/p/chromium/issues/detail?id=381596) for this problem, but since it's non-standard (and not supported [in Firefox](https://bugzilla.mozilla.org/show_bug.cgi?id=1740573)), I only support toggling fullscreen via the keyboard, even if a gamepad is connected. Furthermore, I implement the logic to toggle fullscreen directly in the keyboard input handler, rather than in the game loop as is done on the other platforms. Finally, to ensure audio is initialized correctly, I implement a start screen that only appears on the Web and asks the user for keyboard input to start the game so that it can do so within a keyboard input callback.


Architectural Overview
----------------------

At a high level, the architecture of `space-shooter.c` is composed of 3 layers:

- The **platform layer** is implemented in the [platform/windows](./src/platform/windows/), [platform/linux](./src/platform/linux/) and [platform/web](./src/platform/web/) directories and is responsible for:
    - Initializing a drawing surface (i.e. window or canvas).
    - Initializing OpenGL.
    - Playing audio.
    - Capturing user input.
    - File I/O.
    - Starting the game loop.

- The **game layer** is implemented in [game.c](./src/game/game.c) and is responsible for:
    - Initializing game resources.
    - Updating game state (player actions, collisions, score, etc.).
    - Passing rendering data to the rendering layer based on the current game state.

- The **rendering layer** is implemented in [renderer.c](./src/game/renderer.c) and is responsible for:
    - Managing OpenGL state and drawing to the screen.

The platform layer interacts with the game and rendering layers using an API inspired by [Handmade Hero](https://handmadehero.org/) and defined in [platform-interface.h](./src/shared/platform-interface.h). The platform layer implements the following functions used by the game and rendering layers:
- `platform_getInput(Game_Input* input)`: Get current input state.
- `platform_loadSound(const char* fileName)`: Load a wave file into the audio system and return an id to reference it.
- `platform_playSound(int32_t id, bool loop)`: Output sound to an audio device.
- `platform_debugMessage(const char* message)`: Output a message intended for the developer while debugging.
- `platform_userMessage(const char* message)`: Output a message intended for the end user.
- `platform_loadFile(const char* fileName, Data_Buffer* buffer, bool nullTerminate)`: Load contents of a file into memory. Optionally, null-terminate it if the data will be used as a string.

Once the platform layer initializes system resources, it calls into the game layer using the following life cycle functions:
- `game_init(Game_InitOptions* opts)`: Initialize game resources. Options allow customizations for specific platforms (e.g. don't immediately initialize audio on the Web).
- `game_initAudio()`: Initialize audio, if not done in `game_init` (e.g. for the Web after a user interaction).
- `game_update(float elapsedTime)`: Update game state based on time elapsed since last frame.
- `game_draw()`: Draw current frame.
- `game_resize(int width, int height)`: Update rendering state to match the current window size.
- `game_close()`: Release game resources.

The rendering layer implements the following functions used by the game layer to draw or update state related to drawing: 
- `renderer_init(int width, int height)`: Initialize OpenGL resources.
- `renderer_createTexture(uint8_t* data, int32_t width, int32_t height)`: Create a texture with the provided data.
- `renderer_validate()`: Check that the OpenGL context isn't out of memory.
- `renderer_resize(int width, int height)`: Resize the drawing surface.
- `renderer_beforeFrame()`: Prepare for drawing (primarily to fix aspect ratio and draw borders if necessary).
- `renderer_draw(Renderer_List* list)`: Draw to the screen.

Data Model
----------

### Loading Assets

Image assets for `space-shooter.c` are stored as [BMP files](https://en.wikipedia.org/wiki/BMP_file_format). They are loaded and parsed by the function `utils_loadBmpData` ([utils.c](./src/shared/utils.c)), which is called in `game_init`. To minimize the complexity of the parser, I impose a requirement that image data must be 32bpp, uncompressed BGRA data (the format exported by [GIMP](https://www.gimp.org/)).

Audio assets are stored as [WAVE files](http://soundfile.sapp.org/doc/WaveFormat/). They are loaded and parsed by the function `utils_loadWavData` ([utils.c](./src/shared/utils.c)), which is called in the platform audio layers. To minimize the complexity of the parser, I impose a requirement that audio data must be 44.1kHz, 16-bit stereo data, and the chunks must be in the order `RIFF`, `fmt` then `data`. This is the chunk order I found in all the assets I use (but it isn't imposed by the WAVE format), and I used [Audacity](https://www.audacityteam.org/) to fix the sample rate and number of channels where necessary.

Failure to load image data will cause the game to abort. Failure to load audio data will allow the game to run without the missing sounds. In debug builds, invalid data will cause the game to abort.

### Memory Management

Almost all memory allocations in `space-shooter.c` are static, with dynamic allocations only used to load image and sound assets when the game initializes. This leads to a nice "programmer peace of mind" benefit that once the game initializes, I no longer have to worry about errors related to allocating or freeing memory.

I use object pools to manage objects that can exist in variable numbers in the game, such as game entities or sounds in the mixer. Conceptually, an object pool can be thought of as a static array of objects and a `count` that tracks how many of them are currently active.

```c
// NOTE: This is not the actual implementation!
typedef struct {
    int32_t x;
    int32_t y;
} Object;

Object objects[MAX_OBJECTS];
int32_t count;
```

"Allocating" an object involves initializing `objects[count]` and then incrementing `count`.
```c
// NOTE: This is not the actual implementation!
int32_t newIndex = count;
objects[newIndex].x = 1;
objects[newIndex].y = 2;
++count;
```

"Deleting" an object involves swapping the deleted object with `objects[count - 1]` and decrementing `count`.
```c
// NOTE: This is not the actual implementation!
int32_t lastIndex = count - 1;
objects[deletedIndex].x = objects[lastIndex].x;
objects[deletedIndex].y = objects[lastIndex].y;
--count;
```

All operations on objects are run in batches on `objects[0 .. count - 1]`.

Note that the implementation in most cases doesn't look exactly like the above depending on how the objects will be manipulated by a given system. For example, game entity properties are stored in [structs of arrays](https://en.wikipedia.org/wiki/AoS_and_SoA#Structure_of_arrays), rather than in per-object structs, to simplify submitting them to the GPU as attribute buffers.

### Error Handling

My primary concern in managing errors in `space-shooter.c` is to structure interactions with platform APIs, since they're the only operations for which success or failure is outside my control. My strategy is to run these operations during initialization, so the rest of the game doesn't have to worry about them:
- Acquire all platform and hardware resources during initialization. This includes preparing the drawing surface, getting device handles, starting threads, loading asset data, allocating GPU resources.
- Validate asset data during initialization.
- Use static memory for game objects so allocations aren't required while the game is running.

The platform operations themselves require structure in situations where a sequence of dependent resources is acquired one after the other, and on failure, successfully-acquired resources have to be released. For example, consider the following simplified version of opening a window and initializing OpenGL.

```c
// NOTE: This is not the actual implementation!
Display* display = openDisplay();
// Failures after this point must release display.

Window* window = openWindow(display);
// Failures after this point must release display and window.

GL* gl = initializeOpenGL(window);
// Failures after this point must release display and window and gl.

return SUCCESS;
```

I manage these sequences using [goto chains](https://wiki.sei.cmu.edu/confluence/display/c/MEM12-C.+Consider+using+a+goto+chain+when+leaving+a+function+on+error+when+using+and+releasing+resources) with labels based on the resources have been acquired and running in reverse order of the acquisitions.

```c
// NOTE: This is not the actual implementation!
Display* display = openDisplay();
if (!display) {
    goto ERROR_NO_RESOURCES;
}
// Errors here goto ERROR_DISPLAY

Window* window = openWindow(display);
if (!window) {
    goto ERROR_DISPLAY;
}
// Errors here goto ERROR_WINDOW

GL* gl = initializeOpenGL(window);
if (!gl) {
    goto ERROR_WINDOW;
}
// Errors here goto ERROR_GL

return SUCCESS;

ERROR_GL:
uninitializeOpenGL(gl);

ERROR_WINDOW:
closeWindow(window);

ERROR_DISPLAY:
closeDisplay(display);

ERROR_NO_RESOURCES:
return FAILURE;
```

### Mixin Structs

To simplify passing of game data between the different layers, I implement a struct "mixin" model using anonymous structs and unions. A struct that is to be used as a mixin is defined as follows:

```c
#define MY_STRUCT_BODY { int32_t x; int32_t y }
typedef struct MY_STRUCT_BODY MyStruct;
#define MY_STRUCT_MIXIN(name) union { struct MY_STRUCT_BODY; MyStruct name; }
```

This struct can then be mixed into another struct.

```c
typedef {
    MY_STRUCT_MIXIN(myStruct);
    int32_t z;
} MixedStruct;
```

This allows the members of the mixin struct to be used directly, or the mixin struct can be referenced as a whole by name.

```c
void myStructFunction(MyStruct ms) {
    // ...
}

MixedStruct mixedStruct = { .y = 2, .z = 3 };
mixedStruct.x = mixedStruct.y + mixedStruct.z;
myStructFunction(mixedStruct.myStruct);
```
### Data Structures

#### Sprite

The `Sprites_Sprite` struct ([sprites.h](./src/game/sprites.h)) represents a single sprite sheet, and contains data about dimensions, number of panels, panel dimensions, etc. It also contains the handle of the OpenGL texture used by the sprite sheet. This data is used by the rendering layer for drawing and by the game layer for positioning and collision logic.

#### Renderer_List

The `Renderer_List` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attribute data that will be drawn using a particular sprite sheet, such as position and the current sprite panel. Per-entity data is stored as statically allocated flat arrays to simplify submitting it to the GPU as buffer data for instanced draw calls.

#### Entities_List

The `Entities_List` struct ([entities.h](./src/game/entities.h)) represents all per-entity data used by the game. It contains a mixin of `Renderer_List`, which allows both the game and rendering layers to manipulate data, such as positions, that is relevant to both.

#### Player

The `Player` struct ([game.c](./src/game/game.c)) is a singleton that represents the player's current state. It contains a mixin of `Entities_List`, so it can be manipulated like any other game entity, as well as player-specific data like score and number of lives.

#### Event and Sequence

`space-shooter.c` uses a simplified event system inspired by the one used in [pacman.c](https://github.com/floooh/pacman.c). The `Events_Event` struct ([events.h](./src/game/events.h)) contains a delay in milliseconds, a duration in milliseconds, and an id used for checking whether it's currently active. The `Events_Sequence` struct ([events.h](./src/game/events.h)) contains an array of `Events_Event`s and metadata to manage them. See [Events](#events) for more details.


The Platform Layer
------------------

### The Drawing Surface

Managing the drawing surface in `space-shooter.c` involves standard usage of the relevant APIs ([Win32](https://docs.microsoft.com/en-us/windows/win32/), [Xlib](https://tronche.com/gui/x/xlib/), and [emscripten](https://emscripten.org/docs/api_reference/html5.h.html)), but there are two operations I found poorly-documented on one platform or another: hiding the mouse cursor and displaying a fullscreen window.

#### Windows

Hiding the cursor on Windows is straightforward using the [ShowCursor](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor) function, with the only subtlety being to make sure it only does so when the mouse is in the client area. I open a fullscreen window using [this technique](https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353) described by Raymond Chen.

```c
HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
MONITORINFO monitorInfo = { .cbSize = sizeof(MONITORINFO) };
GetMonitorInfo(monitor, &monitorInfo);
int32_t x = monitorInfo.rcMonitor.left;
int32_t y = monitorInfo.rcMonitor.top;
int32_t width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
int32_t height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
SetWindowLong(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
SetWindowPos(window, HWND_TOP, x, y, width, height, SWP_NOCOPYBITS | SWP_FRAMECHANGED);
```

#### Linux

I hide the cursor on Linux by creating a "blank" cursor.

```c
char hiddenCursorData = 0;
XColor hiddenCursorColor = { 0 };
Pixmap hiddenCursorPixmap = XCreatePixmapFromBitmapData(display, window, &hiddenCursorData, 1, 1, 1, 0, 1);
Cursor hiddenCursor = XCreatePixmapCursor(display, hiddenCursorPixmap, hiddenCursorPixmap, &hiddenCursorColor, &hiddenCursorColor, 0, 0);
XDefineCursor(display, window, hiddenCursor);
```

I open a fullscreen window by sending the relevant [Extended Window Manager Hint](https://specifications.freedesktop.org/wm-spec/1.3/index.html) events to the root window.

```c
#define NET_WM_STATE_ADD    1
Atom NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
Atom NET_WM_STATE_FULLSCREEN = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

XEvent fullscreenEvent = {
    .xclient = {
        // ...
        .message_type = NET_WM_STATE,
        .data = {
            .l = {
                NET_WM_STATE_ADD,
                NET_WM_STATE_FULLSCREEN,
                // ...
            }
        }
    }
};

XSendEvent(display, rootWindow, False, SubstructureNotifyMask | SubstructureRedirectMask, &fullscreenEvent);
```

#### Web

Hiding the cursor on the Web is straightforward using the [CSS cursor property](https://developer.mozilla.org/en-US/docs/Web/CSS/cursor) on the canvas element.

Entering fullscreen mode on the Web is somewhat awkward for a few reasons:
- Browsers enforce a policy that [fullscreen mode](https://developer.mozilla.org/en-US/docs/Web/API/Fullscreen_API/Guide#when_a_fullscreen_request_fails) can only be entered in a user input callback. 
- The [Web Gamepad API](https://developer.mozilla.org/en-US/docs/Web/API/Gamepad_API) uses polling (see below), so there are no user input callbacks that can be used to comply with this policy.

I addressed these problems by making the following design and architectural changes specific to the Web version of `space-shooter.c`:
- Only keyboard input is used to toggle fullscreen, even when a gamepad is being used to play.
- The logic for entering/exiting fullscreen is run in the keyboard event handler, rather than in the game loop as it is for the other platforms.

With those constraints in mind, I implement the fullscreen controls using the relevant [emscripten API](https://emscripten.org/docs/api_reference/html5.h.html#fullscreen) calls.

```c
// NOTE: error checks removed for clarity!
if (strEquals(keyEvent->code, "KeyF", 32)) {
    EmscriptenFullscreenChangeEvent status = { 0 };
    emscripten_get_fullscreen_status(&status)
    if (status.isFullscreen) {
        emscripten_exit_fullscreen();
    } else {
        emscripten_request_fullscreen_strategy("#canvas", EM_FALSE, &(EmscriptenFullscreenStrategy) {
            .scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
            .canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
            .filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST
        }); 
    }
}
``` 


### Initializing OpenGL

#### Windows

Creating a modern OpenGL context on Windows is a [convoluted process](https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)). The steps are:
1. Create a dummy window.
2. Create a dummy OpenGL context.
3. Get pointers to the `wglChoosePixelFormatARB` and `wglCreateContextAttribsARB` extension functions.
4. Destroy the dummy window and context (they cannot be reused because the pixel format can only be [set once for a window](https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat#remarks)).
5. Create the real window and context using the extension functions.

I extracted this functionality into a single-header library, [create-opengl-window.h](./lib/create-opengl-window.h). Once the context is created, OpenGL functions are loaded in the manner described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows).

```c
void *fn = (void *) wglGetProcAddress(openGLFunctionName);
if(fn == 0 || (fn == (void *) 0x1) || (fn == (void *) 0x2) || (fn == (void*) 0x3) || (fn == (void *) -1)) {
    fn = (void *) GetProcAddress(sogl_libHandle, openGLFunctionName);
}
```

I also extracted the logic for loading OpenGL functions on Windows and Linux into a single-header library, [simple-opengl-loader.h](./lib/simple-opengl-loader.h).

#### Linux

OpenGL context creation on Linux is also convoluted but doesn't require dummy context creation. The first step is to find a framebuffer configuration that satisfies the rendering requirements.

```c
int32_t numFBC = 0;
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

GLXFBConfig framebufferConfig = fbcList[0];
XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, framebufferConfig);
```

This configuration is then used to create a window.

```c
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
```

Finally, an OpenGL context is created with a matching framebuffer configuration. 

```c
GLXContext gl = glXCreateContextAttribsARB(display, framebufferConfig, NULL, True, (int32_t []) {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
});
```

A complete example of the process is available [here](https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)). Again, once the context is created, loading functions is straightforward using the process described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Linux_and_X-Windows).

```c
void* libHandle = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
void *fn = dlsym(sogl_libHandle, openGLFunctionName);
```

As mentioned above, I extracted the logic for loading OpenGL functions on Windows and Linux into a single-header library, [simple-opengl-loader.h](./lib/simple-opengl-loader.h).

#### Web

For the Web, WebGL context creation is straightforward using the appropriate emscripten API calls.

```c
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl = emscripten_webgl_create_context("#canvas", &(EmscriptenWebGLContextAttributes) {
    .majorVersion = 2,
    .minorVersion = 0
});

emscripten_webgl_make_context_current(gl);
```

I require a WebGL 2 context because the GLSL 3.3 versions of the shaders compile as GLSL ES 3.0 with almost no changes, while compiling to GLSL ES 1.0 would require, for example, changes to keywords and additional logic to determine attribute locations. The only modifications required to the compile the shaders as GLSL ES 3.0 were the version directives and precision qualifiers.

```c
#version 300 es
precision highp float;
```

### Audio

#### Windows

I implement Windows audio ([windows-audio.c](./src/platform/windows/windows-audio.c)) using [Xaudio2](https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-introduction), which structures mixing as an audio graph and handles creating a separate audio thread. Documentation on how to use Xaudio2 in C is scarce (I created a [demo application](https://github.com/tsherif/xaudio2-c-demo) to help with that), but the process is mostly straightforward using provided macros that map to the C++ methods described in the documentation, e.g. instead calling a method on an object,

```c++
xaudio->CreateMasteringVoice(xaudioMasterVoice, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
```

one passes the object to a similarly-named macro.

```c
IXAudio2_CreateMasteringVoice(xaudio, xaudioMasterVoice, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
```

One subtlety is defining the callbacks for a source voice, which in C++ is done by inheriting from `IXAudio2VoiceCallback` and overriding the relevant methods. I had to read the preprocessor output from `xaudio2.h` to discover that in C, this requires setting the `lpVtbl` member in the `IXAudio2VoiceCallback` struct.

```c
IXAudio2VoiceCallback callbacks = {
    .lpVtbl = &(IXAudio2VoiceCallbackVtbl) {
        .OnStreamEnd = OnStreamEnd,
        .OnVoiceProcessingPassEnd = OnVoiceProcessingPassEnd,
        .OnVoiceProcessingPassStart = OnVoiceProcessingPassStart,
        .OnBufferEnd = OnBufferEnd,
        .OnBufferStart = OnBufferStart,
        .OnLoopEnd = OnLoopEnd,
        .OnVoiceError = OnVoiceError
    }
};
```

The `space-shooter.c` audio graph is composed of 32 source voices connected directly to a single master voice. A source voice, its buffer and whether it's in use are managed by a simple `AudioStream` struct.

```c
typedef struct {
    IXAudio2SourceVoice* voice;
    XAUDIO2_BUFFER buffer;
    bool inUse;
} AudioStream;
```

This struct is also used as the context for the associated buffer, so it can be referenced in audio callbacks (see below).

```c
audioStrean.buffer.pContext = &audioStream;
```

Loading a sound involves loading the WAVE file and parsing out the PCM data, but otherwise no intermediate processing is required before it's submitted to a source voice. When a sound is played, the first available source voice is found and marked as in-use, and the audio buffer is submitted. 

```c
void platform_playSound(int32_t id, bool loop) {
    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        if (!audio.channels[i].inUse) {
            XAUDIO2_BUFFER* buffer = &audio.channels[i].buffer;
            buffer->LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
            buffer->AudioBytes = sounds.data[id].size;
            buffer->pAudioData = sounds.data[id].data;
            IXAudio2SourceVoice_Start(audio.channels[i].voice, 0, XAUDIO2_COMMIT_NOW);
            IXAudio2SourceVoice_SubmitSourceBuffer(audio.channels[i].voice, buffer, NULL);
            audio.channels[i].inUse = true;
            break;
        }
    }
}
```

The voice is then released using its `onBufferEnd` callback. The `pBufferContext` passed here is the address of `AudioStream` struct containing the buffer, and which the buffer's `pContext` member was set to above.

```c
void WINAPI OnBufferEnd(IXAudio2VoiceCallback* This, void* pBufferContext)    {
    AudioStream* channel = (AudioStream*) pBufferContext;
    channel->inUse = false;
}
```

#### Linux

I implement Linux audio ([linux-audio.c](./src/platform/linux/linux-audio.c)) using [ALSA](https://www.alsa-project.org/alsa-doc/alsa-lib/) to submit data to the audio device and [pthread](https://en.wikipedia.org/wiki/Pthreads) to create a separate audio thread. As on Windows, after the PCM data is parsed out of a file, no intermediate processing is required before it is submitted to the sound queue. Playing a sound involves adding the sound to a queue on the main thread, and sounds are copied from the queue into the mixer on each loop of the audio thread. ALSA only handles submission of audio data to the device so I implement a 32-channel additive mixer explicitly on the audio thread. Each channel of the mixer is represented by an `AudioStream` struct (defined differently than in the Windows audio layer) that keeps track of the data playing on the channel, how much of the data has already been played, and whether the channel is looping.

```c
typedef struct {
    int16_t* data;
    int32_t count;
    int32_t cursor;
    bool loop;
} AudioStream;
```

Mixing is performed by piecewise addition of corresponding samples from each channel. Results are [hard-clipped](https://www.hackaudio.com/digital-signal-processing/distortion-effects/hard-clipping/) to the 16-bit signed integer range.

```c
for (int32_t i = 0; i < numSamples; ++i) {
    mixer.buffer[i] = 0;
}  

for (int32_t i = 0; i < mixer.count; ++i) {
    AudioStream* channel = mixer.channels + i;

    for (int32_t i = 0; i < numSamples; ++i) {
        if (channel->cursor == channel->count) {
            if (channel->loop) {
                channel->cursor = 0;
            } else {
                break;
            }
        }

        int32_t sample = mixer.buffer[i] + channel->data[channel->cursor];
        
        if (sample < INT16_MIN) {
            sample = INT16_MIN;
        }

        if (sample > INT16_MAX) {
            sample = INT16_MAX;
        }

        mixer.buffer[i] = sample;
        ++channel->cursor;
    }
}
```

At the end of the audio thread loop, mixed audio is submitted to the device with a buffer size of 2048 frames (~50ms of audio).

```c
snd_pcm_writei(device, mixer.buffer, 2048);
```

`snd_pcm_writei` blocks until the device requires data, so the audio thread will wake up approximately once every 50ms.


#### Web

I implement audio for the Web using [OpenAL](https://www.openal.org/), which models audio as a graph similar to XAudio2. I create 32 sources, all connected to the default listener. Unlike Windows and Linux PCM data isn't directly submitted to a source, but must instead be copied into a buffer. To avoid continually copying data into a buffer when a sound is played, I create a buffer for each sound once when it is loaded.

```c
int32_t platform_loadSound(const char* fileName) {
    int32_t id = sounds.count;
    utils_loadWavData(fileName, sounds.data + id);
    alGenBuffers(1, sounds.buffers + id);
    alBufferData(sounds.buffers[id], AL_FORMAT_STEREO16, sounds.data[id].data, sounds.data[id].size, 44100);
    ++sounds.count;

    return id;
}
```

Similarly to the XAudio2 implementation, when a sound is played, the first available source is found and marked as in-use, but instead of submitting audio data to it directly, the source is simply connected to the sound's buffer which was created `platform_loadSound`.

```c
void platform_playSound(int32_t id, bool loop) {
    ALuint buffer = sounds.buffers[id];

    for (int32_t i = 0; i < SPACE_SHOOTER_AUDIO_MIXER_CHANNELS; ++i) {
        if (!audio.channels[i].inUse) {
            AudioStream* channel = audio.channels + i; 
            alSourcei(channel->source, AL_BUFFER, buffer);
            alSourcei(channel->source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE); 
            alSourcePlay(channel->source);
            channel->inUse = true;
            break;
        }
    }
}
```

Since OpenAL provides no callback API to signal when a sound has finished playing, I created a function `web_updateAudio`, called on each frame, that queries the state of each source currently marked as in-use and makes it available if it is no longer playing.

```c
void web_updateAudio(void) {
    for (int32_t i = 0; i < 32; ++i) {
        if (audio.channels[i].inUse) {
            ALint value;
            alGetSourcei(audio.channels[i].source, AL_SOURCE_STATE, &value);
            if (value != AL_PLAYING) {
                audio.channels[i].inUse = false;
            }
        }
    }
}
```

### Gamepad Support

#### Windows

[XInput](https://docs.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput) is by far the simplest OS API I worked with on `space-shooter.c`. The function `XInputGetState` queries the current state at a gamepad index and returns `ERROR_SUCCESS` if it's successful, so detecting a gamepad can be done with a simple loop.

```c
XINPUT_STATE xInputState;
int32_t gamepadIndex = -1;
for (int32_t i = 0; i < XUSER_MAX_COUNT; ++i) {
    if (XInputGetState(i, &xInputState) == ERROR_SUCCESS) {
        gamepadIndex = i;
        break;
    }
}
```

The current state of the gamepad is provided in the `XINPUT_STATE` struct.

```c
if (XInputGetState(gamepadIndex, &xInputState) == ERROR_SUCCESS) {
    int16_t stickX = xInputState.Gamepad.sThumbLX
    int16_t stickY = xInputState.Gamepad.sThumbLY
    bool aButton = (xInputState->Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    bool startButton = (xInputState->Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
    bool backButton = (xInputState->Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;

    // Process input
}
```

#### Linux

I implement Gamepad support on Linux using the [evdev](https://www.kernel.org/doc/html/v4.13/input/input.html#evdev) interface of the [Linux Input Subsystem](https://www.kernel.org/doc/html/v4.13/input/input.html). The first step is detecting whether a gamepad is connected by looking for files ending with ["-event-joystick"](https://wiki.archlinux.org/title/Gamepad#Gamepad_input_systems) in `/dev/input/by-id`.

```c
struct dirent* entry = readdir(inputDir);

while (entry) {
    if (endsWith(entry->d_name, "-event-joystick")) {
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/%s", "/dev/input/by-id/", entry->d_name);
        int32_t gamepadFd = open(path, O_RDONLY | O_NONBLOCK);

        // Test gamepad capabilities 
    }
    entry = readdir(inputDir);
}
```

`evdev` is an extremely generic interface and doesn't differentiate between device types other than to indicate what kinds of events they emit. So the next step is to ensure that the connected device emits events relevant to a gamepad, which is done via [ioctl calls](https://www.linuxjournal.com/article/6429). The event types I'm interested in for a gamepad are `EV_ABS` for the thumb stick and `EV_KEY` for the buttons, and the specific events I want are `ABS_X`, `ABS_Y`, `BTN_A`, `BTN_START`, `BTN_SELECT` (see the [Linux Gamepad Specification](https://www.kernel.org/doc/html/v4.13/input/gamepad.html) for more details). 

```c
// NOTE: ioctl error checks removed for clarity!

// Check for gamepad event types
uint8_t evBits[(EV_CNT + 7) / 8] = { 0 };
ioctl(gamepadFd, EVIOCGBIT(0, sizeof(evBits)), evBits);
if (!testBit(evBits, EV_ABS) || !testBit(evBits, EV_KEY)) {
    // Failure: go to next device
}

// Check for thumbstick events
uint8_t absBits[(ABS_CNT + 7) / 8] = { 0 };
ioctl(gamepadFd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits);
if (!testBit(absBits, ABS_X) || !testBit(absBits, ABS_Y)) {
   // Failure: go to next device
}

// Check for button events
uint8_t keyBits[(KEY_CNT + 7) / 8] = { 0 };
ioctl(gamepadFd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits);
if (!testBit(keyBits, BTN_A) || !testBit(keyBits, BTN_START) || !testBit(keyBits, BTN_SELECT)) {
    // Failure: go to next device
}

// Success!
```

Capturing gamepad input involves reading from the gamepad input file into [input_event](https://www.kernel.org/doc/html/v4.13/input/input.html#event-interface) structs and parsing them.

```c
struct input_event events[32];
int32_t bytesRead = read(gamepadFd, events, sizeof(events));

if (bytesRead >= 0) {
    int32_t numEvents = bytesRead / sizeof(struct input_event);

    for (int32_t i = 0; i < numEvents; ++i) {
        struct input_event* event = events + i;
        switch (event->type) {
            case EV_ABS: {
                // int16_t data will be in event->value
                switch (event->code) {
                    case ABS_X: // Do something...
                    case ABS_Y: // Do something...
                }
            } break;
            case EV_KEY: {
                // bool data will be in event->value
                switch (event->code) {
                    case BTN_A:      // Do something...
                    case BTN_START:  // Do something...
                    case BTN_SELECT: // Do something...
                }
            } break;
        }
    }
}

```

Raphael De Vasconcelos Nascimento provides a more detailed description of the entire process [here](https://web.archive.org/web/20210518003010/https://ourmachinery.com/post/gamepad-implementation-on-linux/).

#### Web

Gamepad support for the Web uses the relevant emscripten API functions. Connections are detected by setting the following callback via `emscripten_set_gamepadconnected_callback`, which simply tracks the index of the connected gamepad.

```c
EM_BOOL onGamepadConnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData) {
    if (gamepad.index == -1) {
        gamepad.index = gamepadEvent->index;
    }

    return EM_TRUE;
}
```

It's important to note that this function will only be called after the first user interaction with a gamepad while the page is visible, rather when a gamepad is actually connected. See [here](https://developer.mozilla.org/en-US/docs/Web/API/Gamepad_API/Using_the_Gamepad_API#connecting_to_a_gamepad) for details.

The logic for capturing gamepad input is similar to that of XInput.

```c
EmscriptenGamepadEvent gamepadState = { 0 };
emscripten_sample_gamepad_data();
emscripten_get_gamepad_status(gamepad.index, &gamepadState);

float x = (float) gamepadState.axis[GAMEPAD_HORIZONTAL_AXIS];
float y = (float) gamepadState.axis[GAMEPAD_VERTICAL_AXIS];
bool aButton = gamepadState.digitalButton[GAMEPAD_A_BUTTON];

// Process input
```

Unlike gamepad input for Windows and Linux, I don't capture the Back and Start buttons, which are the quit and fullscreen toggle inputs respectively. The latter was left out as "quitting" the web page didn't seem like a meaningful interaction. The latter was left out because, as mentioned above, gamepad inputs don't trigger input events which are required by the standard fullscreen API's security model.

Gamepad disconnections are detected in a callback set using `emscripten_set_gamepaddisconnected_callback`. If the currently-active gamepad is disconnected, the function will attempt to find another.

```c
EM_BOOL onGamepadDisconnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData) {
    if (gamepad.index == gamepadEvent->index) {
        gamepad.index = -1;
        
        emscripten_sample_gamepad_data()
        int32_t numGamepads = emscripten_get_num_gamepads();
        for (int32_t i = 0; i < numGamepads; ++i) {
            EmscriptenGamepadEvent gamepadState = { 0 };
            emscripten_get_gamepad_status(i, &gamepadState);
            if (gamepadState.connected) {
                gamepad.index = i;
                break;
            }
        }
    }

    return EM_TRUE;
}
```


### High-resolution Time

#### Windows

On Windows, time deltas for the game simulation are calculated using [QueryPerformanceCounter](https://docs.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter) and [QueryPerformanceFrequency](https://docs.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency) in the following manner:

```c
LARGE_INTEGER lastPerfCount, tickFrequency;
QueryPerformanceFrequency(&tickFrequency);
QueryPerformanceCounter(&lastPerfCount);

// Game loop
while (running) {

    LARGE_INTEGER perfCount;
    QueryPerformanceCounter(&perfCount);

    // Multiply into nanoseconds first to avoid precision loss in the division.
    int64_t elapsedCount = (perfCount.QuadPart - lastPerfCount.QuadPart) * SPACE_SHOOTER_SECOND;
    int64_t elapsedTime = elapsedCount / tickFrequency.QuadPart;

    game_update((float) elapsedTime / SPACE_SHOOTER_MILLISECOND);

    lastPerfCount = perfCount;
}
```

#### Linux

On Linux, time deltas for the game simulation are calculated using [clock_gettime](https://linux.die.net/man/3/clock_gettime).

```c
struct timespec timeSpec = { 0 };
clock_gettime(CLOCK_MONOTONIC, &timeSpec);
int64_t lastTime = timeSpec.tv_sec * SPACE_SHOOTER_SECOND + timeSpec.tv_nsec;

// Game loop
while (running) {

    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    int64_t time = timeSpec.tv_sec * SPACE_SHOOTER_SECOND + timeSpec.tv_nsec;
    int64_t elapsedTime = time - lastTime;

    game_update((float) elapsedTime / SPACE_SHOOTER_MILLISECOND);

    lastTime = time;
}
```

#### Web

The Web does not require any high-resolution timing as high-resolution timestamps are passed directly to game loop callback passed to `emscripten_request_animation_frame_loop`.

```c
EM_BOOL gameLoop(double time, void *userData) {

    float dt = time - state.lastFrameTime;
    state.lastFrameTime = time;

    game_update(dt);
}
```


### High-resolution Sleep

`space-shooter.c` uses vsync, if available, to control the frequency of the game loop. To avoid busy-looping when vsync isn't available, `space-shooter.c` will sleep if a frame runs under a minimum frame time of 3ms using the high-resolution sleep function available on each platform.

#### Windows

On Windows, the first step is to ensure the scheduler supports a granularity of 1ms using [timeBeginPeriod](https://docs.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod).

```
bool useSleep = timeBeginPeriod(1) == TIMERR_NOERROR;
```

If so, the game sleeps using [Sleep](https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep) and updates the calculated elapsed time.

```c
// Sleep if 1ms granularity is supported and the frame ran 
// as least 1ms faster than the minimum.
if (useSleep && SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime > SPACE_SHOOTER_MILLISECOND) {
    DWORD sleepMs = (DWORD) ((SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime) / SPACE_SHOOTER_MILLISECOND);
    Sleep(sleepMs);

    QueryPerformanceCounter(&perfCount);
    elapsedCount = (perfCount.QuadPart - lastPerfCount.QuadPart) * SPACE_SHOOTER_SECOND;
    elapsedTime = elapsedCount / tickFrequency.QuadPart;
}
```

#### Linux

On Linux, `space-shooter.c` sleeps using [nanosleep](https://linux.die.net/man/2/nanosleep).

```c
// Sleep if the frame ran at least 1ms less than the minimum.
if (SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime > SPACE_SHOOTER_MILLISECOND) {
    struct timespec sleepTime = {
        .tv_nsec = SPACE_SHOOTER_MIN_FRAME_TIME - elapsedTime
    };
    nanosleep(&sleepTime, NULL);

    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    time = timeSpec.tv_sec * SPACE_SHOOTER_SECOND + timeSpec.tv_nsec;
    elapsedTime = time - lastTime;
}
```

#### Web

The Web did not require any sleep logic as suspending execution is handled by `emscripten_request_animation_frame_loop`.


The Game Layer
--------------

### The Update Loop

The platform layer calls `game_update` in a loop, passing in the elapsed time in milliseconds since the last call. The behavior of the update depends on which of five states the game is in: `INPUT_TO_START_SCREEN`, `TITLE_SCREEN`, `LEVEL_TRANSITION`, `MAIN_GAME` or `GAME_OVER`. I implement each state as a single function and make the updates framerate-independent using [this technique](https://www.gafferongames.com/post/fix_your_timestep/) described by Glenn Fiedler. 

```c
#define TICK_DURATION 16.6667f

tickTime += elapsedTime;

if (tickTime > TICK_DURATION) {
    while (tickTime > TICK_DURATION) {
        currentStateFunction(TICK_DURATION);    
        tickTime -= TICK_DURATION;
    }

    currentStateFunction(tickTime);
    tickTime = 0.0f;
}
```

Essentially, the update functions "consume" the elapsed time in fixed time steps of 16ms and run a partial step for any time left over. In all honesty, this is overkill for `space-shooter.c` since all movement is linear, but I chose to leave it in after implementing it since it's not much more complex than using a single variable time step for each frame.

### Events

`space-shooter.c` uses an event system inspired by the one used in [pacman.c](https://github.com/floooh/pacman.c) but driven by time rather than frame ticks. An event is represented by the `Events_Event` struct.

```c
typedef struct {
    float delay;
    float duration;
    int32_t id;
} Events_Event;
```

The `id` member is an `enum` representing an event used in the game, e.g. `EVENTS_TITLE`, `EVENTS_DISPLAY`, `EVENTS_RESTART`. `Events_Sequence` structs represent a sequence of events that "play out" over time. They are defined as follows:

```c
Events_Sequence sequence = {
    .events = {
        { 
            .delay = 1000.0f,
            .duration = 2000.0f,
            .id =  EVENTS_DISPLAY
        },
        {
            .duration = 2500.0f,
            .id = EVENTS_FADE
        }
    },
    .count = 2
};
```

This defines a sequence that once started waits 1s, triggers the `EVENTS_DISPLAY` event, which lasts 2s, then triggers the `EVENTS_FADE` event, which lasts for 2.5s. Sequences can be made to loop by setting the `loop` member. A sequence similar to the following, for example, is used to flash text on the screen. 

```c
Events_Sequence loopingSequence = {
    .events = {
        { 
            .duration = 1000.0f,
            .id =  EVENTS_DISPLAY
        },
        {
            .duration = 1000.0f,
        }
    },
    .count = 2,
    .loop = true
};
``` 

Event sequences are managed using the following functions:
- `events_start(Events_Sequence* sequence)`: Start a sequence.
- `events_stop(Events_Sequence* sequence)`: Stop and reset a sequence.
- `events_beforeFrame(Events_Sequence* sequence, float elapsedTime)`: Update a sequence based on the elapsed time since the last frame.
- `events_on(Events_Sequence* sequence, int32_t id)`: Check if the provided event `id` is currently active in a sequence.

Usage might look like the following:

```c
events_start(&sequence);

// On each frame:
events_beforeFrame(&sequence, elapsedTime);

if (events_on(&sequence, EVENTS_DISPLAY)) {
    // ... Display something   
}
```

Sequences also have a `complete` member that is set when the sequence finishes running through all its events (and is, of course, never set for looping sequences). 

```c
events_beforeFrame(&sequence, elapsedTime);

if (sequence.complete) {
    // ... Do something
}
```

Note that only one event can be active per sequence at a given time. This wasn't a significant constraint in practice as multiple active events can be implemented by running multiple sequences in parallel.

The Rendering Layer
-------------------

#### Interface

To simplify calculations in the game layer, I define coordinates in `space-shooter.c` in terms of a rectangular canvas of 320 x 180 pixels, with the origin at the top-left corner. Mapping this space to the window's dimensions is done in `renderer_beforeFrame` via `glScissor` and `glViewport` calls which draw gray bars around the game canvas to ensure the aspect ratio doesn't change when the window is resized. `game_draw` calls `renderer_beforeFrame` once and then passes the `Renderer_List` mixin of each `Entity_List` to the rendering layer in calls to `renderer_draw`.

### OpenGL Primitives

In `renderer_draw`, the arrays of the `Renderer_List` are submitted to the GL in buffers that are used as instance attributes, and the dimensions and texture handle for each `Renderer_List`'s sprite are submitted as uniforms. All objects represented in a given `Renderer_List` are drawn in a single, instanced draw call, with each object represented as a quad sized to match the sprite panel. The values in `Renderer_List.positions` are interpreted as the top-left corner of the quad. The transformation between game and clip coordinates is done in the [vertex shader](./assets/shaders/vs.glsl).

```c
vec2 clipOffset = pixelOffset * pixelClipSize - 1.0;
gl_Position = vec4((vertexPosition * panelPixelSize * pixelClipSize + clipOffset) * vec2(1.0, -1.0), 0.0, 1.0);
``` 
- `vertexPosition` is the quad vertex position between (0.0, 0.0) and (1.0, 1.0).
- `pixelOffset` is the offset of the top-left corner of the quad in game pixel coordinates (i.e. the position from the `Renderer_List.positions` array).
- `panelPixelSize` is the dimensions of the sprite's panels in game pixel units.
- `clipOffset` is the offset of the top-left corner of the quad in clip coordinates.
- `pixelClipSize` is the size of one game pixel in clip units (i.e. `2.0 / vec2(gamePixelWidth, gamePixelHeight)`) .
