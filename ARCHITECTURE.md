The Architecture of space-shooter.c [WIP]
=========================================

This document outlines the high-level architecture of `space-shooter.c`. More detailed implementation details can be found in the source code.

Architectural Layers
--------------------

**Platform**

**Game**

**Rendering**


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

**Mixin Structs**

To simplify passing of game data between the different modules, a struct "mixin" model was implemented using anonymous structs and unions. A struct that is to be used as a mixin is defined as follows: 

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

**Sprite**

A `Sprites_Sprite` struct ([sprites.h](./src/game/sprites.h)) represents a single sprite sheet, and contains data about dimensions, number of panels, panel dimensions, etc. It also contains the handle of the OpenGL texture used by the sprite sheet. This data is used by the renderer layer for drawing and the game layer for positioning/collision logic.

**Renderer_List**

A `Renderer_List` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attribute data that will drawn with a particular sprite sheet, such as position and current sprite panel. Per-entity data is stored as statically allocated flat arrays to simplify uploading it to the GPU buffer data for instanced draw calls.

**Entities_List**

An `Entities_List` struct ([entities.h](./src/game/entities.h)) represents all per-entity data used by the game. It contains a mixin of `Renderer_List`, which allows both the game and rendering layers to manipulate data, such as positions, that is relevant to both.