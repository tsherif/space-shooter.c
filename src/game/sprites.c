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
#include "sprites.h"

static Sprites_Animation shipAnimations[]  = {
    // Center
    {
        .frames = {{2, 0}, {2, 1}},
        .numFrames = 2
    },
    // Center left
    {
        .frames = {{1, 0}, {1, 1}},
        .numFrames = 2
    },
    // Left
    {
        .frames = {{0, 0}, {0, 1}},
        .numFrames = 2
    },
    // Center right
    {
        .frames = {{3, 0}, {3, 1}},
        .numFrames = 2
    },
    // Right
    {
        .frames = {{4, 0}, {4, 1}},
        .numFrames = 2
    }
};

static Sprites_Animation playerBulletAnimations[]  = {
    {
        .frames = {{0, 1}, {1, 1}},
        .numFrames = 2
    }
};

static Sprites_Animation enemyAnimations[]  = {
    {
        .frames = {{0, 0}, {1, 0}},
        .numFrames = 2
    }
};

static Sprites_Animation enemyBulletAnimations[]  = {
    {
        .frames = {{0, 0}, {1, 0}},
        .numFrames = 2
    }
};

static Sprites_Animation explosionAnimations[]  = {
    {
        .frames = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}},
        .numFrames = 5,
        .endBehavior = SPRITES_ANIMATION_END_KILL
    }
};

extern Sprites_Sprite sprites_shipSprite = {
    .panelDims = { 16.0f, 24.0f },
    .sheetDims = { 5.0f, 2.0f },
    .animations = shipAnimations,
    .numAnimations = sizeof(shipAnimations) / sizeof(shipAnimations[0])
};

extern Sprites_Sprite sprites_smallEnemySprite = {
    .panelDims = { 16.0f, 16.0f },
    .sheetDims = { 2.0f, 1.0f },
    .animations = enemyAnimations,
    .numAnimations = sizeof(enemyAnimations) / sizeof(enemyAnimations[0])
};

extern Sprites_Sprite sprites_mediumEnemySprite = {
    .panelDims = { 32.0f, 16.0f },
    .sheetDims = { 2.0f, 1.0f },
    .animations = enemyAnimations,
    .numAnimations = sizeof(enemyAnimations) / sizeof(enemyAnimations[0])
};

extern Sprites_Sprite sprites_largeEnemySprite = {
    .panelDims = { 32.0f, 32.0f },
    .sheetDims = { 2.0f, 1.0f },
    .animations = enemyAnimations,
    .numAnimations = sizeof(enemyAnimations) / sizeof(enemyAnimations[0])
};

extern Sprites_Sprite sprites_playerBulletSprite = {
    .panelDims = { 16.0f, 16.0f },
    .sheetDims = { 2.0f, 2.0f },
    .animations = playerBulletAnimations,
    .numAnimations = sizeof(playerBulletAnimations) / sizeof(playerBulletAnimations[0])
};

extern Sprites_Sprite sprites_enemyBulletSprite = {
    .panelDims = { 16.0f, 16.0f },
    .sheetDims = { 2.0f, 2.0f },
    .animations = enemyBulletAnimations,
    .numAnimations = sizeof(enemyBulletAnimations) / sizeof(enemyBulletAnimations[0])
};

extern Sprites_Sprite sprites_explosionSprite = {
    .panelDims = { 16.0f, 16.0f },
    .sheetDims = { 5.0f, 1.0f },
    .animations = explosionAnimations,
    .numAnimations = sizeof(explosionAnimations) / sizeof(explosionAnimations[0])
};