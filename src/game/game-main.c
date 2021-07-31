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

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "../../lib/simple-opengl-loader.h"
#include "../shared/platform-interface.h"
#include "game-renderer.h"

// Animations
#define SHIP_CENTER       0
#define SHIP_CENTER_LEFT  1
#define SHIP_LEFT         2
#define SHIP_CENTER_RIGHT 3
#define SHIP_RIGHT        4

#define LARGE_ENEMY_VELOCITY 1.0f
#define LARGE_ENEMY_SPAWN_PROBABILITY 0.002f

#define BULLET_VELOCITY (-5.0f)
#define BULLET_THROTTLE_SHIP 20

typedef struct {
    uint8_t frames[32][2];
    uint8_t numFrames;
} Animation;

typedef struct {
    RenderList;
    Animation* animations;
    uint8_t numAnimations;
} Sprite;

typedef struct {
    float* position;           // vec2 pointer into RenderList arrays
    float* currentSpritePanel; // vec2 pointer into RenderList arrays
    float velocity[2];
    uint8_t currentAnimation;
    uint8_t animationTick;
    Sprite* sprite;
    uint16_t bulletThrottle;
} Character;

PlatformSound* music;
PlatformSound* shipBulletSound;

static struct {
    uint16_t width;
    uint16_t height;
} canvas;

static Animation shipAnimations[]  = {
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

static Sprite shipSprite = {
    .panelDims = { 16.0f, 24.0f },
    .sheetDims = { 5.0f, 2.0f },
    .animations = shipAnimations,
    .numAnimations = sizeof(shipAnimations) / sizeof(shipAnimations[0])
};

static Character ship = {
    .sprite = &shipSprite
};

static Animation enemyAnimations[]  = {
    {
        .frames = {{0, 0}, {1, 0}},
        .numFrames = 2
    }
};

static Sprite largeEnemySprite = {
    .panelDims = { 32.0f, 32.0f },
    .sheetDims = { 2.0f, 1.0f },
    .animations = enemyAnimations,
    .numAnimations = sizeof(enemyAnimations) / sizeof(enemyAnimations[0])
};

static Character largeEnemies[DRAWLIST_MAX];
static uint8_t numLargeEnemies;

static Animation bulletAnimations[]  = {
    {
        .frames = {{0, 0}, {1, 0}},
        .numFrames = 2
    },
    {
        .frames = {{0, 1}, {1, 1}},
        .numFrames = 2
    }
};

static Sprite bulletSprite = {
    .panelDims = { 16.0f, 16.0f },
    .sheetDims = { 2.0f, 2.0f },
    .animations = bulletAnimations,
    .numAnimations = sizeof(bulletAnimations) / sizeof(bulletAnimations[0])
};

static Character bullets[DRAWLIST_MAX];
static uint8_t numBullets;
static float shipBulletOffset[2];

static float randomRange(float min, float max) {
    float range = max - min;
    return min + ((float) rand() / (RAND_MAX + 1)) * range;
}

static void updateAnimationPanel(Character* character) {
    uint8_t* panel = character->sprite->animations[character->currentAnimation].frames[character->animationTick];

    character->currentSpritePanel[0] = panel[0];
    character->currentSpritePanel[1] = panel[1];
}

static void setCharacterAnimation(Character* character, uint8_t animation) {
    if (character->currentAnimation == animation) {
        return;
    }

    character->currentAnimation = animation;
    character->animationTick = 0;
    updateAnimationPanel(character);
}

static void spawnLargeEnemy() {
    if (numLargeEnemies == DRAWLIST_MAX) {
        return;
    }

    if (randomRange(0.0f, 1.0f) > LARGE_ENEMY_SPAWN_PROBABILITY) {
        return;
    }

    largeEnemies[numLargeEnemies].position = largeEnemySprite.positions + numLargeEnemies * 2;
    largeEnemies[numLargeEnemies].currentSpritePanel = largeEnemySprite.currentSpritePanels + numLargeEnemies * 2;

    largeEnemies[numLargeEnemies].position[0] = randomRange(0.0f, 1.0f) * (canvas.width - largeEnemySprite.panelDims[0] * SPRITE_SCALE);
    largeEnemies[numLargeEnemies].position[1] = -largeEnemySprite.panelDims[1] * SPRITE_SCALE;
    largeEnemies[numLargeEnemies].velocity[0] = 0.0f;
    largeEnemies[numLargeEnemies].velocity[1] = LARGE_ENEMY_VELOCITY;
    largeEnemies[numLargeEnemies].sprite = &largeEnemySprite;
    largeEnemies[numLargeEnemies].currentAnimation = 0;
    largeEnemies[numLargeEnemies].animationTick = 0;

    updateAnimationPanel(&largeEnemies[numLargeEnemies]);

    ++numLargeEnemies;
}

static void fireBullet(float x, float y) {
    if (numBullets == DRAWLIST_MAX) {
        return;
    }

    bullets[numBullets].position = bulletSprite.positions + numBullets * 2;
    bullets[numBullets].currentSpritePanel = bulletSprite.currentSpritePanels + numBullets * 2;

    bullets[numBullets].position[0] = x;
    bullets[numBullets].position[1] = y;
    bullets[numBullets].velocity[0] = 0.0f;
    bullets[numBullets].velocity[1] = BULLET_VELOCITY;
    bullets[numBullets].sprite = &bulletSprite;
    bullets[numBullets].currentAnimation = 1;
    bullets[numBullets].animationTick = 0;

    updateAnimationPanel(&bullets[numBullets]);
    platform_playSound(shipBulletSound);

    ++numBullets;
}

static void killCharacter(Character* list, uint8_t* count, uint8_t i) {
    if (i >= *count) {
        return;
    }

    uint8_t last = *count - 1;

    float* positionPointer = list[i].position;
    float* currentSpritePanelPointer =  list[i].currentSpritePanel;

    positionPointer[0] = list[last].position[0];
    positionPointer[1] = list[last].position[1];
    currentSpritePanelPointer[0] = list[last].currentSpritePanel[0];
    currentSpritePanelPointer[1] = list[last].currentSpritePanel[1];

    list[i] = list[last];

    list[i].position = positionPointer;
    list[i].currentSpritePanel = currentSpritePanelPointer;

    *count -= 1;
}

static void updateCharacterPositions(Character* list, uint8_t* count) {
    for (uint8_t i = 0; i < *count; ++i) {
        list[i].position[0] += list[i].velocity[0];
        list[i].position[1] += list[i].velocity[1];

        if (list[i].position[1] + list[i].sprite->panelDims[1] * SPRITE_SCALE < 0) {
            killCharacter(list, count, i);
        }
    }
}

static void updateCharacterAnimations(Character* list, uint8_t count) {
    for (uint8_t i = 0; i < count; ++i) {
        uint8_t count = list[i].sprite->animations[list[i].currentAnimation].numFrames;
        list[i].animationTick = (list[i].animationTick + 1) % count;
        updateAnimationPanel(&list[i]);
    }
}

void game_init(void) {
    srand((unsigned int) time(NULL));

    music = platform_loadSound("assets/audio/music.wav");
    shipBulletSound = platform_loadSound("assets/audio/Laser_002.wav");

    platform_playSound(music);

    shipBulletOffset[0] = (shipSprite.panelDims[0] - bulletSprite.panelDims[0]) * SPRITE_SCALE / 2;
    shipBulletOffset[1] =  -bulletSprite.panelDims[1] * SPRITE_SCALE;

    ship.position = shipSprite.positions;
    ship.currentSpritePanel = shipSprite.currentSpritePanels;
    ship.position[0] = canvas.width / 2 - ship.sprite->panelDims[0] * SPRITE_SCALE / 2;
    ship.position[1] = canvas.height - 150.0f;

    renderer_init();

    renderer_loadTexture("assets/img/ship.png", &shipSprite.texture);
    renderer_loadTexture("assets/img/enemy-big.png", &largeEnemySprite.texture);
    renderer_loadTexture("assets/img/laser-bolts.png", &bulletSprite.texture);


    setCharacterAnimation(&ship, SHIP_CENTER);
}

static int tick = 0;
void game_update(void) {
    ship.position[0] += ship.velocity[0];
    ship.position[1] += ship.velocity[1];

    if (ship.position[0] < 0.0f) {
        ship.position[0] = 0.0f;
    }

    if (ship.position[0] + ship.sprite->panelDims[0] * SPRITE_SCALE > canvas.width) {
        ship.position[0] = canvas.width - ship.sprite->panelDims[0] * SPRITE_SCALE;
    }

    if (ship.position[1] < 0.0f) {
        ship.position[1] = 0.0f;
    }

    if (ship.position[1] + ship.sprite->panelDims[1] * SPRITE_SCALE > canvas.height) {
        ship.position[1] = canvas.height - ship.sprite->panelDims[1] * SPRITE_SCALE;
    }

    if (ship.bulletThrottle > 0) {
        --ship.bulletThrottle; 
    }

    spawnLargeEnemy();

    updateCharacterPositions(largeEnemies, &numLargeEnemies);
    updateCharacterPositions(bullets, &numBullets);

    if (tick == 0) {
        uint8_t count = ship.sprite->animations[ship.currentAnimation].numFrames;
        ship.animationTick = (ship.animationTick + 1) % count;
        updateAnimationPanel(&ship);

        updateCharacterAnimations(largeEnemies, numLargeEnemies);
        updateCharacterAnimations(bullets, numBullets);  

        tick = 20;
    }
    --tick;
}

void game_resize(int width, int height) {
    canvas.width = width;
    canvas.height = height;
    renderer_resize(width, height);
    game_draw();
}

void game_keyboard(GameKeyboard* inputKeys) {
    if (inputKeys->left) {
        ship.velocity[0] = -2.0f;
        setCharacterAnimation(&ship, SHIP_LEFT);
    } else if (inputKeys->right) {
        ship.velocity[0] = 2.0f;
        setCharacterAnimation(&ship, SHIP_RIGHT);
    } else {
        ship.velocity[0] = 0.0f;
        setCharacterAnimation(&ship, SHIP_CENTER);
    }

    if (inputKeys->up) {
        ship.velocity[1] = -2.0f;
    } else if (inputKeys->down) {
        ship.velocity[1] = 2.0f;
    } else {
        ship.velocity[1] = 0.0f;
    }

    if (inputKeys->space && ship.bulletThrottle == 0) {
        fireBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1]);
        ship.bulletThrottle = BULLET_THROTTLE_SHIP;
    }
}

void game_controller(GameController* controllerInput) {
    ship.velocity[0] = 2.0f * controllerInput->leftStickX;
    ship.velocity[1] = -2.0f * controllerInput->leftStickY;

    if (ship.velocity[0] < -1.0f) {
        setCharacterAnimation(&ship, SHIP_LEFT);
    } else if (ship.velocity[0] < 0.0f) {
        setCharacterAnimation(&ship, SHIP_CENTER_LEFT);
    } else if (ship.velocity[0] > 1.0f) {
        setCharacterAnimation(&ship, SHIP_RIGHT);
    } else if (ship.velocity[0] > 0.0f) {
        setCharacterAnimation(&ship, SHIP_CENTER_RIGHT);
    } else {
        setCharacterAnimation(&ship, SHIP_CENTER);
    }

    if (controllerInput->aButton && ship.bulletThrottle == 0) {
        fireBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1]);
        ship.bulletThrottle = BULLET_THROTTLE_SHIP;
    }
}

void game_draw(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    renderer_draw((RenderList *) &shipSprite, 1);
    renderer_draw((RenderList *) &largeEnemySprite, numLargeEnemies);
    renderer_draw((RenderList *) &bulletSprite, numBullets);
}
