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

#define PUNCTUATION ".,/<>(){}[]?;:'\"/!|=+_-*^%#@$"

int32_t sprites_charToAnimationIndex(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }

    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    }

    if (c >= '1' && c <= '9') {
        return c - '1' + 26;
    }

    if (c == '0') {
        return 35;
    }

    int32_t i = 0; 
    while (PUNCTUATION[i]) {
        if (c == PUNCTUATION[i]) {
            return i + 36;
        }
        ++i;
    }

    return -1;
}

static Sprites_Animation shipAnimations[]  = {
    // Center
    {
        .frames = {{2.0f, 0.0f}, {2.0f, 1.0f}},
        .numFrames = 2
    },
    // Center left
    {
        .frames = {{1.0f, 0.0f}, {1.0f, 1.0f}},
        .numFrames = 2
    },
    // Left
    {
        .frames = {{0.0f, 0.0f}, {0.0f, 1.0f}},
        .numFrames = 2
    },
    // Center right
    {
        .frames = {{3.0f, 0.0f}, {3.0f, 1.0f}},
        .numFrames = 2
    },
    // Right
    {
        .frames = {{4.0f, 0.0f}, {4.0f, 1.0f}},
        .numFrames = 2
    }
};

static Sprites_Animation playerBulletAnimations[]  = {
    {
        .frames = {{0.0f, 1.0f}, {1.0f, 1.0f}},
        .numFrames = 2
    }
};

static Sprites_Animation enemyAnimations[]  = {
    {
        .frames = {{0.0f, 0.0f}, {1.0f, 0.0f}},
        .numFrames = 2
    }
};

static Sprites_Animation enemyBulletAnimations[]  = {
    {
        .frames = {{0.0f, 0.0f}, {1.0f, 0.0f}},
        .numFrames = 2
    }
};

static Sprites_Animation explosionAnimations[]  = {
    {
        .frames = {{0.0f, 0.0f}, {1.0f, 0.0f}, {2.0f, 0.0f}, {3.0f, 0.0f}, {4.0f, 0.0f}},
        .numFrames = 5,
        .endBehavior = SPRITES_ANIMATION_END_KILL
    }
};

static Sprites_Animation fontAnimations[]  = {
    { .frames = {{0.0f, 0.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 0.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 0.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 0.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 0.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 1.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 1.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 1.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 1.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 1.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 2.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 2.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 2.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 2.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 2.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 3.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 3.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 3.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 3.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 3.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 4.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 4.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 4.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 4.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 4.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 5.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 5.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 5.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 5.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 5.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 6.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 6.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 6.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 6.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 6.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 7.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 7.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 7.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 7.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 7.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 8.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 8.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 8.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 8.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 8.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 9.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 9.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 9.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 9.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 9.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 10.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 10.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 10.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 10.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 10.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 11.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 11.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 11.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 11.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 11.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 12.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 12.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 12.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 12.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 12.0f}}, .numFrames = 1},
    { .frames = {{0.0f, 13.0f}}, .numFrames = 1},
    { .frames = {{1.0f, 13.0f}}, .numFrames = 1},
    { .frames = {{2.0f, 13.0f}}, .numFrames = 1},
    { .frames = {{3.0f, 13.0f}}, .numFrames = 1},
    { .frames = {{4.0f, 13.0f}}, .numFrames = 1}    

};

Sprites_Sprite sprites_ship = {
    .panelDims = { SPRITES_SHIP_PANEL_WIDTH, SPRITES_SHIP_PANEL_HEIGHT },
    .sheetDims = { 5.0f, 2.0f },
    .collisionBox = {
        .min = {0.0f, 0.0f},
        .max = {16.0f, 15.0f}
    },
    .animations = shipAnimations,
    .numAnimations = sizeof(shipAnimations) / sizeof(shipAnimations[0])
};

Sprites_Sprite sprites_smallEnemy = {
    .panelDims = { SPRITES_SMALL_ENEMY_PANEL_WIDTH, SPRITES_SMALL_ENEMY_PANEL_HEIGHT },
    .sheetDims = { 2.0f, 1.0f },
    .collisionBox = {
        .min = {0.0f, 5.0f},
        .max = {16.0f, 16.0f}
    },
    .animations = enemyAnimations,
    .numAnimations = sizeof(enemyAnimations) / sizeof(enemyAnimations[0])
};

Sprites_Sprite sprites_mediumEnemy = {
    .panelDims = { SPRITES_MEDIUM_ENEMY_PANEL_WIDTH, SPRITES_MEDIUM_ENEMY_PANEL_HEIGHT },
    .sheetDims = { 2.0f, 1.0f },
    .collisionBox = {
        .min = {0.0f, 0.0f},
        .max = {32.0f, 16.0f}
    },
    .animations = enemyAnimations,
    .numAnimations = sizeof(enemyAnimations) / sizeof(enemyAnimations[0])
};

Sprites_Sprite sprites_largeEnemy = {
    .panelDims = { SPRITES_LARGE_ENEMY_PANEL_WIDTH, SPRITES_LARGE_ENEMY_PANEL_HEIGHT },
    .sheetDims = { 2.0f, 1.0f },
    .collisionBox = {
        .min = {3.0f, 12.0f},
        .max = {29.0f, 32.0f}
    },
    .animations = enemyAnimations,
    .numAnimations = sizeof(enemyAnimations) / sizeof(enemyAnimations[0])
};

Sprites_Sprite sprites_playerBullet = {
    .panelDims = { SPRITES_BULLET_PANEL_WIDTH, SPRITES_BULLET_PANEL_HEIGHT },
    .sheetDims = { 2.0f, 2.0f },
    .collisionBox = {
        .min = {4.0f, 2.0f},
        .max = {11.0f, 15.0f}
    },
    .animations = playerBulletAnimations,
    .numAnimations = sizeof(playerBulletAnimations) / sizeof(playerBulletAnimations[0])
};

Sprites_Sprite sprites_enemyBullet = {
    .panelDims = { SPRITES_BULLET_PANEL_WIDTH, SPRITES_BULLET_PANEL_HEIGHT },
    .sheetDims = { 2.0f, 2.0f },
    .collisionBox = {
        .min = {4.0f, 7.0f},
        .max = {11.0f, 12.0f}
    },
    .animations = enemyBulletAnimations,
    .numAnimations = sizeof(enemyBulletAnimations) / sizeof(enemyBulletAnimations[0])
};

Sprites_Sprite sprites_explosion = {
    .panelDims = { SPRITES_EXPLOSION_PANEL_WIDTH, SPRITES_EXPLOSION_PANEL_HEIGHT },
    .sheetDims = { 5.0f, 1.0f },
    .animations = explosionAnimations,
    .numAnimations = sizeof(explosionAnimations) / sizeof(explosionAnimations[0])
};

Sprites_Sprite sprites_text = {
    .panelDims = { SPRITES_TEXT_PANEL_WIDTH, SPRITES_TEXT_PANEL_HEIGHT },
    .sheetDims = { 5.0f, 13.0f },
    .animations = fontAnimations,
    .numAnimations = sizeof(fontAnimations) / sizeof(fontAnimations[0])
};

Sprites_Sprite sprites_whitePixel = {
    .panelDims = { 1, 1 },
    .sheetDims = { 1.0f, 1.0f },
};
