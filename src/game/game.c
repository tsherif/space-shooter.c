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
#include "utils.h"
#include "renderer.h"
#include "sprites.h"

#define GAME_WIDTH 320
#define GAME_HEIGHT 180

#define SHIP_VELOCITY 0.5f
#define SHIP_BULLET_VELOCITY (-1.5f)
#define SHIP_BULLET_THROTTLE 20
#define SHIP_DEAD_COUNTER 500

#define SMALL_ENEMY_VELOCITY 0.2f
#define SMALL_ENEMY_SPAWN_PROBABILITY 0.001f
#define SMALL_ENEMY_BULLET_PROBABILITY 0.002f
#define SMALL_ENEMY_HEALTH 1

#define MEDIUM_ENEMY_VELOCITY 0.1f
#define MEDIUM_ENEMY_SPAWN_PROBABILITY 0.0004f
#define MEDIUM_ENEMY_BULLET_PROBABILITY 0.005f
#define MEDIUM_ENEMY_HEALTH 3

#define LARGE_ENEMY_VELOCITY 0.04f
#define LARGE_ENEMY_SPAWN_PROBABILITY 0.0001f
#define LARGE_ENEMY_BULLET_PROBABILITY 0.02f
#define LARGE_ENEMY_HEALTH 5

#define ENEMY_BULLET_SPEED 0.3f

#define COLLISION_SCALE 0.7f

typedef struct {
    float* position;           // vec2 pointer into Renderer_RenderList arrays
    float* currentSpritePanel; // vec2 pointer into Renderer_RenderList arrays
    uint8_t* whiteOut;         // pointer into Renderer_RenderList arrays
    float* scale;              // pointer into Renderer_RenderList arrays
    float velocity[2];
    uint8_t currentAnimation;
    uint8_t animationTick;
    Sprites_Sprite* sprite;
    uint8_t health;
} Entity;

typedef struct {
    INLINE_STRUCT(Entity, entity);
    uint16_t bulletThrottle;
    uint16_t deadCounter;
} Player;

typedef struct {
    Entity entities[RENDERER_DRAWLIST_MAX];
    uint8_t count;
} EntityList;

static PlatformSound* music;
static PlatformSound* shipBulletSound;
static PlatformSound* enemyBulletSound;
static PlatformSound* explosionSound;
static PlatformSound* enemyHit;

static Player ship = { .sprite = &sprites_shipSprite };
static EntityList smallEnemies;
static EntityList mediumEnemies;
static EntityList largeEnemies;
static EntityList playerBullets;
static EntityList enemyBullets;
static EntityList explosions;
static EntityList textEntities;


static float shipBulletOffset[2];
static float shipExplosionOffset[2];
static float smallEnemyBulletOffset[2];
static float smallEnemyExplosionOffset[2];
static float mediumEnemyBulletOffset[2];
static float mediumEnemyExplosionOffset[2];
static float largeEnemyBulletOffset[2];
static float largeEnemyExplosionOffset[2];

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

static void spawnEnemy(EntityList* list, Sprites_Sprite* sprite, float velocity, uint8_t health) {
    if (list->count == RENDERER_DRAWLIST_MAX) {
        return;
    }

    Entity* newEnemy = list->entities + list->count;

    newEnemy->position = sprite->positions + list->count * 2;
    newEnemy->currentSpritePanel = sprite->currentSpritePanels + list->count * 2;
    newEnemy->whiteOut = sprite->whiteOut + list->count;

    newEnemy->position[0] = randomRange(0.0f, 1.0f) * (GAME_WIDTH - sprite->panelDims[0]);
    newEnemy->position[1] = -sprite->panelDims[1];
    newEnemy->velocity[0] = 0.0f;
    newEnemy->velocity[1] = velocity;
    newEnemy->sprite = sprite;
    newEnemy->currentAnimation = 0;
    newEnemy->animationTick = 0;
    newEnemy->health = health;

    updateAnimationPanel(newEnemy);

    ++list->count;
}

static void spawnExplosion(float x, float y) {
    if (explosions.count == RENDERER_DRAWLIST_MAX) {
        return;
    }

    Entity* newExplosion = explosions.entities + explosions.count;

    newExplosion->position = sprites_explosionSprite.positions + explosions.count * 2;
    newExplosion->currentSpritePanel = sprites_explosionSprite.currentSpritePanels + explosions.count * 2;
    newExplosion->whiteOut = sprites_explosionSprite.whiteOut + explosions.count;

    newExplosion->position[0] = x;
    newExplosion->position[1] = y;
    newExplosion->velocity[0] = 0.0f;
    newExplosion->velocity[1] = 0.0f;
    newExplosion->sprite = &sprites_explosionSprite;
    newExplosion->currentAnimation = 0;
    newExplosion->animationTick = 0;

    updateAnimationPanel(newExplosion);

    ++explosions.count;
}

static void firePlayerBullet(float x, float y) {
    if (
        playerBullets.count == RENDERER_DRAWLIST_MAX || 
        ship.bulletThrottle > 0 || 
        ship.deadCounter > 0
    ) {
        return;
    }

    Entity* bullet = playerBullets.entities + playerBullets.count;

    bullet->position = sprites_playerBulletSprite.positions + playerBullets.count * 2;
    bullet->currentSpritePanel = sprites_playerBulletSprite.currentSpritePanels + playerBullets.count * 2;
    bullet->whiteOut = sprites_playerBulletSprite.whiteOut + playerBullets.count;

    bullet->position[0] = x;
    bullet->position[1] = y;
    bullet->velocity[0] = 0.0f;
    bullet->velocity[1] = SHIP_BULLET_VELOCITY;
    bullet->sprite = &sprites_playerBulletSprite;
    bullet->animationTick = 0;
    bullet->currentAnimation = 0;

    updateAnimationPanel(bullet);
    platform_playSound(shipBulletSound, false);

    ++playerBullets.count;
}

static void fireEnemyBullet(float x, float y) {
    if (enemyBullets.count == RENDERER_DRAWLIST_MAX) {
        return;
    }

    Entity* bullet = enemyBullets.entities + enemyBullets.count;

    bullet->position = sprites_enemyBulletSprite.positions + enemyBullets.count * 2;
    bullet->currentSpritePanel = sprites_enemyBulletSprite.currentSpritePanels + enemyBullets.count * 2;
    bullet->whiteOut = sprites_enemyBulletSprite.whiteOut + enemyBullets.count;

    bullet->position[0] = x;
    bullet->position[1] = y;
    bullet->sprite = &sprites_enemyBulletSprite;
    bullet->animationTick = 0;

    float shipCenterX = ship.position[0] + ship.sprite->panelDims[0] / 2.0f;
    float shipCenterY = ship.position[1] + ship.sprite->panelDims[1] / 2.0f;
    float dx = ship.position[0] - x;
    float dy = ship.position[1] - y;
    float d = sqrtf(dx * dx + dy * dy);

    bullet->velocity[0] = (dx / d) * ENEMY_BULLET_SPEED;
    bullet->velocity[1] = (dy / d) * ENEMY_BULLET_SPEED;
    bullet->currentAnimation = 0;

    platform_playSound(enemyBulletSound, false);
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
    uint8_t* whiteOutPointer = list->entities[i].whiteOut;

    positionPointer[0] = list->entities[last].position[0];
    positionPointer[1] = list->entities[last].position[1];
    currentSpritePanelPointer[0] = list->entities[last].currentSpritePanel[0];
    currentSpritePanelPointer[1] = list->entities[last].currentSpritePanel[1];
    whiteOutPointer[0] = list->entities[last].whiteOut[0];

    list->entities[i] = list->entities[last];

    list->entities[i].position = positionPointer;
    list->entities[i].currentSpritePanel = currentSpritePanelPointer;
    list->entities[i].whiteOut = whiteOutPointer;

    --list->count;
}

static void updateEntityPositions(EntityList* list, float killBuffer) {
    for (uint8_t i = 0; i < list->count; ++i) {
        list->entities[i].position[0] += list->entities[i].velocity[0];
        list->entities[i].position[1] += list->entities[i].velocity[1];

        // TODO(Tarek): Don't love that this is happening here.
        if (list->entities[i].whiteOut[0]) {
            list->entities[i].whiteOut[0] = 0;
        }

        if (
            list->entities[i].position[0] + list->entities[i].sprite->panelDims[0] + killBuffer < 0 ||
            list->entities[i].position[1] + list->entities[i].sprite->panelDims[1] + killBuffer < 0 ||
            list->entities[i].position[0] - killBuffer > GAME_WIDTH ||
            list->entities[i].position[1] - killBuffer > GAME_HEIGHT
        ) {
            killEntity(list, i);
        }
    }
}

static void updateEntityAnimations(EntityList* list) {
    for (uint8_t i = 0; i < list->count; ++i) {
        uint8_t numFrames = list->entities[i].sprite->animations[list->entities[i].currentAnimation].numFrames;
        ++list->entities[i].animationTick;
        if (list->entities[i].animationTick == numFrames) {
            Sprites_AnimationEndBehavior endBehavior = list->entities[i].sprite->animations[list->entities[i].currentAnimation].endBehavior;

            if (endBehavior == SPRITES_ANIMATION_END_KILL) {
                killEntity(list, i);
                continue;
            } else {
                list->entities[i].animationTick = 0; 
            }
        }

        updateAnimationPanel(&list->entities[i]);
    }
}

static int8_t charToAnimationIndex(char c) {
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

    uint8_t i = 0; 
    while (SPRITES_PUNCTUATION[i]) {
        if (c == SPRITES_PUNCTUATION[i]) {
            return i + 36;
        }
        ++i;
    }

    return -1;
}

static void textToEntities(float x, float y, const char* text, float scale) {
    uint8_t i = 0;
    uint8_t start = textEntities.count;

    while (text[i] && i + start < RENDERER_DRAWLIST_MAX) {
        int8_t animationIndex = charToAnimationIndex(text[i]);

        if (animationIndex < 0) {
            ++i;
            continue;
        }

        Entity* letter = textEntities.entities + i + start;

        letter->position = sprites_textSprite.positions + textEntities.count * 2;
        letter->currentSpritePanel = sprites_textSprite.currentSpritePanels + textEntities.count * 2;
        letter->whiteOut = sprites_textSprite.whiteOut + textEntities.count;
        letter->scale = sprites_textSprite.scale + textEntities.count;

        letter->position[0] = x + i * sprites_textSprite.panelDims[0] * scale * SPRITES_TEXT_SPACING_SCALE;
        letter->position[1] = y;
        letter->velocity[0] = 0.0f;
        letter->velocity[1] = 0.0f;
        letter->scale[0] = scale;
        letter->sprite = &sprites_textSprite;
        letter->currentAnimation = animationIndex;
        letter->animationTick = 0;

        updateAnimationPanel(letter);
        
        ++textEntities.count;
        ++i;
    }

}

void game_init(void) {
    srand((unsigned int) time(NULL));

    music = platform_loadSound("assets/audio/music.wav");
    shipBulletSound = platform_loadSound("assets/audio/Laser_002.wav");
    enemyBulletSound = platform_loadSound("assets/audio/Hit_Hurt2.wav");
    explosionSound = platform_loadSound("assets/audio/Explode1.wav");
    enemyHit = platform_loadSound("assets/audio/Jump1.wav");

    platform_playSound(music, true);

    shipBulletOffset[0] = (sprites_shipSprite.panelDims[0] - sprites_playerBulletSprite.panelDims[0]) / 2;
    shipBulletOffset[1] =  -sprites_playerBulletSprite.panelDims[1];
    shipExplosionOffset[0] = (sprites_shipSprite.panelDims[0] - sprites_explosionSprite.panelDims[0]) / 2;
    shipExplosionOffset[1] =  (sprites_shipSprite.panelDims[1] - sprites_explosionSprite.panelDims[1]) / 2;

    smallEnemyBulletOffset[0] = (sprites_smallEnemySprite.panelDims[0] - sprites_enemyBulletSprite.panelDims[0]) / 2;
    smallEnemyBulletOffset[1] =  (sprites_smallEnemySprite.panelDims[1] - sprites_enemyBulletSprite.panelDims[1]) / 2;
    smallEnemyExplosionOffset[0] = (sprites_smallEnemySprite.panelDims[0] - sprites_explosionSprite.panelDims[0]) / 2;
    smallEnemyExplosionOffset[1] =  (sprites_smallEnemySprite.panelDims[1] - sprites_explosionSprite.panelDims[1]) / 2;

    mediumEnemyBulletOffset[0] = (sprites_mediumEnemySprite.panelDims[0] - sprites_enemyBulletSprite.panelDims[0]) / 2;
    mediumEnemyBulletOffset[1] =  (sprites_mediumEnemySprite.panelDims[1] - sprites_enemyBulletSprite.panelDims[1]) / 2;
    mediumEnemyExplosionOffset[0] = (sprites_mediumEnemySprite.panelDims[0] - sprites_explosionSprite.panelDims[0]) / 2;
    mediumEnemyExplosionOffset[1] =  (sprites_mediumEnemySprite.panelDims[1] - sprites_explosionSprite.panelDims[1]) / 2;


    largeEnemyBulletOffset[0] = (sprites_largeEnemySprite.panelDims[0] - sprites_enemyBulletSprite.panelDims[0]) / 2;
    largeEnemyBulletOffset[1] =  (sprites_largeEnemySprite.panelDims[1] - sprites_enemyBulletSprite.panelDims[1]) / 2;
    largeEnemyExplosionOffset[0] = (sprites_largeEnemySprite.panelDims[0] - sprites_explosionSprite.panelDims[0]) / 2;
    largeEnemyExplosionOffset[1] =  (sprites_largeEnemySprite.panelDims[1] - sprites_explosionSprite.panelDims[1]) / 2;

    ship.position = sprites_shipSprite.positions;
    ship.currentSpritePanel = sprites_shipSprite.currentSpritePanels;
    ship.position[0] = GAME_WIDTH / 2 - ship.sprite->panelDims[0] / 2;
    ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[0] * 3.0f;

    textToEntities(GAME_WIDTH / 2.0f - 80.0f, 20.0f, "space-shooter.c", 0.5f);

    renderer_init(GAME_WIDTH, GAME_HEIGHT);

    renderer_loadTexture("assets/sprites/ship.png", &sprites_shipSprite.texture);
    renderer_loadTexture("assets/sprites/enemy-small.png", &sprites_smallEnemySprite.texture);
    renderer_loadTexture("assets/sprites/enemy-medium.png", &sprites_mediumEnemySprite.texture);
    renderer_loadTexture("assets/sprites/enemy-big.png", &sprites_largeEnemySprite.texture);
    renderer_loadTexture("assets/sprites/explosion.png", &sprites_explosionSprite.texture);
    renderer_loadTexture("assets/sprites/pixelspritefont32.png", &sprites_textSprite.texture);

    GLuint bulletTexture; // Shared between player and enemy bullets.
    renderer_loadTexture("assets/sprites/laser-bolts.png", &bulletTexture);
    sprites_playerBulletSprite.texture = bulletTexture;
    sprites_enemyBulletSprite.texture = bulletTexture;

    setEntityAnimation(&ship.entity, SPRITES_SHIP_CENTER);
}

bool boxCollision(float min1[2], float max1[2], float min2[2], float max2[2]) {
     float correctionFactor = (1.0 - COLLISION_SCALE) * 0.5;
     float xCorrection1 = (max1[0] - min1[0]) * correctionFactor;
     float yCorrection1 = (max1[1] - min1[1]) * correctionFactor;
     float xCorrection2 = (max2[0] - min2[0]) * correctionFactor;
     float yCorrection2 = (max2[1] - min2[1]) * correctionFactor;
 
    if (min1[0] + xCorrection1 > max2[0] - xCorrection2) {
        return false;
    }

    if (min2[0] + xCorrection2 > max1[0] - xCorrection1) {
        return false;
    }

    if (min1[1] + yCorrection1 > max2[1] - yCorrection2) {
        return false;
    }

    if (min2[1] + yCorrection2 > max1[1] - yCorrection1) {
        return false;
    }

    return true;
}

bool checkBulletCollision(float bulletMin[2], float bulletMax[2], EntityList* enemies, Sprites_CollisionBox* enemyCollisionBox, float* offset) {
    bool hit = false;
    for (uint8_t j = 0; j < enemies->count; ++j) {
        Entity* enemy = enemies->entities + j;
        float enemyMin[] = {
            enemy->position[0] + enemyCollisionBox->min[0],
            enemy->position[1] + enemyCollisionBox->min[1]
        };
        float enemyMax[] = {
            enemy->position[0] + enemyCollisionBox->max[0],
            enemy->position[1] + enemyCollisionBox->max[1]
        };
        
        if (boxCollision(bulletMin, bulletMax, enemyMin, enemyMax)) {
            hit = true;
            --enemy->health;
            if (enemy->health == 0) {
                spawnExplosion(enemy->position[0] + offset[0], enemy->position[1] + offset[1]);
                platform_playSound(explosionSound, false);
                killEntity(enemies, j);
            } else {
                platform_playSound(enemyHit, false);
                enemy->whiteOut[0] = 1;
            }
        }    
    } 

    return hit;
}
 
static int tick = 0;
void game_update(void) {

    if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_SPAWN_PROBABILITY) {
        spawnEnemy(&smallEnemies, &sprites_smallEnemySprite, SMALL_ENEMY_VELOCITY, SMALL_ENEMY_HEALTH); 
    }

    if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_SPAWN_PROBABILITY) {
        spawnEnemy(&mediumEnemies, &sprites_mediumEnemySprite, MEDIUM_ENEMY_VELOCITY, MEDIUM_ENEMY_HEALTH); 
    }

    if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_SPAWN_PROBABILITY) {
        spawnEnemy(&largeEnemies, &sprites_largeEnemySprite, LARGE_ENEMY_VELOCITY, LARGE_ENEMY_HEALTH); 
    }

    updateEntityPositions(&smallEnemies, 0.0f);
    updateEntityPositions(&mediumEnemies, 0.0f);
    updateEntityPositions(&largeEnemies, 0.0f);
    updateEntityPositions(&playerBullets, 32.0f);
    updateEntityPositions(&enemyBullets, 32.0f);

    Sprites_CollisionBox* playerBulletCollisionBox = &sprites_playerBulletSprite.collisionBox;
    for (uint8_t i = 0; i < playerBullets.count; ++i) {
        Entity* bullet = playerBullets.entities + i;
        float bulletMin[] = {
            bullet->position[0] + playerBulletCollisionBox->min[0],
            bullet->position[1] + playerBulletCollisionBox->min[1]
        };
        float bulletMax[] = {
            bullet->position[0] + playerBulletCollisionBox->max[0],
            bullet->position[1] + playerBulletCollisionBox->max[1]
        };
        
        if (checkBulletCollision(bulletMin, bulletMax, &smallEnemies, &sprites_smallEnemySprite.collisionBox, smallEnemyExplosionOffset) ||
            checkBulletCollision(bulletMin, bulletMax, &mediumEnemies, &sprites_mediumEnemySprite.collisionBox, mediumEnemyExplosionOffset) ||
            checkBulletCollision(bulletMin, bulletMax, &largeEnemies, &sprites_largeEnemySprite.collisionBox, largeEnemyExplosionOffset)) {
            killEntity(&playerBullets, i);
        }  
    }

    if (ship.bulletThrottle > 0) {
        --ship.bulletThrottle; 
    }

    if (ship.deadCounter > 0) {
        --ship.deadCounter;
    } else {
        ship.position[0] += ship.velocity[0];
        ship.position[1] += ship.velocity[1];

        if (ship.position[0] < 0.0f) {
            ship.position[0] = 0.0f;
        }

        if (ship.position[0] + ship.sprite->panelDims[0] > GAME_WIDTH) {
            ship.position[0] = GAME_WIDTH - ship.sprite->panelDims[0];
        }

        if (ship.position[1] < 0.0f) {
            ship.position[1] = 0.0f;
        }

        if (ship.position[1] + ship.sprite->panelDims[1] > GAME_HEIGHT) {
            ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[1];
        }

        Sprites_CollisionBox* shipCollisionBox = &sprites_shipSprite.collisionBox;
        float shipMin[] = {
            ship.position[0] + shipCollisionBox->min[0],
            ship.position[1] + shipCollisionBox->min[1]
        };
        float shipMax[] = {
            ship.position[0] + shipCollisionBox->max[0],
            ship.position[1] + shipCollisionBox->max[1]
        };

        Sprites_CollisionBox* enemyBulletCollisionBox = &sprites_enemyBulletSprite.collisionBox;
        for (uint8_t i = 0; i < enemyBullets.count; ++i) {
            Entity* bullet = enemyBullets.entities + i;
            float bulletMin[] = {
                bullet->position[0] + enemyBulletCollisionBox->min[0],
                bullet->position[1] + enemyBulletCollisionBox->min[1]
            };
            float bulletMax[] = {
                bullet->position[0] + enemyBulletCollisionBox->max[0],
                bullet->position[1] + enemyBulletCollisionBox->max[1]
            };      
            if (boxCollision(shipMin, shipMax, bulletMin, bulletMax)) {
                spawnExplosion(ship.position[0] + shipExplosionOffset[0], ship.position[1] + shipExplosionOffset[1]);
                platform_playSound(explosionSound, false);
                ship.position[0] = GAME_WIDTH / 2 - ship.sprite->panelDims[0] / 2;
                ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[0] * 3.0f;
                ship.deadCounter = SHIP_DEAD_COUNTER;
                killEntity(&enemyBullets, i);
                break;
            }   
        }

        if (ship.deadCounter == 0) {
            Sprites_CollisionBox* smallEnemyCollisionBox = &sprites_smallEnemySprite.collisionBox;
            for (uint8_t i = 0; i < smallEnemies.count; ++i) {
                Entity* enemy = smallEnemies.entities + i;
                float enemyMin[] = {
                    enemy->position[0] + smallEnemyCollisionBox->min[0],
                    enemy->position[1] + smallEnemyCollisionBox->min[1]
                };
                float enemyMax[] = {
                    enemy->position[0] + smallEnemyCollisionBox->max[0],
                    enemy->position[1] + smallEnemyCollisionBox->max[1]
                };      
                if (boxCollision(shipMin, shipMax, enemyMin, enemyMax)) {
                    spawnExplosion(ship.position[0] + shipExplosionOffset[0], ship.position[1] + shipExplosionOffset[1]);
                    spawnExplosion(enemy->position[0] + smallEnemyExplosionOffset[0], enemy->position[1] + smallEnemyExplosionOffset[1]);
                    platform_playSound(explosionSound, false);
                    ship.position[0] = GAME_WIDTH / 2 - ship.sprite->panelDims[0] / 2;
                    ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[0] * 3.0f;
                    ship.deadCounter = SHIP_DEAD_COUNTER;
                    killEntity(&smallEnemies, i);
                    break;
                }   
            }
        }

        if (ship.deadCounter == 0) {
            Sprites_CollisionBox* mediumEnemyCollisionBox = &sprites_mediumEnemySprite.collisionBox;
            for (uint8_t i = 0; i < mediumEnemies.count; ++i) {
                Entity* enemy = mediumEnemies.entities + i;
                float enemyMin[] = {
                    enemy->position[0] + mediumEnemyCollisionBox->min[0],
                    enemy->position[1] + mediumEnemyCollisionBox->min[1]
                };
                float enemyMax[] = {
                    enemy->position[0] + mediumEnemyCollisionBox->max[0],
                    enemy->position[1] + mediumEnemyCollisionBox->max[1]
                };      
                if (boxCollision(shipMin, shipMax, enemyMin, enemyMax)) {
                    spawnExplosion(ship.position[0] + shipExplosionOffset[0], ship.position[1] + shipExplosionOffset[1]);
                    spawnExplosion(enemy->position[0] + mediumEnemyExplosionOffset[0], enemy->position[1] + mediumEnemyExplosionOffset[1]);
                    platform_playSound(explosionSound, false);
                    ship.position[0] = GAME_WIDTH / 2 - ship.sprite->panelDims[0] / 2;
                    ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[0] * 3.0f;
                    ship.deadCounter = SHIP_DEAD_COUNTER;
                    killEntity(&mediumEnemies, i);
                    break;
                }   
            }
        }

        if (ship.deadCounter == 0) {
            Sprites_CollisionBox* largeEnemyCollisionBox = &sprites_largeEnemySprite.collisionBox;
            for (uint8_t i = 0; i < largeEnemies.count; ++i) {
                Entity* enemy = largeEnemies.entities + i;
                float enemyMin[] = {
                    enemy->position[0] + largeEnemyCollisionBox->min[0],
                    enemy->position[1] + largeEnemyCollisionBox->min[1]
                };
                float enemyMax[] = {
                    enemy->position[0] + largeEnemyCollisionBox->max[0],
                    enemy->position[1] + largeEnemyCollisionBox->max[1]
                };      
                if (boxCollision(shipMin, shipMax, enemyMin, enemyMax)) {
                    spawnExplosion(ship.position[0] + shipExplosionOffset[0], ship.position[1] + shipExplosionOffset[1]);
                    spawnExplosion(enemy->position[0] + largeEnemyExplosionOffset[0], enemy->position[1] + largeEnemyExplosionOffset[1]);
                    platform_playSound(explosionSound, false);
                    ship.position[0] = GAME_WIDTH / 2 - ship.sprite->panelDims[0] / 2;
                    ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[0] * 3.0f;
                    ship.deadCounter = SHIP_DEAD_COUNTER;
                    killEntity(&largeEnemies, i);
                    break;
                }   
            }
        }

        if (ship.deadCounter == 0) {
            for (uint8_t i = 0; i < smallEnemies.count; ++i) {
                if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_BULLET_PROBABILITY) {
                    fireEnemyBullet(smallEnemies.entities[i].position[0] + smallEnemyBulletOffset[0], smallEnemies.entities[i].position[1] + smallEnemyBulletOffset[1]);
                }
            }

            for (uint8_t i = 0; i < mediumEnemies.count; ++i) {
                if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_BULLET_PROBABILITY) {
                    fireEnemyBullet(mediumEnemies.entities[i].position[0] + mediumEnemyBulletOffset[0], mediumEnemies.entities[i].position[1] + mediumEnemyBulletOffset[1]);
                }
            }

            for (uint8_t i = 0; i < largeEnemies.count; ++i) {
                if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_BULLET_PROBABILITY) {
                    fireEnemyBullet(largeEnemies.entities[i].position[0] + largeEnemyBulletOffset[0], largeEnemies.entities[i].position[1] + largeEnemyBulletOffset[1]);
                }
            }
        }
    }

    if (tick == 0) {
        uint8_t count = ship.sprite->animations[ship.currentAnimation].numFrames;
        ship.animationTick = (ship.animationTick + 1) % count;
        updateAnimationPanel(&ship.entity);

        updateEntityAnimations(&playerBullets);  
        updateEntityAnimations(&smallEnemies);
        updateEntityAnimations(&mediumEnemies);
        updateEntityAnimations(&largeEnemies);
        updateEntityAnimations(&enemyBullets);  
        updateEntityAnimations(&explosions);  

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
        setEntityAnimation(&ship.entity, SPRITES_SHIP_LEFT);
    } else if (inputKeys->right) {
        ship.velocity[0] = SHIP_VELOCITY;
        setEntityAnimation(&ship.entity, SPRITES_SHIP_RIGHT);
    } else {
        ship.velocity[0] = 0.0f;
        setEntityAnimation(&ship.entity, SPRITES_SHIP_CENTER);
    }

    if (inputKeys->up) {
        ship.velocity[1] = -SHIP_VELOCITY;
    } else if (inputKeys->down) {
        ship.velocity[1] = SHIP_VELOCITY;
    } else {
        ship.velocity[1] = 0.0f;
    }

    if (inputKeys->space) {
        firePlayerBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1]);
        ship.bulletThrottle = SHIP_BULLET_THROTTLE;
    }
}

void game_controller(GameController* controllerInput) {
    ship.velocity[0] = SHIP_VELOCITY * controllerInput->leftStickX;
    ship.velocity[1] = -SHIP_VELOCITY * controllerInput->leftStickY;

    if (ship.velocity[0] < -1.0f) {
        setEntityAnimation(&ship.entity, SPRITES_SHIP_LEFT);
    } else if (ship.velocity[0] < 0.0f) {
        setEntityAnimation(&ship.entity, SPRITES_SHIP_CENTER_LEFT);
    } else if (ship.velocity[0] > 1.0f) {
        setEntityAnimation(&ship.entity, SPRITES_SHIP_RIGHT);
    } else if (ship.velocity[0] > 0.0f) {
        setEntityAnimation(&ship.entity, SPRITES_SHIP_CENTER_RIGHT);
    } else {
        setEntityAnimation(&ship.entity, SPRITES_SHIP_CENTER);
    }

    if (controllerInput->aButton) {
        firePlayerBullet(ship.position[0] + shipBulletOffset[0], ship.position[1] + shipBulletOffset[1]);
        ship.bulletThrottle = SHIP_BULLET_THROTTLE;
    }
}

void game_draw(void) {
    renderer_beforeFrame();

    if (ship.deadCounter == 0) {
        renderer_draw((Renderer_RenderList *) &sprites_shipSprite, 1);
    }

    renderer_draw((Renderer_RenderList *) &sprites_textSprite, textEntities.count);

    renderer_draw(&sprites_explosionSprite.renderList, explosions.count);
    renderer_draw(&sprites_smallEnemySprite.renderList, smallEnemies.count);
    renderer_draw(&sprites_mediumEnemySprite.renderList, mediumEnemies.count);
    renderer_draw(&sprites_largeEnemySprite.renderList, largeEnemies.count);
    renderer_draw(&sprites_enemyBulletSprite.renderList, enemyBullets.count);
    renderer_draw(&sprites_playerBulletSprite.renderList, playerBullets.count);
}
