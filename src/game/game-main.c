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
#include "game-sprites.h"

// Animations
#define SHIP_CENTER       0
#define SHIP_CENTER_LEFT  1
#define SHIP_LEFT         2
#define SHIP_CENTER_RIGHT 3
#define SHIP_RIGHT        4
#define SHIP_VELOCITY 0.5f
#define SHIP_BULLET_VELOCITY (-1.5f)
#define SHIP_BULLET_THROTTLE 20

#define LARGE_ENEMY_VELOCITY 0.2f
#define LARGE_ENEMY_SPAWN_PROBABILITY 0.002f
#define ENEMY_BULLET_SPEED 0.3f
#define ENEMY_BULLET_PROBABILITY 0.002f

#define COLLISION_SCALE 0.8f

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

PlatformSound* music;
PlatformSound* shipBulletSound;
PlatformSound* enemyBulletSound;

static struct {
    uint16_t width;
    uint16_t height;
} game = {
    .width = 300,
    .height = 200
};

static Entity ship = { .sprite = &shipSprite };
static EntityList largeEnemies;
static EntityList playerBullets;
static EntityList enemyBullets;
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

    largeEnemies.entities[largeEnemies.count].position[0] = randomRange(0.0f, 1.0f) * (game.width - largeEnemySprite.panelDims[0]);
    largeEnemies.entities[largeEnemies.count].position[1] = -largeEnemySprite.panelDims[1];
    largeEnemies.entities[largeEnemies.count].velocity[0] = 0.0f;
    largeEnemies.entities[largeEnemies.count].velocity[1] = LARGE_ENEMY_VELOCITY;
    largeEnemies.entities[largeEnemies.count].sprite = &largeEnemySprite;
    largeEnemies.entities[largeEnemies.count].currentAnimation = 0;
    largeEnemies.entities[largeEnemies.count].animationTick = 0;

    updateAnimationPanel(&largeEnemies.entities[largeEnemies.count]);

    ++largeEnemies.count;
}

static void firePlayerBullet(float x, float y) {
    if (playerBullets.count == DRAWLIST_MAX) {
        return;
    }

    playerBullets.entities[playerBullets.count].position = playerBulletSprite.positions + playerBullets.count * 2;
    playerBullets.entities[playerBullets.count].currentSpritePanel = playerBulletSprite.currentSpritePanels + playerBullets.count * 2;

    playerBullets.entities[playerBullets.count].position[0] = x;
    playerBullets.entities[playerBullets.count].position[1] = y;
    playerBullets.entities[playerBullets.count].velocity[0] = 0.0f;
    playerBullets.entities[playerBullets.count].velocity[1] = SHIP_BULLET_VELOCITY;
    playerBullets.entities[playerBullets.count].sprite = &playerBulletSprite;
    playerBullets.entities[playerBullets.count].animationTick = 0;
    playerBullets.entities[playerBullets.count].currentAnimation = 0;

    updateAnimationPanel(&playerBullets.entities[playerBullets.count]);
    platform_playSound(shipBulletSound);

    ++playerBullets.count;
}

static void fireEnemyBullet(float x, float y) {
    if (enemyBullets.count == DRAWLIST_MAX) {
        return;
    }

    enemyBullets.entities[enemyBullets.count].position = enemyBulletSprite.positions + enemyBullets.count * 2;
    enemyBullets.entities[enemyBullets.count].currentSpritePanel = enemyBulletSprite.currentSpritePanels + enemyBullets.count * 2;

    enemyBullets.entities[enemyBullets.count].position[0] = x;
    enemyBullets.entities[enemyBullets.count].position[1] = y;
    enemyBullets.entities[enemyBullets.count].sprite = &enemyBulletSprite;
    enemyBullets.entities[enemyBullets.count].animationTick = 0;

    float shipCenterX = ship.position[0] + ship.sprite->panelDims[0] / 2.0f;
    float shipCenterY = ship.position[1] + ship.sprite->panelDims[1] / 2.0f;

    float dx = ship.position[0] - x;
    float dy = ship.position[1] - y;
    float d = sqrtf(dx * dx + dy * dy);

    enemyBullets.entities[enemyBullets.count].velocity[0] = (dx / d) * ENEMY_BULLET_SPEED;
    enemyBullets.entities[enemyBullets.count].velocity[1] = (dy / d) * ENEMY_BULLET_SPEED;
    enemyBullets.entities[enemyBullets.count].currentAnimation = 0;
    platform_playSound(enemyBulletSound);

    updateAnimationPanel(&enemyBullets.entities[enemyBullets.count]);

    ++enemyBullets.count;
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
            list->entities[i].position[0] + list->entities[i].sprite->panelDims[0] < 0 ||
            list->entities[i].position[1] + list->entities[i].sprite->panelDims[1] < 0 ||
            list->entities[i].position[0] > game.width ||
            list->entities[i].position[1] > game.height
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

    shipBulletOffset[0] = (shipSprite.panelDims[0] - playerBulletSprite.panelDims[0]) / 2;
    shipBulletOffset[1] =  -playerBulletSprite.panelDims[1];

    largeEnemyBulletOffset[0] = (largeEnemySprite.panelDims[0] - enemyBulletSprite.panelDims[0]) / 2;
    largeEnemyBulletOffset[1] =  (largeEnemySprite.panelDims[1] - enemyBulletSprite.panelDims[1]) / 2;

    ship.position = shipSprite.positions;
    ship.currentSpritePanel = shipSprite.currentSpritePanels;
    ship.position[0] = game.width / 2 - ship.sprite->panelDims[0] / 2;
    ship.position[1] = game.height - ship.sprite->panelDims[0] * 3.0f;

    renderer_init(game.width, game.height);

    renderer_loadTexture("assets/img/ship.png", &shipSprite.texture);
    renderer_loadTexture("assets/img/enemy-big.png", &largeEnemySprite.texture);

    GLuint bulletTexture; // Shared between player and enemy bullets.
    renderer_loadTexture("assets/img/laser-bolts.png", &bulletTexture);
    playerBulletSprite.texture = bulletTexture;
    enemyBulletSprite.texture = bulletTexture;

    setEntityAnimation(&ship, SHIP_CENTER);
}

bool boxCollision(float minx1, float miny1, float maxx1, float maxy1, float minx2, float miny2, float maxx2, float maxy2) {
    float correctionFactor = (1.0 - COLLISION_SCALE) * 0.5;
    float xCorrection1 = (maxx1 - minx1) * correctionFactor;
    float yCorrection1 = (maxy1 - miny1) * correctionFactor;
    float xCorrection2 = (maxx2 - minx2) * correctionFactor;
    float yCorrection2 = (maxy2 - miny2) * correctionFactor;

    if (minx1 + xCorrection1 > maxx2 - xCorrection2) {
        return false;
    }

    if (minx2 + xCorrection2 > maxx1 - xCorrection1) {
        return false;
    }

    if (miny1 + yCorrection1 > maxy2 - yCorrection2) {
        return false;
    }

    if (miny2 + yCorrection2 > maxy1 - yCorrection1) {
        return false;
    }

    return true;
}
 
static int tick = 0;
void game_update(void) {
    ship.position[0] += ship.velocity[0];
    ship.position[1] += ship.velocity[1];

    if (ship.position[0] < 0.0f) {
        ship.position[0] = 0.0f;
    }

    if (ship.position[0] + ship.sprite->panelDims[0] > game.width) {
        ship.position[0] = game.width - ship.sprite->panelDims[0];
    }

    if (ship.position[1] < 0.0f) {
        ship.position[1] = 0.0f;
    }

    if (ship.position[1] + ship.sprite->panelDims[1] > game.height) {
        ship.position[1] = game.height - ship.sprite->panelDims[1];
    }

    if (ship.bulletThrottle > 0) {
        --ship.bulletThrottle; 
    }

    spawnLargeEnemy();

    updateEntityPositions(&playerBullets);
    updateEntityPositions(&largeEnemies);
    updateEntityPositions(&enemyBullets);

    for (uint8_t i = 0; i < largeEnemies.count; ++i) {
        if (randomRange(0.0f, 1.0f) < ENEMY_BULLET_PROBABILITY) {
            fireEnemyBullet(largeEnemies.entities[i].position[0] + largeEnemyBulletOffset[0], largeEnemies.entities[i].position[1] + largeEnemyBulletOffset[1]);
        }
    }

    for (uint8_t i = 0; i < playerBullets.count; ++i) {
        Entity* bullet = playerBullets.entities + i;
        float minx1 = bullet->position[0];
        float miny1 = bullet->position[1];
        float maxx1 = minx1 + bullet->sprite->panelDims[0];
        float maxy1 = miny1 + bullet->sprite->panelDims[1];
        for (uint8_t j = 0; j < largeEnemies.count; ++j) {
            Entity* enemy = largeEnemies.entities + j;
            float minx2 = enemy->position[0];
            float miny2 = enemy->position[1];
            float maxx2 = minx2 + enemy->sprite->panelDims[0];
            float maxy2 = miny2 + enemy->sprite->panelDims[1];
            
            if (boxCollision(minx1, miny1, maxx1, maxy1, minx2, miny2, maxx2, maxy2)) {
                killEntity(&playerBullets, i);
                killEntity(&largeEnemies, j);
            }    
        }    
    }



    if (tick == 0) {
        uint8_t count = ship.sprite->animations[ship.currentAnimation].numFrames;
        ship.animationTick = (ship.animationTick + 1) % count;
        updateAnimationPanel(&ship);

        updateEntityAnimations(&playerBullets);  
        updateEntityAnimations(&largeEnemies);
        updateEntityAnimations(&enemyBullets);  

        tick = 20;
    }
    --tick;
}

void game_resize(int width, int height) {
    renderer_resize(width, height);
    game_draw();
}

void game_keyboard(GameKeyboard* inputKeys) {
    if (inputKeys->left) {
        ship.velocity[0] = -SHIP_VELOCITY;
        setEntityAnimation(&ship, SHIP_LEFT);
    } else if (inputKeys->right) {
        ship.velocity[0] = SHIP_VELOCITY;
        setEntityAnimation(&ship, SHIP_RIGHT);
    } else {
        ship.velocity[0] = 0.0f;
        setEntityAnimation(&ship, SHIP_CENTER);
    }

    if (inputKeys->up) {
        ship.velocity[1] = -SHIP_VELOCITY;
    } else if (inputKeys->down) {
        ship.velocity[1] = SHIP_VELOCITY;
    } else {
        ship.velocity[1] = 0.0f;
    }

    if (inputKeys->space && ship.bulletThrottle == 0) {
        firePlayerBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1]);
        ship.bulletThrottle = SHIP_BULLET_THROTTLE;
    }
}

void game_controller(GameController* controllerInput) {
    ship.velocity[0] = SHIP_VELOCITY * controllerInput->leftStickX;
    ship.velocity[1] = -SHIP_VELOCITY * controllerInput->leftStickY;

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
        firePlayerBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1]);
        ship.bulletThrottle = SHIP_BULLET_THROTTLE;
    }
}

void game_draw(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    renderer_draw((RenderList *) &largeEnemySprite, largeEnemies.count);
    renderer_draw((RenderList *) &shipSprite, 1);
    renderer_draw((RenderList *) &enemyBulletSprite, enemyBullets.count);
    renderer_draw((RenderList *) &playerBulletSprite, playerBullets.count);
}
