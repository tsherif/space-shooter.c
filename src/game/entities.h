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

#define ENTITIES_LIST_BODY {\
    RENDERER_LIST_MIXIN(renderList);\
    float velocity[RENDERER_DRAWLIST_MAX * 2];\
    int32_t currentAnimation[RENDERER_DRAWLIST_MAX];\
    int32_t animationTick[RENDERER_DRAWLIST_MAX];\
    int32_t health[RENDERER_DRAWLIST_MAX];\
    bool dead[RENDERER_DRAWLIST_MAX];\
}

typedef struct ENTITIES_LIST_BODY EntitiesList;

#define ENTITIES_LIST_MIXIN(name) union { struct ENTITIES_LIST_BODY; EntitiesList name; }

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int32_t currentAnimation;
    int32_t health;
    float scale;
    float transparency;
    uint8_t whiteOut;
} EntitiesInitOptions;

typedef struct {
    float x;
    float y;
    float scale;
    float transparency;
    bool reset;
} EntitiesFromTextOptions;

extern void entities_updateAnimationPanel(EntitiesList* list, int32_t i);
extern void entities_setAnimation(EntitiesList* list, int32_t i, int32_t animation);
extern void entities_updateAnimations(EntitiesList* list);
extern void entities_spawn(EntitiesList* list, EntitiesInitOptions* opts);
extern void entities_init(EntitiesList* list, int32_t i, EntitiesInitOptions* opts);
void entities_filterDead(EntitiesList* list);
extern void entities_fromText(EntitiesList* list, const char* text, EntitiesFromTextOptions* opts);

#endif
