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

#ifndef _GAME_RENDERER_H_
#define _GAME_RENDERER_H_
#include <stdbool.h>
#include "../../lib/simple-opengl-loader.h"

#define SPRITE_SCALE 4.0f

typedef struct {
    uint8_t frames[32][2];
    uint8_t numFrames;
} Animation;

typedef struct {
    float panelDims[2];
    float sheetDims[2];
    Animation* animations;
    uint8_t numAnimations;
    GLuint texture;
} Sprite;

typedef struct {
    float position[2];
    float velocity[2];
    bool faceLeft;
    uint8_t currentAnimation;
    uint8_t animationTick;
    uint8_t currentSpritePanel[2];
    Sprite* sprite;
    uint16_t bulletThrottle;
} Character;

void renderer_drawCharacters(Character* list, uint8_t count);
bool renderer_loadTexture(const char* fileName, GLuint* texture);
void renderer_resize(int width, int height);
void renderer_init();

#endif