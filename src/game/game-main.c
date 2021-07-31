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
#include <math.h>
#include "../../lib/simple-opengl-loader.h"
#include "../shared/platform-interface.h"
#include "game-renderer.h"

// Animations
#define SHIP_CENTER       0
#define SHIP_CENTER_LEFT  1
#define SHIP_LEFT         2
#define SHIP_CENTER_RIGHT 3
#define SHIP_RIGHT        4
#define SHIP_BULLET_VELOCITY (-5.0f)
#define SHIP_BULLET_THROTTLE 20

#define LARGE_ENEMY_VELOCITY 1.0f
#define LARGE_ENEMY_SPAWN_PROBABILITY 0.001f
#define ENEMY_BULLET_SPEED 1.0f
#define ENEMY_BULLET_PROBABILITY 0.002f


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
} Entity;

typedef struct {
    Entity entities[DRAWLIST_MAX];
    uint8_t count;
} EntityList;


typedef enum {
    SHIP_BULLET,
    ENEMY_BULLET
} BulletType;


PlatformSound* music;
PlatformSound* shipBulletSound;
PlatformSound* enemyBulletSound;

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

static Entity ship = {
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

static EntityList largeEnemies;
static EntityList bullets;
static float shipBulletOffset[2];
static float largeEnemyBulletOffset[2];

static float randomRange(float min, float max) {
    float range = max - min;
    return min + ((float) rand() / (RAND_MAX + 1)) * range;
}

static void updateAnimationPanel(Entity* character) {
    uint8_t* panel = character->sprite->animations[character->currentAnimation].frames[character->animationTick];

    character->currentSpritePanel[0] = panel[0];
    character->currentSpritePanel[1] = panel[1];
}

static void setEntityAnimation(Entity* character, uint8_t animation) {
    if (character->currentAnimation == animation) {
        return;
    }

    character->currentAnimation = animation;
    character->animationTick = 0;
    updateAnimationPanel(character);
}

static void spawnLargeEnemy() {
    if (largeEnemies.count == DRAWLIST_MAX) {
        return;
    }

    if (randomRange(0.0f, 1.0f) > LARGE_ENEMY_SPAWN_PROBABILITY) {
        return;
    }

    largeEnemies.entities[largeEnemies.count].position = largeEnemySprite.positions + largeEnemies.count * 2;
    largeEnemies.entities[largeEnemies.count].currentSpritePanel = largeEnemySprite.currentSpritePanels + largeEnemies.count * 2;

    largeEnemies.entities[largeEnemies.count].position[0] = randomRange(0.0f, 1.0f) * (canvas.width - largeEnemySprite.panelDims[0] * SPRITE_SCALE);
    largeEnemies.entities[largeEnemies.count].position[1] = -largeEnemySprite.panelDims[1] * SPRITE_SCALE;
    largeEnemies.entities[largeEnemies.count].velocity[0] = 0.0f;
    largeEnemies.entities[largeEnemies.count].velocity[1] = LARGE_ENEMY_VELOCITY;
    largeEnemies.entities[largeEnemies.count].sprite = &largeEnemySprite;
    largeEnemies.entities[largeEnemies.count].currentAnimation = 0;
    largeEnemies.entities[largeEnemies.count].animationTick = 0;

    updateAnimationPanel(&largeEnemies.entities[largeEnemies.count]);

    ++largeEnemies.count;
}

static void fireBullet(float x, float y, BulletType type) {
    if (bullets.count == DRAWLIST_MAX) {
        return;
    }

    bullets.entities[bullets.count].position = bulletSprite.positions + bullets.count * 2;
    bullets.entities[bullets.count].currentSpritePanel = bulletSprite.currentSpritePanels + bullets.count * 2;

    bullets.entities[bullets.count].position[0] = x;
    bullets.entities[bullets.count].position[1] = y;
    bullets.entities[bullets.count].sprite = &bulletSprite;
    bullets.entities[bullets.count].animationTick = 0;

    if (type == SHIP_BULLET) {
        bullets.entities[bullets.count].velocity[0] = 0.0f;
        bullets.entities[bullets.count].velocity[1] = SHIP_BULLET_VELOCITY;
        bullets.entities[bullets.count].currentAnimation = 1;
        platform_playSound(shipBulletSound);
    } else {
        float shipCenterX = ship.position[0] + ship.sprite->panelDims[0] * SPRITE_SCALE / 2.0f;
        float shipCenterY = ship.position[1] + ship.sprite->panelDims[1] * SPRITE_SCALE / 2.0f;

        float dx = ship.position[0] - x;
        float dy = ship.position[1] - y;
        float d = sqrtf(dx * dx + dy * dy);

        bullets.entities[bullets.count].velocity[0] = (dx / d) * ENEMY_BULLET_SPEED;
        bullets.entities[bullets.count].velocity[1] = (dy / d) * ENEMY_BULLET_SPEED;
        bullets.entities[bullets.count].currentAnimation = 0;
        platform_playSound(enemyBulletSound);
    }

    updateAnimationPanel(&bullets.entities[bullets.count]);

    ++bullets.count;
}

static void killEntity(EntityList* list, uint8_t i) {
    if (i >= list->count) {
        return;
    }

    uint8_t last = list->count - 1;

    float* positionPointer = list->entities[i].position;
    float* currentSpritePanelPointer =  list->entities[i].currentSpritePanel;

    positionPointer[0] = list->entities[last].position[0];
    positionPointer[1] = list->entities[last].position[1];
    currentSpritePanelPointer[0] = list->entities[last].currentSpritePanel[0];
    currentSpritePanelPointer[1] = list->entities[last].currentSpritePanel[1];

    list->entities[i] = list->entities[last];

    list->entities[i].position = positionPointer;
    list->entities[i].currentSpritePanel = currentSpritePanelPointer;

    --list->count;
}

static void updateEntityPositions(EntityList* list) {
    for (uint8_t i = 0; i < list->count; ++i) {
        list->entities[i].position[0] += list->entities[i].velocity[0];
        list->entities[i].position[1] += list->entities[i].velocity[1];

        if (
            list->entities[i].position[0] + list->entities[i].sprite->panelDims[0] * SPRITE_SCALE < 0 ||
            list->entities[i].position[1] + list->entities[i].sprite->panelDims[1] * SPRITE_SCALE < 0 ||
            list->entities[i].position[0] > canvas.width ||
            list->entities[i].position[1] > canvas.height
        ) {
            killEntity(list, i);
        }
    }
}

static void updateEntityAnimations(EntityList* list) {
    for (uint8_t i = 0; i < list->count; ++i) {
        uint8_t numFrames = list->entities[i].sprite->animations[list->entities[i].currentAnimation].numFrames;
        list->entities[i].animationTick = (list->entities[i].animationTick + 1) % numFrames;
        updateAnimationPanel(&list->entities[i]);
    }
}

void game_init(void) {
    srand((unsigned int) time(NULL));

    music = platform_loadSound("assets/audio/music.wav");
    shipBulletSound = platform_loadSound("assets/audio/Laser_002.wav");
    enemyBulletSound = platform_loadSound("assets/audio/Hit_Hurt2.wav");

    platform_playSound(music);

    shipBulletOffset[0] = (shipSprite.panelDims[0] - bulletSprite.panelDims[0]) * SPRITE_SCALE / 2;
    shipBulletOffset[1] =  -bulletSprite.panelDims[1] * SPRITE_SCALE;

    largeEnemyBulletOffset[0] = (largeEnemySprite.panelDims[0] - bulletSprite.panelDims[0]) * SPRITE_SCALE / 2;
    largeEnemyBulletOffset[1] =  (largeEnemySprite.panelDims[1] - bulletSprite.panelDims[1]) * SPRITE_SCALE / 2;

    ship.position = shipSprite.positions;
    ship.currentSpritePanel = shipSprite.currentSpritePanels;
    ship.position[0] = canvas.width / 2 - ship.sprite->panelDims[0] * SPRITE_SCALE / 2;
    ship.position[1] = canvas.height - 150.0f;

    renderer_init();

    renderer_loadTexture("assets/img/ship.png", &shipSprite.texture);
    renderer_loadTexture("assets/img/enemy-big.png", &largeEnemySprite.texture);
    renderer_loadTexture("assets/img/laser-bolts.png", &bulletSprite.texture);


    setEntityAnimation(&ship, SHIP_CENTER);
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

    updateEntityPositions(&largeEnemies);
    updateEntityPositions(&bullets);

    for (uint8_t i = 0; i < largeEnemies.count; ++i) {
        if (randomRange(0.0f, 1.0f) < ENEMY_BULLET_PROBABILITY) {
            fireBullet(largeEnemies.entities[i].position[0] + largeEnemyBulletOffset[0], largeEnemies.entities[i].position[1] + largeEnemyBulletOffset[1], ENEMY_BULLET);
        }
    }

    if (tick == 0) {
        uint8_t count = ship.sprite->animations[ship.currentAnimation].numFrames;
        ship.animationTick = (ship.animationTick + 1) % count;
        updateAnimationPanel(&ship);

        updateEntityAnimations(&largeEnemies);
        updateEntityAnimations(&bullets);  

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
        setEntityAnimation(&ship, SHIP_LEFT);
    } else if (inputKeys->right) {
        ship.velocity[0] = 2.0f;
        setEntityAnimation(&ship, SHIP_RIGHT);
    } else {
        ship.velocity[0] = 0.0f;
        setEntityAnimation(&ship, SHIP_CENTER);
    }

    if (inputKeys->up) {
        ship.velocity[1] = -2.0f;
    } else if (inputKeys->down) {
        ship.velocity[1] = 2.0f;
    } else {
        ship.velocity[1] = 0.0f;
    }

    if (inputKeys->space && ship.bulletThrottle == 0) {
        fireBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1], SHIP_BULLET);
        ship.bulletThrottle = SHIP_BULLET_THROTTLE;
    }
}

void game_controller(GameController* controllerInput) {
    ship.velocity[0] = 2.0f * controllerInput->leftStickX;
    ship.velocity[1] = -2.0f * controllerInput->leftStickY;

    if (ship.velocity[0] < -1.0f) {
        setEntityAnimation(&ship, SHIP_LEFT);
    } else if (ship.velocity[0] < 0.0f) {
        setEntityAnimation(&ship, SHIP_CENTER_LEFT);
    } else if (ship.velocity[0] > 1.0f) {
        setEntityAnimation(&ship, SHIP_RIGHT);
    } else if (ship.velocity[0] > 0.0f) {
        setEntityAnimation(&ship, SHIP_CENTER_RIGHT);
    } else {
        setEntityAnimation(&ship, SHIP_CENTER);
    }

    if (controllerInput->aButton && ship.bulletThrottle == 0) {
        fireBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1], SHIP_BULLET);
        ship.bulletThrottle = SHIP_BULLET_THROTTLE;
    }
}

void game_draw(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    renderer_draw((RenderList *) &shipSprite, 1);
    renderer_draw((RenderList *) &largeEnemySprite, largeEnemies.count);
    renderer_draw((RenderList *) &bulletSprite, bullets.count);
}
