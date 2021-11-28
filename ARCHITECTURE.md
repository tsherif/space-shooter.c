The Architecture of space-shooter.c [WIP]
=========================================

This document outlines the high-level architecture of `space-shooter.c`. More detailed implementation details can be found in the source code.

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

The `SpritesSprite` struct ([sprites.h](./src/game/sprites.h)) represents a single spritesheet, and contains data about dimensions, number of panels, panel dimensions, etc. This data is used by the renderer layer for drawing and the game layer for positioning/collision logic.

**RendererList**

The `RendererList` struct ([renderer.h](./src/game/renderer.h)) represents all per-entity attibute data that will drawn with a particular spritesheet, such as position and current sprite panel.
