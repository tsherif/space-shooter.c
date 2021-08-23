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

#ifndef _GAME_ENTITIES_H_
#define _GAME_ENTITIES_H_
#include "renderer.h"
#include "sprites.h"

typedef struct {
    float* position;           // vec2 pointer into Renderer_RenderList arrays
    float* currentSpritePanel; // vec2 pointer into Renderer_RenderList arrays
    uint8_t* whiteOut;         // pointer into Renderer_RenderList arrays
    float* scale;              // pointer into Renderer_RenderList arrays
    float* alpha;              // pointer into Renderer_RenderList arrays
    float velocity[2];
    uint8_t currentAnimation;
    uint8_t animationTick;
    Sprites_Sprite* sprite;
    uint8_t health;
} EntitiesEntity;

typedef struct {
    EntitiesEntity entities[RENDERER_DRAWLIST_MAX];
    uint8_t count;
} EntitiesList;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t currentAnimation;
    uint8_t health;
    float scale;
} EntitiesInitOptions;

extern void entities_updateAnimationPanel(EntitiesEntity* entity);
extern void entities_setAnimation(EntitiesEntity* entity, uint8_t animation);
extern void entities_updateAnimations(EntitiesList* list);
extern void entities_spawn(EntitiesList* list, Sprites_Sprite* sprite, EntitiesInitOptions* opts);
extern void entities_init(EntitiesEntity* entity, EntitiesInitOptions* opts);
extern void entities_kill(EntitiesList* list, uint8_t i);
extern void entities_fromText(EntitiesList* list, Sprites_Sprite* sprite, float x, float y, const char* text, float scale);

#endif