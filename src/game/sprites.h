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

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"

// Animations
#define SPRITES_SHIP_CENTER       0
#define SPRITES_SHIP_CENTER_LEFT  1
#define SPRITES_SHIP_LEFT         2
#define SPRITES_SHIP_CENTER_RIGHT 3
#define SPRITES_SHIP_RIGHT        4

#define SPRITES_SHIP_PANEL_WIDTH            16.0f
#define SPRITES_SHIP_PANEL_HEIGHT           24.0f
#define SPRITES_SMALL_ENEMY_PANEL_WIDTH     16.0f
#define SPRITES_SMALL_ENEMY_PANEL_HEIGHT    16.0f
#define SPRITES_MEDIUM_ENEMY_PANEL_WIDTH    32.0f
#define SPRITES_MEDIUM_ENEMY_PANEL_HEIGHT   16.0f
#define SPRITES_LARGE_ENEMY_PANEL_WIDTH     32.0f
#define SPRITES_LARGE_ENEMY_PANEL_HEIGHT    32.0f
#define SPRITES_BULLET_PANEL_WIDTH          16.0f
#define SPRITES_BULLET_PANEL_HEIGHT         16.0f
#define SPRITES_EXPLOSION_PANEL_WIDTH       16.0f
#define SPRITES_EXPLOSION_PANEL_HEIGHT      16.0f
#define SPRITES_TEXT_PANEL_WIDTH            32.0f
#define SPRITES_TEXT_PANEL_HEIGHT           32.0f

#define SPRITES_SHIP_BULLET_X_OFFSET ((SPRITES_SHIP_PANEL_WIDTH - SPRITES_BULLET_PANEL_WIDTH) / 2)
#define SPRITES_SHIP_BULLET_Y_OFFSET (-SPRITES_BULLET_PANEL_HEIGHT)
#define SPRITES_SHIP_EXPLOSION_X_OFFSET ((SPRITES_SHIP_PANEL_WIDTH - SPRITES_EXPLOSION_PANEL_WIDTH) / 2)
#define SPRITES_SHIP_EXPLOSION_Y_OFFSET ((SPRITES_SHIP_PANEL_HEIGHT - SPRITES_EXPLOSION_PANEL_HEIGHT) / 2)

#define SPRITES_SMALL_ENEMY_BULLET_X_OFFSET ((SPRITES_SMALL_ENEMY_PANEL_WIDTH - SPRITES_BULLET_PANEL_WIDTH) / 2)
#define SPRITES_SMALL_ENEMY_BULLET_Y_OFFSET ((SPRITES_SMALL_ENEMY_PANEL_HEIGHT - SPRITES_BULLET_PANEL_HEIGHT) / 2)
#define SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET ((SPRITES_SMALL_ENEMY_PANEL_WIDTH - SPRITES_EXPLOSION_PANEL_WIDTH) / 2)
#define SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET ((SPRITES_SMALL_ENEMY_PANEL_HEIGHT - SPRITES_EXPLOSION_PANEL_HEIGHT) / 2)

#define SPRITES_MEDIUM_ENEMY_BULLET_X_OFFSET ((SPRITES_MEDIUM_ENEMY_PANEL_WIDTH - SPRITES_BULLET_PANEL_WIDTH) / 2)
#define SPRITES_MEDIUM_ENEMY_BULLET_Y_OFFSET ((SPRITES_MEDIUM_ENEMY_PANEL_HEIGHT - SPRITES_BULLET_PANEL_HEIGHT) / 2)
#define SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET ((SPRITES_MEDIUM_ENEMY_PANEL_WIDTH - SPRITES_EXPLOSION_PANEL_WIDTH) / 2)
#define SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET ((SPRITES_MEDIUM_ENEMY_PANEL_HEIGHT - SPRITES_EXPLOSION_PANEL_HEIGHT) / 2)

#define SPRITES_LARGE_ENEMY_BULLET_X_OFFSET ((SPRITES_LARGE_ENEMY_PANEL_WIDTH - SPRITES_BULLET_PANEL_WIDTH) / 2)
#define SPRITES_LARGE_ENEMY_BULLET_Y_OFFSET ((SPRITES_LARGE_ENEMY_PANEL_HEIGHT - SPRITES_BULLET_PANEL_HEIGHT) / 2)
#define SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET ((SPRITES_LARGE_ENEMY_PANEL_WIDTH - SPRITES_EXPLOSION_PANEL_WIDTH) / 2)
#define SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET ((SPRITES_LARGE_ENEMY_PANEL_HEIGHT - SPRITES_EXPLOSION_PANEL_HEIGHT) / 2)

// Text sprite
#define SPRITES_TEXT_SPACING_SCALE 0.7f

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
    Sprites_CollisionBox collisionBox;
    Sprites_Animation* animations;
    float sheetDims[2];
    float panelDims[2];
    uint8_t numAnimations;
} Sprites_Sprite;

extern Sprites_Sprite sprites_ship;
extern Sprites_Sprite sprites_smallEnemy;
extern Sprites_Sprite sprites_mediumEnemy;
extern Sprites_Sprite sprites_largeEnemy;
extern Sprites_Sprite sprites_playerBullet;
extern Sprites_Sprite sprites_enemyBullet;
extern Sprites_Sprite sprites_explosion;
extern Sprites_Sprite sprites_text;
extern Sprites_Sprite sprites_whitePixel;

extern float sprites_shipBulletOffset[2];
extern float sprites_shipExplosionOffset[2];
extern float sprites_smallEnemyBulletOffset[2];
extern float sprites_smallEnemyExplosionOffset[2];
extern float sprites_mediumEnemyBulletOffset[2];
extern float sprites_mediumEnemyExplosionOffset[2];
extern float sprites_largeEnemyBulletOffset[2];
extern float sprites_largeEnemyExplosionOffset[2];

extern int8_t sprites_charToAnimationIndex(char c);

#endif