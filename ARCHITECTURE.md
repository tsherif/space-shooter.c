The Architecture of space-shooter.c [WIP]
=========================================

- [Architectural Overview](#architectural-overview)
- [Data Model](#architectural-overview)
- [The Platform Layer](#the-platform-layer)
- [The Game Layer](#the-game-layer)
- [The Rendering Layer](#the-rendering-layer)

Architectural Overview
----------------------

At a high level, the architecture of `space-shooter.c` involves 3 layers:

- The **platform layer** is implemented in the [platform/windows](./src/platform/windows/) and [platform/linux](./src/platform/linux/) and is responsible for:
	- Opening a window
	- Initializing OpenGL
	- Initializing audio
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
- `game_close()`: Release game resources.

The rendering layer implements the following functions used by the game layer to draw (or update state related to drawing): 
- `renderer_init(int width, int height)`: Initialize OpenGL resources.
- `renderer_createTexture(uint8_t* data, int32_t width, int32_t height)`: Create a texture with the given data.
- `renderer_resize(int width, int height)`: Resize the drawing surface.
- `renderer_beforeFrame(void)`: Prepare for drawing (primarily to fix aspect ratio and draw borders if necessary).
- `renderer_draw(Renderer_List* list)`: Draw to the screen.

Data Model
----------

### Loading Assets

Image assets for `space-shooter.c` are stored as [BMP files](https://en.wikipedia.org/wiki/BMP_file_format). They are parsed using the function `utils_bmpToImage()` ([utils.c]((./src/game/utils.c))) in `game_init()`. To simplify parsing, the BMP files are required to be 32bpp, uncompressed BGRA data (the format exported by [GIMP](https://www.gimp.org/)).

Audio assets are stored as [WAVE files](http://soundfile.sapp.org/doc/WaveFormat/). They are parsed using the function `utils_wavToSound()` ([utils.c]((./src/game/utils.c))) in `game_init()`. To simplify parsing, the WAVE files are required to contain 44.1kHz, 16-bit stereo data, and the chunks must be in the order RIFF, fmt then data.

Failure to load image data will cause the game to abort. Failure to load audio data will still allow the game to run without the missing sounds. In debug builds, invalid data will cause the game to abort.

### Memory Management

All memory for game objects in `space-shooter.c` is statically allocated in the arrays in `Renderer_List` and `Entities_List`, which are managed as an object pool. Conceptually, the object pool can be thought of as a static array of game objects and a `count` that tracks how many objects are currently active.

```c
// NOTE: This is not the actual implementation!
struct {
	int32_t x;
	int32_t y;
} object

objects[MAX_OBJECTS];
int32_t count;
```

"Allocating" an object involves initializing the `objects[count]` and then incrementing `count`.
```c
// NOTE: This is not the actual implementation!
int32_t newIndex = count;
objects[newIndex].x = 1;
objects[newIndex].y = 2;
++count;
```

"Deleting" and object involves swapping the deleted object with the `objects[count - 1]` and decrementing `count`.
```c
// NOTE: This is not the actual implementation!
int32_t lastIndex = count - 1;
objects[deletedIndex].x = objects[lastIndex].x;
objects[deletedIndex].y = objects[lastIndex].y;
--count;
```

All operations on game objects are run in batches on `objects[0 .. count - 1]`. The actual implementation is slightly more involved in that game object attributes are stored in parallel arrays to simplify their use as buffer data in the rendering layer.

The only use of dynamic memory is in loading image and sound assets when the game initializes. This leads to a nice "dev ex" benefit that once the game initializes, errors related to memory allocation are no longer possible.

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

A `Sprites_Sprite` struct ([sprites.h](./src/game/sprites.h)) represents a single sprite sheet, and contains data about dimensions, number of panels, panel dimensions, etc. It also contains the handle of the OpenGL texture used by the sprite sheet. This data is used by the renderer layer for drawing and the game layer for positioning/collision logic.

#### Renderer_List

A `Renderer_List` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attribute data that will drawn with a particular sprite sheet, such as position and current sprite panel. Per-entity data is stored as statically allocated flat arrays to simplify uploading it to the GPU buffer data for instanced draw calls.

#### Entities_List

An `Entities_List` struct ([entities.h](./src/game/entities.h)) represents all per-entity data used by the game. It contains a mixin of `Renderer_List`, which allows both the game and rendering layers to manipulate data, such as positions, that is relevant to both.

#### Player

The `Player` struct ([game.h](./src/game/game.h)) is singleton that represent the player's current state. It contains a mixin of `Entities_List` so it can be manipulated like any other game entity, as well as player-specific data like score and number of lives.

#### Event and Sequence

I implemented a relatively simple event system for`space-shooter.c` inspired by the one used in [pacman.c](https://github.com/floooh/pacman.c). The `Event` struct ([events.h](./src/game/events.h)) contains a delay in milliseconds, a duration in milliseconds, and an id used for checking whether it's currently active. The `Sequence` struct ([events.h](./src/game/events.h)) contains an array of `Event`s and metadata to manage them. See [Events](#events) for more details.


The Platform Layer
------------------

Many of the techniques described here were built using open-source projects like [sokol](https://github.com/floooh/sokol), [GLFW](https://github.com/glfw/glfw) and [SDL](https://github.com/libsdl-org/SDL) as invaluable references.

### Window Management

Window management involved straightforward usage of [Win32](https://docs.microsoft.com/en-us/windows/win32/) and [Xlib](https://tronche.com/gui/x/xlib/) with the only tricky parts on one or both platforms being hiding the mouse cursor and displaying a fullscreen window.

#### Windows

Hiding the cursor was straightforward using the [ShowCursor](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor) function, with the only subtlety being to make sure it only does so when the mouse is in the client area. Opening a fullscreen window was done using [this technique](https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353) described by Raymond Chen:

```c
HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
GetMonitorInfo(monitor, &monitorInfo);
int32_t x = monitorInfo.rcMonitor.left;
int32_t y = monitorInfo.rcMonitor.top;
int32_t width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
int32_t height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
SetWindowLong(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
SetWindowPos(window, HWND_TOP, x, y, width, height, SWP_NOCOPYBITS | SWP_FRAMECHANGED);
```

#### Linux

Hiding the cursor in Xlib requires creating a "blank" cursor:

```c
char hiddenCursorData = 0;
XColor hiddenCursorColor = { 0 };
Pixmap hiddenCursorPixmap = XCreatePixmapFromBitmapData(display, window, &hiddenCursorData, 1, 1, 1, 0, 1);
Cursor hiddenCursor = XCreatePixmapCursor(display, hiddenCursorPixmap, hiddenCursorPixmap, &hiddenCursorColor, &hiddenCursorColor, 0, 0);
XDefineCursor(display, window, hiddenCursor);
```

Opening a fullscreen window requires sending [Extended Window Manager Hint](https://specifications.freedesktop.org/wm-spec/1.3/index.html) events to the root window: 

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

Creating a modern OpenGL context in Windows is a [convoluted process](https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)). The steps involve:
1. Creating a dummy window
2. Creating a dummy OpenGL context
3. Getting pointers to the `wglChoosePixelFormatARB` and `wglCreateContextAttribsARB` extension functions.
4. Destroying the dummy window and context (they cannot be reused because the pixel format can only be [set once for a window](https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat#remarks))
5. Creating the real window and context using the extension functions.

I extracted this functionality into a single-header library, [create-opengl.window.h](./lib/create-opengl.window.h).

Once the context is created, OpenGL functions were loaded in the manner described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows):

```c
void *fn = (void *)wglGetProcAddress(openGLFunctionName);
if(fn == 0 || (fn == (void *) 0x1) || (fn == (void *) 0x2) || (fn == (void*) 0x3) || (fn == (void *) -1)) {
    fn = (void *) GetProcAddress(sogl_libHandle, openGLFunctionName);
}
```

I also extracted the logic for loading OpenGL functions into a single-header library, [simple-opengl-loader.h](./lib/simple-opengl-loader.h).

#### Linux

OpenGL context creation in Linux is a little simpler and doesn't require dummy context creation:

```c
glXCreateContextAttribsARBFUNC glXCreateContextAttribsARB = (glXCreateContextAttribsARBFUNC) glXGetProcAddress((const uint8_t *) "glXCreateContextAttribsARB");
glXSwapIntervalEXTFUNC glXSwapIntervalEXT = (glXSwapIntervalEXTFUNC) glXGetProcAddress((const uint8_t *) "glXSwapIntervalEXT");

int32_t visualAtt[] = { ... };
GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), visualAtt, &numFBC);

int32_t contextAttribs[] = { ... };
GLXContext ctx = glXCreateContextAttribsARB(display, *fbc, NULL, True, contextAttribs);

glXMakeCurrent(display, window, ctx);
```

A complete example of the process is provided by Apoorva Joshi [here](https://apoorvaj.io/creating-a-modern-opengl-context/).

Again, once the context is created, loading functions was straightforward using the process described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Linux_and_X-Windows):

```c

void* libHandle = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
void *fn = dlsym(sogl_libHandle, openGLFunctionName);
```

As mentioned above, I extracted the logic for loading OpenGL functions into a single-header library, [simple-opengl-loader.h](./lib/simple-opengl-loader.h).

### Audio

#### Windows

Windows audio ([windows-audio.c](./platform/windows/windows-audio.c)) is implemented using [Xaudio2](https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-introduction), which structures mixing as an audio graph and handles creating a separate audio thread. The `space-shooter.c` audio graph has 32 source voices connected directly to a single master voice. When a sound is played, the first available source voice is found and marked as in-use, and the audio buffer is submitted. The voice is then released using its `onBufferEnd` callback.

Documentation on how to use Xaudio2 in C is scarce (I created a [demo application](https://github.com/tsherif/xaudio2-c-demo) to help with that) but is mostly straightforward using provided macros that map to the C++ methods described in the documentation, e.g. instead of:

```c++
xaudio->CreateMasteringVoice(xaudioMasterVoice, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
```

you write:

```c
IXAudio2_CreateMasteringVoice(xaudio, xaudioMasterVoice, 2, 44100, 0, NULL, NULL, AudioCategory_GameEffects);
```

Once trickier subtlety is how to define the callbacks for a source voice, which in C++ is done by inheriting from `IXAudio2VoiceCallback` and overriding the relevant methods. I had to read the preprocessor output from `xaudio2.h` to discover this requires setting a `lpVtbl` member in the `IXAudio2VoiceCallback` struct:

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

Linux audio ([linux-audio.c](./platform/linux/linux-audio.c)) is implemented using [ALSA](https://www.alsa-project.org/alsa-doc/alsa-lib/) to submit audio to the device and [pthread](https://en.wikipedia.org/wiki/Pthreads) to create a separate audio thread. Playing a sound involves adding the sound to a queue on the main thread, and sounds are copied from the queue into the mixer on each loop of the audio thread. ALSA only handles submission of audio data to the device so a 32-channel additive mixer is implemented explicitly on the audio thread:

```c
for (int32_t i = 0; i < mixer.count; ++i) {
    AudioStream* channel = mixer.channels + i;
    int32_t samplesToMix = numSamples;
    int32_t samplesRemaining = channel->count - channel->cursor;

    if (samplesRemaining < numSamples) {
        samplesToMix = samplesRemaining;
    }

    for (int32_t i = 0; i < samplesToMix; ++i) {
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

#### Linux


The Game Layer
--------------

### Initialization

### Update Loop

### Events

The Rendering Layer
-------------------
