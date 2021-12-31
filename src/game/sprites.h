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
// Sprites represent the sprite sheets used to draw game objects.
//////////////////////////////////////////////////////////////////////

#ifndef _GAME_SPRITES_H_
#define _GAME_SPRITES_H_

#include <stdbool.h>
#include <stdint.h>

//////////////////
// Animations
//////////////////

#define SPRITES_PLAYER_CENTER       0
#define SPRITES_PLAYER_CENTER_LEFT  1
#define SPRITES_PLAYER_LEFT         2
#define SPRITES_PLAYER_CENTER_RIGHT 3
#define SPRITES_PLAYER_RIGHT        4


//////////////////////////////////
// Sprite sheet panel dimensions
//////////////////////////////////

#define SPRITES_PLAYER_PANEL_WIDTH          16.0f
#define SPRITES_PLAYER_PANEL_HEIGHT         24.0f
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


///////////////////////////////////////////////////
// Offsets for positioning bullets and explosions
// relative to game characters.
///////////////////////////////////////////////////

#define SPRITES_PLAYER_BULLET_X_OFFSET ((SPRITES_PLAYER_PANEL_WIDTH - SPRITES_BULLET_PANEL_WIDTH) / 2)
#define SPRITES_PLAYER_BULLET_Y_OFFSET (-SPRITES_BULLET_PANEL_HEIGHT)
#define SPRITES_PLAYER_EXPLOSION_X_OFFSET ((SPRITES_PLAYER_PANEL_WIDTH - SPRITES_EXPLOSION_PANEL_WIDTH) / 2)
#define SPRITES_PLAYER_EXPLOSION_Y_OFFSET ((SPRITES_PLAYER_PANEL_HEIGHT - SPRITES_EXPLOSION_PANEL_HEIGHT) / 2)

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

/////////////////////////////////////////////
// Font spaced the characters too far apart
// so this multiplier is used to scale that 
// spacing.
/////////////////////////////////////////////

#define SPRITES_TEXT_SPACING_SCALE 0.7f


//////////////////////////////////////////////////////////////////////////
// The `Sprites_Sprite` struct contains data relevant to the 
// sprite sheet used for drawing a game object
// 
// Members:
// - collisionBox: bounds of the actual image with the sprite panel.
//      Used in collision detection.
// - animations: array of animations available in the sprite sheet. 
//      Each animation is an array of 2D indices into the sprite
//      sheet indicating the frames of the animation.
// - sheetDims: dimensions of the sprite sheet in panels.
// - panelDims: dimensions of each panel in pixels.
// - numAnimations: length of the animations array.
// - texture: handle to the OpenGL texture containing the sprite sheet.
//////////////////////////////////////////////////////////////////////////

typedef struct {
    float min[2];
    float max[2];
} Sprites_CollisionBox;

typedef enum {
    SPRITES_ANIMATION_END_LOOP,
    SPRITES_ANIMATION_END_KILL
} Sprites_AnimationEndBehavior;

typedef struct {
    float frames[32][2];
    int32_t numFrames;
    Sprites_AnimationEndBehavior endBehavior;
} Sprites_Animation;

typedef struct {
    Sprites_CollisionBox collisionBox;
    Sprites_Animation* animations;
    float sheetDims[2];
    float panelDims[2];
    int32_t numAnimations;
    uint32_t texture;
} Sprites_Sprite;


///////////////////////////////////////////////
// Utility function to convert a char to 
// an animation index in the font sprite
// sheet (which represents a single letter).
///////////////////////////////////////////////

int32_t sprites_charToAnimationIndex(char c);

/////////////////////////////////////////////
// Sprites used in the game
/////////////////////////////////////////////

extern Sprites_Sprite sprites_player;
extern Sprites_Sprite sprites_smallEnemy;
extern Sprites_Sprite sprites_mediumEnemy;
extern Sprites_Sprite sprites_largeEnemy;
extern Sprites_Sprite sprites_playerBullet;
extern Sprites_Sprite sprites_enemyBullet;
extern Sprites_Sprite sprites_explosion;
extern Sprites_Sprite sprites_text;
extern Sprites_Sprite sprites_whitePixel;
extern Sprites_Sprite sprite_rocket;

#endif
