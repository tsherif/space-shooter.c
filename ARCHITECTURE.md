The Architecture of space-shooter.c [WIP]
=========================================

This document outlines the high-level architecture of `space-shooter.c`. More detailed implementation details can be found in the source code.

Memory Model
------------

All memory for game objects in `space-shooter.c` is statically allocated in the arrays in `Renderer_List` and `Entities_List`, which are managed as an object pool. 

**TODO: DESCRIBE OBJECT POOL**

The only use of dynamic memory is in loading image and sound assets when the game initializes. This lead to a nice "dev ex" benefit that once the game initializes, errors related to memory allocation were no long a concern.

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

The `Sprites_Sprite` struct ([sprites.h](./src/game/sprites.h)) represents a single spritesheet, and contains data about dimensions, number of panels, panel dimensions, etc. It also contains the handle of the OpenGL texture used by the spritesheet. This data is used by the renderer layer for drawing and the game layer for positioning/collision logic.

**Renderer_List**

The `RendererList` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attibute data that will drawn with a particular spritesheet, such as position and current sprite panel. Per-entity data is stored as statically allocated flat arrays to simplify uploading it to the GPU buffer data for instanced draw calls.
