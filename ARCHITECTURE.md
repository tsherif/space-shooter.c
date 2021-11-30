The Architecture of space-shooter.c [WIP]
=========================================

- [Architectural Overview](#architectural-overview)
- [The Platform Layer](#the-platform-layer)
- [Data Model](#architectural-overview)
- [The Game Layer](#the-game-layer)

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

The platform layer interacts with the game and rendering layersusing an API insprired by [Handmade Hero](https://handmadehero.org/) and defined in [platform-interface.h](./src/shared/platform-interface.h). The platform layer implements the following functions used by the game and rendering layers:
- `platform_getInput(Game_Input* input)`: Get current input state from the platform.
- `platform_playSound(Data_Buffer* sound, bool loop)`: Output sound to an audio device.
- `platform_debugLog(const char* message)`: Output a message intended for the developer while debugging.
- `platform_userMessage(const char* message)`: Output a message intended for the end user.
- `platform_loadFile(const char* fileName, Data_Buffer* buffer, bool nullTerminate)`: Load contents of a file into memory. Optionally, null-terminate if the data will be used as a string.

Once the platform layer initializes system resources, it calls into the game layer using the following lifecycle functions:
- `game_init(void)`: Initialize game resources.
- `game_update(float elapsedTime)`: Update game state based on time elapsed since last frame.
- `game_draw(void)`: Draw current frame.
- `game_resize(int width, int height)`: Update rendering state to match current window size.

The rendering layer implements the following functions used by the game layer to draw (or update state related to drawing): 
- `renderer_init(int width, int height)`: Initialize OpenGL resources.
- `renderer_createTexture(uint8_t* data, int32_t width, int32_t height)`: Create a texture with the given data.
- `renderer_resize(int width, int height)`: Resize the drawing surface.
- `renderer_beforeFrame(void)`: Prepare for drawing (primarily to fix aspect ratio and draw borders if necessary).
- `renderer_draw(Renderer_List* list)`: Draw to the screen.


The Platform Layer
------------------

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
XEvent fullscreenEvent = {
    .xclient = {
    	// ...
        .message_type = NET_WM_STATE,
        .data = {
            .l = {
                _NET_WM_STATE_ADD,
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
1. Create a dummy window
2. Create a dummy OpenGL context
3. Get pointers to the `wglChoosePixelFormatARB` and `wglCreateContextAttribsARB` extension functions.
4. Destroy the dummy window and context (they cannot be reused because the pixel format can only be [set once for a window](https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat#remarks))
5. Create the real window and context using the extension functions.

This functionality was extracted out into [create-opengl.window.h](./lib/create-opengl.window.h) and is used as a single-header library.

Once the context is created, OpenGL functions were loaded in the manner described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows):

```c
void *fn = (void *)wglGetProcAddress(openGLFunctionName);
if(fn == 0 || (fn == (void *) 0x1) || (fn == (void *) 0x2) || (fn == (void*) 0x3) || (fn == (void *) -1)) {
    fn = (void *) GetProcAddress(sogl_libHandle, openGLFunctionName);
}
```

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

A more complete example of the process can be found [here](https://apoorvaj.io/creating-a-modern-opengl-context/).

Again, once the context is created, loading functions was straightforward using the process described [here](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Linux_and_X-Windows):

```c

void* libHandle = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
void *fn = dlsym(sogl_libHandle, openGLFunctionName);
```

### Audio

#### Windows

#### Linux


### Gamepad Support

#### Windows

#### Linux

Data Model
----------

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

"Deleting" involves swapping the deleted object with the `objects[count - 1]` and decrementing `count`.
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

To simplify passing of game data between the different layers, a struct "mixin" model was implemented using anonymous structs and unions. A struct that is to be used as a mixin is defined as follows: 

```c
#define MY_STRUCT_BODY { int32_t x; int32_t y }
typedef struct MY_STRUCT_BODY MyStruct;
#define MY_STRUCT_MIXIN(name) union { struct MY_STRUCT_BODY; MyStruct name; }
```

This struct can then be mixed in to another struct:

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

### Sprite

A `Sprites_Sprite` struct ([sprites.h](./src/game/sprites.h)) represents a single sprite sheet, and contains data about dimensions, number of panels, panel dimensions, etc. It also contains the handle of the OpenGL texture used by the sprite sheet. This data is used by the renderer layer for drawing and the game layer for positioning/collision logic.

### Renderer_List

A `Renderer_List` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attribute data that will drawn with a particular sprite sheet, such as position and current sprite panel. Per-entity data is stored as statically allocated flat arrays to simplify uploading it to the GPU buffer data for instanced draw calls.

### Entities_List

An `Entities_List` struct ([entities.h](./src/game/entities.h)) represents all per-entity data used by the game. It contains a mixin of `Renderer_List`, which allows both the game and rendering layers to manipulate data, such as positions, that is relevant to both.

### Player

The `Player` struct ([game.h](./src/game/game.h)) is singleton that represent the player's current state. It contains a mixin of `Entities_List` so it can be manipulated like any other game entity, as well as player-specific data like score and number of lives.

### Event and Sequence

`space-shooter.c` implements a relatively simple event system inspired by [pacman.c](https://github.com/floooh/pacman.c). The `Event` struct ([events.h](./src/game/events.h)) contains a delay in milliseconds, a duration in milliseconds, and an id used for checking whether it's currently active. The `Sequence` struct ([events.h](./src/game/events.h)) contains an array of `Event`s and metadata to manage them. See [#events] for more details.


The Game Layer
--------------

### Initialization

### Update Loop

### Framerate Independance

### Events
