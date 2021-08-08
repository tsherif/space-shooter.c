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

#ifndef _GAME_SPRITES_H_
#define _GAME_SPRITES_H_

#include "utils.h"
#include "renderer.h"

// Animations
#define SPRITES_SHIP_CENTER       0
#define SPRITES_SHIP_CENTER_LEFT  1
#define SPRITES_SHIP_LEFT         2
#define SPRITES_SHIP_CENTER_RIGHT 3
#define SPRITES_SHIP_RIGHT        4

typedef struct {
    float min[2];
    float max[2];
} Sprites_CollisionBox;

typedef enum {
    SPRITES_ANIMATION_END_LOOP,
    SPRITES_ANIMATION_END_KILL
} Sprites_AnimationEndBehavior;

typedef struct {
    uint8_t frames[32][2];
    uint8_t numFrames;
    Sprites_AnimationEndBehavior endBehavior;
} Sprites_Animation;

typedef struct {
    INLINE_STRUCT(Renderer_RenderList, renderList);
    Sprites_CollisionBox collisionBox;
    Sprites_Animation* animations;
    uint8_t numAnimations;
} Sprites_Sprite;

extern Sprites_Sprite sprites_shipSprite;
extern Sprites_Sprite sprites_smallEnemySprite;
extern Sprites_Sprite sprites_mediumEnemySprite;
extern Sprites_Sprite sprites_largeEnemySprite;
extern Sprites_Sprite sprites_playerBulletSprite;
extern Sprites_Sprite sprites_enemyBulletSprite;
extern Sprites_Sprite sprites_explosionSprite;

#endif