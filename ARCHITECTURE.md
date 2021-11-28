The Architecture of space-shooter.c [WIP]
=========================================

This document outlines the high-level architecture of `space-shooter.c`. More detailed implementation details can be found in the source code.

Architectural Layers
--------------------

### Platform

The platform layer is implemented in the [platform/windows](./src/platform/windows/) and [platform/linux](./src/platform/linux/) directories and abstracted in a manner inspired by [Handmade Hero](https://handmadehero.org/). The platform layer opens a window, starts the game loop and communicates with the game layer using the interface defined in [platform-interface.h](./src/shared/platform-interface.h).

The game layer implements the following functions used by the platform layer:
- `game_init(void)`: Initialize game resources.
- `game_update(float elapsedTime)`: Update game state based on time elapsed since last call.
- `game_draw(void)`: Draw current frame.
- `game_resize(int width, int height)`: Update rendering state to match current window size.

The platform layer implements the following functions used by the game layer:
- `platform_getInput(Game_Input* input)`: Get current input state from the platform.
- `platform_playSound(Data_Buffer* sound, bool loop)`: Output sound to an audio device.
- `platform_debugLog(const char* message)`: Output a message intended for the developer while debugging.
- `platform_userMessage(const char* message)`: Output a message intended for the end user.
- `platform_loadFile(const char* fileName, Data_Buffer* buffer, bool nullTerminate)`: Load contents of a file into memory. Optionally, null-terminate if the data will be used as a string.

### Game

The game layer is implemented in [game.c](./src/game/game.c) and contains all the game logic, including capturing input from the platform layer, simulating the player and the world, checking for collisions, tracking and displaying the player's score and number of lives, etc. Most of this is done in operations on `Entities_List` structs. Once entities have been updated, the `Renderer_List` mixin for each `Entities_List` is passed to the rendering layer for drawing.

### Rendering

The rendering layer is implemented in [renderer.c](./src/game/renderer.c) and contains all OpenGL drawing logic. Each `Renderer_List` drawn in an instanced draw call.


Memory Model
------------

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

The only use of dynamic memory is in loading image and sound assets when the game initializes. This leads to a nice "dev ex" benefit that once the game initializes, errors related to memory allocation are no long a concern.

Data Model
----------

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

This allows the properties of the mixin struct to be used directly, or the mixin struct can be referenced as a whole by name:

```c
	void myStructFunction(MyStruct ms) {
		// ...
	}

	MixedStruct mixedStruct = { .y = 2, .z = 3 };
	mixedStruct.x = mixedStruct.y + mixedStruct.z;
	myStructFunction(ms.myStruct);
```

### Sprite

A `Sprites_Sprite` struct ([sprites.h](./src/game/sprites.h)) represents a single sprite sheet, and contains data about dimensions, number of panels, panel dimensions, etc. It also contains the handle of the OpenGL texture used by the sprite sheet. This data is used by the renderer layer for drawing and the game layer for positioning/collision logic.

### Renderer_List

A `Renderer_List` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attribute data that will drawn with a particular sprite sheet, such as position and current sprite panel. Per-entity data is stored as statically allocated flat arrays to simplify uploading it to the GPU buffer data for instanced draw calls.

### Entities_List

An `Entities_List` struct ([entities.h](./src/game/entities.h)) represents all per-entity data used by the game. It contains a mixin of `Renderer_List`, which allows both the game and rendering layers to manipulate data, such as positions, that is relevant to both.