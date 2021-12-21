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

//////////////////////////////////////////////////////////////////////
// The Renderer manages all OpenGL state and handles drawing.
//////////////////////////////////////////////////////////////////////

#ifndef _GAME_RENDERER_H_
#define _GAME_RENDERER_H_
#include <stdbool.h>
#include "sprites.h"

///////////////////////////////////////////////////////////////////////
// The `Renderer_List` struct contains data required for drawing. Data
// is stored in statically allocated parallel arrays to simplify 
// uploading them to the GPU as instance attribute data. It is 
// implemented as a mixin to be used by `Entites_List` and `Player`.
// 
// Members:
// - position: pixel position of top-left corner of the entity
// - currentSpritePanel: 2D index of current sprite panel
// - scale: multiplicative scaling factor for entity sprite
// - alpha: blending alpha
// - whiteOut: boolean indicating entitiy should be drawn all white 
//      (used to indicate damage on enemies)
// - sprite: sprite sheet used to draw these entities
// - count: number of currently active entities
///////////////////////////////////////////////////////////////////////
#define RENDERER_DRAWLIST_MAX 256

#define RENDERER_LIST_BODY {\
    float position[RENDERER_DRAWLIST_MAX * 2];\
    float currentSpritePanel[RENDERER_DRAWLIST_MAX * 2];\
    float scale[RENDERER_DRAWLIST_MAX];\
    float alpha[RENDERER_DRAWLIST_MAX];\
    float whiteOut[RENDERER_DRAWLIST_MAX];\
    Sprites_Sprite* sprite;\
    int32_t count;\
}

typedef struct RENDERER_LIST_BODY Renderer_List;

#define RENDERER_LIST_MIXIN(name) union { struct RENDERER_LIST_BODY; Renderer_List name; }

///////////////////////////////////////////////////////////////////////////
// Renderer lifecycle functions.
//
// - renderer_init(): Initialize OpenGL resources.
// - renderer_createTexture(): Create a texture from the provided data.
// - renderer_validate(): Check that OpenGL context isn't out of memory.
// - renderer_resize(): Resize the viewport.
// - renderer_beforeFrame(): Prepare for a frame (fixes aspect ratio
//      and draws borders if necessary).
// - renderer_draw(): Draw the Renderer_List to the screen.
///////////////////////////////////////////////////////////////////////////

bool renderer_init(int width, int height);
uint32_t renderer_createTexture(uint8_t* data, int32_t width, int32_t height);
bool renderer_validate(void);
void renderer_resize(int width, int height);
void renderer_beforeFrame(void);
void renderer_draw(Renderer_List* list);

#endif
