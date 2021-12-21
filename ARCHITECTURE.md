The Architecture of space-shooter.c [WIP]
=========================================

- [Introduction](#introduction)
- [Architectural Overview](#architectural-overview)
- [Data Model](#architectural-overview)
- [The Platform Layer](#the-platform-layer)
- [The Game Layer](#the-game-layer)
- [The Rendering Layer](#the-rendering-layer)

Introduction
------------

In developing `space-shooter.c`, I iterated a several times on how to organize the different parts and learned to use a few more-or-less poorly-documented OS APIs on both Windows and Linux. This document is intended as a record of that process and to hopefully serve as a reference for others doing similar work. I'll note that none of this is intended as a definitive description of how any of this **should** be done. I'm not a professional C programmer, nor a professional game programmer. I read about the APIs, wrote code and solved problems as they arose, and this is where I ended up. 

I link to the references I used in building different parts of `space-shooter.c` throughout the text, but I'll call out a few that were especially invaluable:
- [Handmade Hero](https://handmadehero.org/) is an incredibly generous resource on many levels, but I think its most important effect on me was simply demystifying low-level OS APIs.
- [pacman.c](https://github.com/floooh/pacman.c) is a goldmine of ideas for simplified game systems.
- The source code of [sokol](https://github.com/floooh/sokol), [GLFW](https://github.com/glfw/glfw) and [SDL](https://github.com/libsdl-org/SDL) were my encyclopedias for how get things done in the platform layer. This was especially helpful on Linux where functionality is spread across several APIs and the documentation tends to be much worse.

Architectural Overview
----------------------

At a high level, the architecture of `space-shooter.c` involves 3 layers:

- The **platform layer** is implemented in the [platform/windows](./src/platform/windows/) and [platform/linux](./src/platform/linux/) directories and is responsible for:
	- Opening a window
	- Initializing OpenGL
	- Playing audio
	- Capturing user input
	- File I/O
	- Starting the game loop

- The **game layer** is implemented in [game.c](./src/game/game.c) and is responsible for
	- Initializing game resources
	- Updating game state (player actions based on input, collisions, tracking score, etc.)
	- Passing rendering data to the rendering layer based on the current game state

- The **rendering layer** is implemented in [renderer.c](./src/game/renderer.c) and is responsible for:
	- Managing OpenGL state and drawing to the screen

The platform layer interacts with the game and rendering layers using an API inspired by [Handmade Hero](https://handmadehero.org/) and defined in [platform-interface.h](./src/shared/platform-interface.h). The platform layer implements the following functions used by the game and rendering layers:
- `platform_getInput(Game_Input* input)`: Get current input state.
- `platform_playSound(Data_Buffer* sound, bool loop)`: Output sound to an audio device.
- `platform_debugLog(const char* message)`: Output a message intended for the developer while debugging.
- `platform_userMessage(const char* message)`: Output a message intended for the end user.
- `platform_loadFile(const char* fileName, Data_Buffer* buffer, bool nullTerminate)`: Load contents of a file into memory. Optionally, null-terminate if the data will be used as a string.

Once the platform layer initializes system resources, it calls into the game layer using the following lifecycle functions:
- `game_init(void)`: Initialize game resources.
- `game_update(float elapsedTime)`: Update game state based on time elapsed since last frame.
- `game_draw(void)`: Draw current frame.
- `game_resize(int width, int height)`: Update rendering state to match current window size.
- `game_close(void)`: Release game resources.

The rendering layer implements the following functions used by the game layer to draw (or update state related to drawing): 
- `renderer_init(int width, int height)`: Initialize OpenGL resources.
- `renderer_createTexture(uint8_t* data, int32_t width, int32_t height)`: Create a texture with the given data.
- `renderer_validate(void)`: Check that OpenGL context isn't out of memory.
- `renderer_resize(int width, int height)`: Resize the drawing surface.
- `renderer_beforeFrame(void)`: Prepare for drawing (primarily to fix aspect ratio and draw borders if necessary).
- `renderer_draw(Renderer_List* list)`: Draw to the screen.

Data Model
----------

### Loading Assets

Image assets for `space-shooter.c` are stored as [BMP files](https://en.wikipedia.org/wiki/BMP_file_format). They are parsed by the function `utils_bmpToImage()` ([utils.c]((./src/game/utils.c))), which is called in `game_init()`. To minimize the complexity of the parser, I imposed the requirement that the BMP data must be 32bpp, uncompressed BGRA data (the format exported by [GIMP](https://www.gimp.org/)).

Audio assets are stored as [WAVE files](http://soundfile.sapp.org/doc/WaveFormat/). They are parsed by the function `utils_wavToSound()` ([utils.c]((./src/game/utils.c))), which is called in `game_init()`. To minimize the complexity of the parser, I imposed the requirement that the WAVE data must be 44.1kHz, 16-bit stereo data, and the chunks must be in the order `RIFF`, `fmt` then `data`. This chunk order was the one I found in all the assets I use (but it isn't imposed by the WAVE format), and I used [Audacity](https://www.audacityteam.org/) to fix the sample rate and number of channels where necessary.

Failure to load image data will cause the game to abort. Failure to load audio data will still allow the game to run without the missing sounds. In debug builds, invalid data will cause the game to abort.

### Memory Management

Almost all memory allocations in `space-shooter.c` are static, with dynamic allocations only used to load image and sound assets when the game initializes. This leads to a nice "programmer peace of mind" benefit that once the game initializes, I no longer have to worry about errors related to allocating or freeing memory.

I implemented objects pools to manage objects that could exist in variable numbers in the game, such as game entities or sounds in the mixer. Conceptually, the object pool can be thought of as a static array of objects and a `count` that tracks how many of them are currently active.

```c
// NOTE: This is not the actual implementation!
struct {
	int32_t x;
	int32_t y;
} object

objects[MAX_OBJECTS];
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

Note that the implementations in most cases don't look exactly like the above depending on how the objects will be consumed by a given system. For example, game entity properties are stored as parallel arrays, rather than in per-object structs, to simplify submitting them to the GPU as attribute buffers.

### Error Handling

My primary concern in managing errors in `space-shooter.c` is to structure interactions with OS APIs, since they're the only operations that can fail in a manner outside my control. The key strategy is to run as many of these operations as possible during initialization, so the rest of the game doesn't have to worry about them:
- Acquire any required OS or hardware resources during initialization. This includes opening windows, getting device handles, starting threads, loading asset data, allocating GPU resources.
- Validate asset data during initialization.
- Use static memory for game objects so allocations aren't required while the game is running.

In terms of structuring the OS operations themselves, the only concern was in sequences where resources that are dependent on each other are allocated one after the other, and on failure, any allocated resources have to be freed. For example, consider the following simplified version of opening a window and initializing OpenGL:

```c
// NOTE: This is not the actual implementation!

Display* display = openDisplay();
// Failures after this point must release display.

Window* window = openWindow(display);
// Failures after this point must release display and window.

GL* gl = initializeOpenGL(display, window);
// Failures after this point must release display and window and gl.

return SUCCESS;
```

I manage these types of sequences using `goto` chains with labels based on the resources have been allocated and running in reverse order of the allocations:

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

GL* gl = initializeOpenGL(display, window);
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

To simplify passing of game data between the different layers, I implemented a struct "mixin" model using anonymous structs and unions. A struct that is to be used as a mixin is defined as follows: 

```c
#define MY_STRUCT_BODY { int32_t x; int32_t y }
typedef struct MY_STRUCT_BODY MyStruct;
#define MY_STRUCT_MIXIN(name) union { struct MY_STRUCT_BODY; MyStruct name; }
```

This struct can then be mixed into another struct:

```c
typedef {
	MY_STRUCT_MIXIN(myStruct);
	int32_t z;
} MixedStruct;
```

This allows the members of the mixin struct to be used directly, or the mixin struct can be referenced as a whole by name:

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

A `Sprites_Sprite` struct ([sprites.h](./src/game/sprites.h)) represents a single sprite sheet, and contains data about dimensions, number of panels, panel dimensions, etc. It also contains the handle of the OpenGL texture used by the sprite sheet. This data is used by the rendering layer for drawing and by the game layer for positioning/collision logic.

#### Renderer_List

A `Renderer_List` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attribute data that will be drawn with a particular sprite sheet, such as position and current sprite panel. Per-entity data is stored as statically allocated flat arrays to simplify uploading it to the GPU as buffer data for instanced draw calls.

#### Entities_List

An `Entities_List` struct ([entities.h](./src/game/entities.h)) represents all per-entity data used by the game. It contains a mixin of `Renderer_List`, which allows both the game and rendering layers to manipulate data, such as positions, that is relevant to both.

#### Player

The `Player` struct ([game.h](./src/game/game.h)) is singleton that represents the player's current state. It contains a mixin of `Entities_List` so it can be manipulated like any other game entity, as well as player-specific data like score and number of lives.

#### Event and Sequence

I implemented a relatively simple event system for`space-shooter.c` inspired by the one used in [pacman.c](https://github.com/floooh/pacman.c). The `Event` struct ([events.h](./src/game/events.h)) contains a delay in milliseconds, a duration in milliseconds, and an id used for checking whether it's currently active. The `Sequence` struct ([events.h](./src/game/events.h)) contains an array of `Event`s and metadata to manage them. See [Events](#events) for more details.


The Platform Layer
------------------

### Window Management

Most window management in `space-shooter.c` involves standard usage of the relevant APIS ([Win32](https://docs.microsoft.com/en-us/windows/win32/) and [Xlib](https://tronche.com/gui/x/xlib/)), but there are two pieces of functionality that aren't well-documented on one or both platforms: hiding the mouse cursor and displaying a fullscreen window.

#### Windows

Hiding the cursor was straightforward using the [ShowCursor](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor) function, with the only subtlety being to make sure it only does so when the mouse is in the client area. I implemented a fullscreen window using [this technique](https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353) described by Raymond Chen:

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

I hide the cursor in Xlib by creating a "blank" cursor:

```c
char hiddenCursorData = 0;
XColor hiddenCursorColor = { 0 };
Pixmap hiddenCursorPixmap = XCreatePixmapFromBitmapData(display, window, &hiddenCursorData, 1, 1, 1, 0, 1);
Cursor hiddenCursor = XCreatePixmapCursor(display, hiddenCursorPixmap, hiddenCursorPixmap, &hiddenCursorColor, &hiddenCursorColor, 0, 0);
XDefineCursor(display, window, hiddenCursor);
```

I display a fullscreen window by sending the relevant [Extended Window Manager Hint](https://specifications.freedesktop.org/wm-spec/1.3/index.html) events to the root window: 

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

### Initializing OpenGL

#### Windows

Creating a modern OpenGL context in Windows is a [convoluted process](https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)). The steps are:
1. Create a dummy window.
2. Create a dummy OpenGL context.
3. Get pointers to the `wglChoosePixelFormatARB` and `wglCreateContextAttribsARB` extension functions.
4. Destroy the dummy window and context (they cannot be reused because the pixel format can only be [set once for a window](https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat#remarks)).
5. Create the real window and context using the extension functions.

I extracted this functionality into a single-header library, [create-opengl.window.h](./lib/create-opengl.window.h). Once the context is created, OpenGL functions are loaded in the manner described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows):

```c
void *fn = (void *)wglGetProcAddress(openGLFunctionName);
if(fn == 0 || (fn == (void *) 0x1) || (fn == (void *) 0x2) || (fn == (void*) 0x3) || (fn == (void *) -1)) {
    fn = (void *) GetProcAddress(sogl_libHandle, openGLFunctionName);
}
```

I also extracted the logic for loading OpenGL functions into a single-header library, [simple-opengl-loader.h](./lib/simple-opengl-loader.h).

#### Linux

OpenGL context creation in Linux is also convoluted but doesn't require dummy context creation. The first step is to find a framebuffer configuration that satisfies the rendering requirements:

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
    GLX_SAMPLE_BUFFERS, 1,
    GLX_SAMPLES, 4,
    None
}, &numFBC);

GLXFBConfig framebufferConfig = fbcList[0];
XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, framebufferConfig);
```

This configuration can then be used to create a window that's configured properly:

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

Finally, an OpenGL context can be created with a matching framebuffer configuration: 

```c
GLXContext gl = glXCreateContextAttribsARB(display, framebufferConfig, NULL, True, (int32_t []) {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
});
```

A complete example of the process is available [here](https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)). Again, once the context is created, loading functions is straightforward using the process described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Linux_and_X-Windows):

```c

void* libHandle = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
void *fn = dlsym(sogl_libHandle, openGLFunctionName);
```

As mentioned above, I extracted the logic for loading OpenGL functions into a single-header library, [simple-opengl-loader.h](./lib/simple-opengl-loader.h).

### Audio

#### Windows

I implemented Windows audio ([windows-audio.c](./platform/windows/windows-audio.c)) using [Xaudio2](https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-introduction), which structures mixing as an audio graph and handles creating a separate audio thread. The `space-shooter.c` audio graph has 32 source voices connected directly to a single master voice. When a sound is played, the first available source voice is found and marked as in-use, and the audio buffer is submitted. The voice is then released using its `onBufferEnd` callback.

Documentation on how to use Xaudio2 in C is scarce (I created a [demo application](https://github.com/tsherif/xaudio2-c-demo) to help with that) but is mostly straightforward using provided macros that map to the C++ methods described in the documentation, e.g. instead calling a method on an object:

```c++
xaudio->CreateMasteringVoice(xaudioMasterVoice, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
```

one passes the object to a similarly-named macro:

```c
IXAudio2_CreateMasteringVoice(xaudio, xaudioMasterVoice, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
```

One trickier subtlety is how to define the callbacks for a source voice, which in C++ is done by inheriting from `IXAudio2VoiceCallback` and overriding the relevant methods. I had to read the preprocessor output from `xaudio2.h` to discover this requires setting a `lpVtbl` member in the `IXAudio2VoiceCallback` struct:

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

#### Linux

I implemented Linux audio ([linux-audio.c](./platform/linux/linux-audio.c)) using [ALSA](https://www.alsa-project.org/alsa-doc/alsa-lib/) to submit audio to the device and [pthread](https://en.wikipedia.org/wiki/Pthreads) to create a separate audio thread. Playing a sound involves adding the sound to a queue on the main thread, and sounds are copied from the queue into the mixer on each loop of the audio thread. ALSA only handles submission of audio data to the device so I implemented a 32-channel additive mixer explicitly on the audio thread:

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

At the end of the audio thread loop, mixed audio is submitted to the device with a buffer size of 2048 frames (~50ms of audio):

```c
	snd_pcm_writei(device, mixer.buffer, 2048)
```

`snd_pcm_writei` blocks until the device requires data, so the audio thread will wake up approximately once every 50ms.

### Gamepad Support

#### Windows

[XInput](https://docs.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput) was by far the simplest OS API I had to work with on `space-shooter.c`. The function `XInputGetState()` queries the current state at a gamepad index and returns `ERROR_SUCCESS` if it's successful, so detecting a gamepad can be done with a simple loop:

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

The current state of the gamepad is provided in the `XINPUT_STATE` struct:

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

I implemented Gamepad support on Linux using the [evdev](https://www.kernel.org/doc/html/v4.13/input/input.html#evdev) interface of the [Linux Input Subsystem](https://www.kernel.org/doc/html/v4.13/input/input.html). The first step is detecting whether a gamepad is connected by looking for files ending with ["-event-joystick"](https://wiki.archlinux.org/title/Gamepad#Gamepad_input_systems) in `/dev/input/by-id`: 

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

Capturing gamepad input involves reading from the gamepad input file into [input_event](https://www.kernel.org/doc/html/v4.13/input/input.html#event-interface) structs and parsing them: 

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
                    case BTN_A: 	 // Do something...
                    case BTN_START:  // Do something...
                    case BTN_SELECT: // Do something...
                }
            } break;
        }
    }
}

```

Raphael De Vasconcelos Nascimento provides a more detailed description of the entire process [here](https://ourmachinery.com/post/gamepad-implementation-on-linux/).


### High-resolution Time


### High-resolution Sleep


The Game Layer
--------------

### The Update Loop

The platform layer calls `game_update()` in a loop, passing in the elapsed time in milliseconds since the last call. The behavior of the update will depend on which of four states the game is in: `TITLE_SCREEN`, `MAIN_GAME`, `LEVEL_TRANSITION` or `GAME_OVER`. I implemented each state as a single function and made the updates framerate-independent using [this technique](https://www.gafferongames.com/post/fix_your_timestep/) described by Glenn Fiedler: 

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

Essentially, the update functions "consume" the elapsed time in fixed time steps of 16.6ms and run a partial step for any time left over. In all honesty, this is overkill for `space-shooter.c` since all the movement is linear, but I chose to leave it in after implementing it since it's not much more complex than the minimal approach of calling a state function once with the full elapsed time.

### Events

I implemented a simple event system in `space-shooter.c` inspired by the one used in [pacman.c](https://github.com/floooh/pacman.c), but driven by time rather than frame ticks. An event is represented by the `Events_Event` struct:

```c
typedef struct {
    float delay;
    float duration;
    int32_t id;
} Events_Event;
```

The `id` member will be an `enum` representing an event used in the game, e.g. `EVENTS_TITLE`, `EVENTS_DISPLAY`, `EVENTS_RESTART`. `Events_Sequence` structs represent a sequence of events that "play out" over time. They are defined as follows:

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
- `events_beforeFrame(Events_Sequence* sequence, float deltaTime)`: Update a sequence based on elapsed time since last frame.
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

if (sequence->complete) {
	// ... Do something
}
```

Note that only one event can be active per sequence at a given time. This wasn't a significant constraint in practice as multiple active events can be implemented by running multiple sequences in parallel.

The Rendering Layer
-------------------

#### Interface

To simplify calculations in the game layer, I defined coordinates in `space-shooter.c` in terms of a rectangular canvas of 320 x 180 pixels, with (0, 0) at the top-left. Mapping this space to the window's dimensions is done in `renderer_beforeFrame()` via `glScissor` and `glViewport` calls which draw gray bars around the game canvas to ensure the aspect ratio doesn't change if the window is resized. 

`game_draw()` calls `renderer_beforeFrame()` once and then passes the `Renderer_List` mixin of each `Entity_List` to the rendering layer in a call `renderer_draw()`.

### OpenGL Primitives

In `renderer_draw()`, the arrays in the passed `Renderer_List` are submitted to the GL in buffers that are used as instance attributes, and the dimensions and texture handle for each `Renderer_List`'s sprite are submitted as uniforms. All objects represented in the `Renderer_List` are drawn in a single, instanced draw call, with each object represented as a quad sized to match the sprite panel. The values in `Renderer_List.positions` are interpreted as the top-left corner of the quad. The transformation between game and clip coordinates is done in the [vertex shader](./assets/shaders/vs.glsl):

```c
vec2 clipOffset = pixelOffset * pixelClipSize - 1.0;
gl_Position = vec4((vertexPosition * panelPixelSize * pixelClipSize + clipOffset) * vec2(1.0, -1.0), 0.0, 1.0);
``` 
- `vertexPosition` is the quad vertex position between (0.0, 0.0) and (1.0, 1.0).
- `pixelOffset` is the offset of the top-left corner of the quad in game pixel coordinates (i.e. the position from the `Renderer_List.positions` array).
- `panelPixelSize` is the dimensions of the sprite's panels in game pixel units.
- `clipOffset` is the offset of the top-left corner of the quad in clip coordinates.
- `pixelClipSize` is the size of one game pixel in clip units (i.e. `1.0 / vec2(gamePixelWidth, gamePixelHeight)`) .
