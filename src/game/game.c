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
#include "entities.h"

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
    MIXIN_STRUCT(EntitiesEntity, entity);
    uint16_t bulletThrottle;
    uint16_t deadCounter;
} Player;

typedef struct {
    float xOffset;
    float yOffset;
} PlayerCollisionExplosionOptions;

static PlatformSound* music;
static PlatformSound* shipBulletSound;
static PlatformSound* enemyBulletSound;
static PlatformSound* explosionSound;
static PlatformSound* enemyHit;

static Player ship = { .sprite = &sprites_ship };
static EntitiesList smallEnemies;
static EntitiesList mediumEnemies;
static EntitiesList largeEnemies;
static EntitiesList playerBullets;
static EntitiesList enemyBullets;
static EntitiesList explosions;
static EntitiesList textEntities;

static float randomRange(float min, float max) {
    float range = max - min;
    return min + ((float) rand() / (RAND_MAX + 1)) * range;
}

static void firePlayerBullet(float x, float y) {
    if (
        ship.bulletThrottle > 0 || 
        ship.deadCounter > 0
    ) {
        return;
    }

    entities_spawn(&playerBullets, &sprites_playerBullet, &(EntitiesSpawnOptions) {
        .x = x, 
        .y = y, 
        .vy = SHIP_BULLET_VELOCITY
    });
    platform_playSound(shipBulletSound, false);
    ship.bulletThrottle = SHIP_BULLET_THROTTLE;
}

static void fireEnemyBullet(float x, float y) {
    float shipCenterX = ship.position[0] + ship.sprite->panelDims[0] / 2.0f;
    float shipCenterY = ship.position[1] + ship.sprite->panelDims[1] / 2.0f;
    float dx = ship.position[0] - x;
    float dy = ship.position[1] - y;
    float d = sqrtf(dx * dx + dy * dy);

    entities_spawn(&enemyBullets, &sprites_enemyBullet, &(EntitiesSpawnOptions) {
        .x = x,
        .y = y,
        .vx = (dx / d) * ENEMY_BULLET_SPEED,
        .vy = (dy / d) * ENEMY_BULLET_SPEED
    });

    platform_playSound(enemyBulletSound, false);
}

static void updateEntities(EntitiesList* list, float killBuffer) {
    for (uint8_t i = 0; i < list->count; ++i) {
        list->entities[i].position[0] += list->entities[i].velocity[0];
        list->entities[i].position[1] += list->entities[i].velocity[1];

        if (list->entities[i].whiteOut[0]) {
            list->entities[i].whiteOut[0] = 0;
        }

        if (
            list->entities[i].position[0] + list->entities[i].sprite->panelDims[0] + killBuffer < 0 ||
            list->entities[i].position[1] + list->entities[i].sprite->panelDims[1] + killBuffer < 0 ||
            list->entities[i].position[0] - killBuffer > GAME_WIDTH ||
            list->entities[i].position[1] - killBuffer > GAME_HEIGHT
        ) {
            entities_kill(list, i);
        }
    }
}

static bool boxCollision(float min1[2], float max1[2], float min2[2], float max2[2]) {
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

static bool checkPlayerBulletCollision(float bulletMin[2], float bulletMax[2], EntitiesList* enemies, Sprites_CollisionBox* enemyCollisionBox, float explosionXOffset, float explosionYOffset) {
    bool hit = false;
    for (uint8_t j = 0; j < enemies->count; ++j) {
        EntitiesEntity* enemy = enemies->entities + j;
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
                entities_spawn(&explosions, &sprites_explosion, &(EntitiesSpawnOptions) {
                    .x = enemy->position[0] + explosionXOffset, 
                    .y = enemy->position[1] + explosionYOffset
                });
                platform_playSound(explosionSound, false);
                entities_kill(enemies, j);
            } else {
                platform_playSound(enemyHit, false);
                enemy->whiteOut[0] = 1;
            }
        }    
    } 

    return hit;
}

static bool checkPlayerCollision(float playerMin[2], float playerMax[2], EntitiesList* list, Sprites_Sprite* sprite, PlayerCollisionExplosionOptions* opts) {
    bool playerHit = false;
    Sprites_CollisionBox* collisionBox = &sprite->collisionBox;
    for (uint8_t i = 0; i < list->count; ++i) {
        EntitiesEntity* entity = list->entities + i;
        float entityMin[] = {
            entity->position[0] + collisionBox->min[0],
            entity->position[1] + collisionBox->min[1]
        };
        float entityMax[] = {
            entity->position[0] + collisionBox->max[0],
            entity->position[1] + collisionBox->max[1]
        };      
        if (boxCollision(playerMin, playerMax, entityMin, entityMax)) {
            playerHit = true;
            if (opts) {
                entities_spawn(&explosions, &sprites_explosion, &(EntitiesSpawnOptions) {
                    .x = entity->position[0] + opts->xOffset,
                    .y = entity->position[1] + opts->yOffset 
                });     
            }
            entities_kill(list, i);
        }   
    }

    return playerHit;
}

void game_init(void) {
    srand((unsigned int) time(NULL));

    music = platform_loadSound("assets/audio/music.wav");
    shipBulletSound = platform_loadSound("assets/audio/Laser_002.wav");
    enemyBulletSound = platform_loadSound("assets/audio/Hit_Hurt2.wav");
    explosionSound = platform_loadSound("assets/audio/Explode1.wav");
    enemyHit = platform_loadSound("assets/audio/Jump1.wav");

    platform_playSound(music, true);

    ship.position = sprites_ship.positions;
    ship.currentSpritePanel = sprites_ship.currentSpritePanels;
    ship.position[0] = GAME_WIDTH / 2 - ship.sprite->panelDims[0] / 2;
    ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[0] * 3.0f;

    entities_fromText(&textEntities, &sprites_text, GAME_WIDTH / 2.0f - 80.0f, 20.0f, "space-shooter.c", 0.5f);

    renderer_init(GAME_WIDTH, GAME_HEIGHT);

    renderer_loadTexture("assets/sprites/ship.png", &sprites_ship.texture);
    renderer_loadTexture("assets/sprites/enemy-small.png", &sprites_smallEnemy.texture);
    renderer_loadTexture("assets/sprites/enemy-medium.png", &sprites_mediumEnemy.texture);
    renderer_loadTexture("assets/sprites/enemy-big.png", &sprites_largeEnemy.texture);
    renderer_loadTexture("assets/sprites/explosion.png", &sprites_explosion.texture);
    renderer_loadTexture("assets/sprites/pixelspritefont32.png", &sprites_text.texture);

    GLuint bulletTexture; // Shared between player and enemy bullets.
    renderer_loadTexture("assets/sprites/laser-bolts.png", &bulletTexture);
    sprites_playerBullet.texture = bulletTexture;
    sprites_enemyBullet.texture = bulletTexture;

    entities_setAnimation(&ship.entity, SPRITES_SHIP_CENTER);
}

static int tick = 0;
void game_update(void) {

    if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_SPAWN_PROBABILITY) {
        entities_spawn(&smallEnemies, &sprites_smallEnemy, &(EntitiesSpawnOptions) {
            .x = randomRange(0.0f, 1.0f) * (GAME_WIDTH - sprites_smallEnemy.panelDims[0]), 
            .y = -sprites_smallEnemy.panelDims[1], 
            .vy = SMALL_ENEMY_VELOCITY,
            .health = SMALL_ENEMY_HEALTH
        }); 
    }

    if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_SPAWN_PROBABILITY) {
        entities_spawn(&mediumEnemies, &sprites_mediumEnemy, &(EntitiesSpawnOptions) {
            .x = randomRange(0.0f, 1.0f) * (GAME_WIDTH - sprites_mediumEnemy.panelDims[0]), 
            .y = -sprites_mediumEnemy.panelDims[1], 
            .vy = MEDIUM_ENEMY_VELOCITY,
            .health = MEDIUM_ENEMY_HEALTH
        });
    }

    if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_SPAWN_PROBABILITY) {
        entities_spawn(&largeEnemies, &sprites_largeEnemy, &(EntitiesSpawnOptions) {
            .x = randomRange(0.0f, 1.0f) * (GAME_WIDTH - sprites_largeEnemy.panelDims[0]), 
            .y = -sprites_largeEnemy.panelDims[1], 
            .vy = LARGE_ENEMY_VELOCITY,
            .health = LARGE_ENEMY_HEALTH
        });
    }

    updateEntities(&smallEnemies, 0.0f);
    updateEntities(&mediumEnemies, 0.0f);
    updateEntities(&largeEnemies, 0.0f);
    updateEntities(&playerBullets, 32.0f);
    updateEntities(&enemyBullets, 32.0f);

    Sprites_CollisionBox* playerBulletCollisionBox = &sprites_playerBullet.collisionBox;
    for (uint8_t i = 0; i < playerBullets.count; ++i) {
        EntitiesEntity* bullet = playerBullets.entities + i;
        float bulletMin[] = {
            bullet->position[0] + playerBulletCollisionBox->min[0],
            bullet->position[1] + playerBulletCollisionBox->min[1]
        };
        float bulletMax[] = {
            bullet->position[0] + playerBulletCollisionBox->max[0],
            bullet->position[1] + playerBulletCollisionBox->max[1]
        };
        
        if (checkPlayerBulletCollision(bulletMin, bulletMax, &smallEnemies, &sprites_smallEnemy.collisionBox, SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET, SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET) ||
            checkPlayerBulletCollision(bulletMin, bulletMax, &mediumEnemies, &sprites_mediumEnemy.collisionBox, SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET, SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET) ||
            checkPlayerBulletCollision(bulletMin, bulletMax, &largeEnemies, &sprites_largeEnemy.collisionBox, SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET, SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET)) {
            entities_kill(&playerBullets, i);
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

        for (uint8_t i = 0; i < smallEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_BULLET_PROBABILITY) {
                fireEnemyBullet(smallEnemies.entities[i].position[0] + SPRITES_SMALL_ENEMY_BULLET_X_OFFSET, smallEnemies.entities[i].position[1] + SPRITES_SMALL_ENEMY_BULLET_Y_OFFSET);
            }
        }

        for (uint8_t i = 0; i < mediumEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_BULLET_PROBABILITY) {
                fireEnemyBullet(mediumEnemies.entities[i].position[0] + SPRITES_MEDIUM_ENEMY_BULLET_X_OFFSET, mediumEnemies.entities[i].position[1] + SPRITES_MEDIUM_ENEMY_BULLET_Y_OFFSET);
            }
        }

        for (uint8_t i = 0; i < largeEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_BULLET_PROBABILITY) {
                fireEnemyBullet(largeEnemies.entities[i].position[0] + SPRITES_LARGE_ENEMY_BULLET_X_OFFSET, largeEnemies.entities[i].position[1] + SPRITES_LARGE_ENEMY_BULLET_Y_OFFSET);
            }
        }

        Sprites_CollisionBox* shipCollisionBox = &sprites_ship.collisionBox;
        float shipMin[] = {
            ship.position[0] + shipCollisionBox->min[0],
            ship.position[1] + shipCollisionBox->min[1]
        };
        float shipMax[] = {
            ship.position[0] + shipCollisionBox->max[0],
            ship.position[1] + shipCollisionBox->max[1]
        };


        bool bulletHit = checkPlayerCollision(shipMin, shipMax, &enemyBullets, &sprites_enemyBullet, NULL);
        bool smallEnemyHit = checkPlayerCollision(shipMin, shipMax, &smallEnemies, &sprites_smallEnemy, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool mediumEnemyHit = checkPlayerCollision(shipMin, shipMax, &mediumEnemies, &sprites_mediumEnemy, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool largeEnemyHit = checkPlayerCollision(shipMin, shipMax, &largeEnemies, &sprites_largeEnemy, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET
        });

        if (bulletHit || smallEnemyHit || mediumEnemyHit || largeEnemyHit) {
            entities_spawn(&explosions, &sprites_explosion, &(EntitiesSpawnOptions) {
                .x = ship.position[0] + SPRITES_SHIP_EXPLOSION_X_OFFSET,
                .y = ship.position[1] + SPRITES_SHIP_EXPLOSION_Y_OFFSET 
            });

            platform_playSound(explosionSound, false);
            ship.position[0] = GAME_WIDTH / 2 - ship.sprite->panelDims[0] / 2;
            ship.position[1] = GAME_HEIGHT - ship.sprite->panelDims[0] * 3.0f;
            ship.deadCounter = SHIP_DEAD_COUNTER;
                    
        }
    }

    if (tick == 0) {
        uint8_t count = ship.sprite->animations[ship.currentAnimation].numFrames;
        ship.animationTick = (ship.animationTick + 1) % count;
        entities_updateAnimationPanel(&ship.entity);

        entities_updateAnimations(&playerBullets);  
        entities_updateAnimations(&smallEnemies);
        entities_updateAnimations(&mediumEnemies);
        entities_updateAnimations(&largeEnemies);
        entities_updateAnimations(&enemyBullets);  
        entities_updateAnimations(&explosions);  

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
        entities_setAnimation(&ship.entity, SPRITES_SHIP_LEFT);
    } else if (inputKeys->right) {
        ship.velocity[0] = SHIP_VELOCITY;
        entities_setAnimation(&ship.entity, SPRITES_SHIP_RIGHT);
    } else {
        ship.velocity[0] = 0.0f;
        entities_setAnimation(&ship.entity, SPRITES_SHIP_CENTER);
    }

    if (inputKeys->up) {
        ship.velocity[1] = -SHIP_VELOCITY;
    } else if (inputKeys->down) {
        ship.velocity[1] = SHIP_VELOCITY;
    } else {
        ship.velocity[1] = 0.0f;
    }

    if (inputKeys->space) {
        firePlayerBullet(ship.position[0] + SPRITES_SHIP_BULLET_X_OFFSET, ship.position[1] + SPRITES_SHIP_BULLET_Y_OFFSET);
    }
}

void game_controller(GameController* controllerInput) {
    ship.velocity[0] = SHIP_VELOCITY * controllerInput->leftStickX;
    ship.velocity[1] = -SHIP_VELOCITY * controllerInput->leftStickY;

    if (ship.velocity[0] < -1.0f) {
        entities_setAnimation(&ship.entity, SPRITES_SHIP_LEFT);
    } else if (ship.velocity[0] < 0.0f) {
        entities_setAnimation(&ship.entity, SPRITES_SHIP_CENTER_LEFT);
    } else if (ship.velocity[0] > 1.0f) {
        entities_setAnimation(&ship.entity, SPRITES_SHIP_RIGHT);
    } else if (ship.velocity[0] > 0.0f) {
        entities_setAnimation(&ship.entity, SPRITES_SHIP_CENTER_RIGHT);
    } else {
        entities_setAnimation(&ship.entity, SPRITES_SHIP_CENTER);
    }

    if (controllerInput->aButton) {
        firePlayerBullet(ship.position[0] + SPRITES_SHIP_BULLET_X_OFFSET, ship.position[1] + SPRITES_SHIP_BULLET_Y_OFFSET);
    }
}

void game_draw(void) {
    renderer_beforeFrame();

    if (ship.deadCounter == 0) {
        renderer_draw((Renderer_RenderList *) &sprites_ship, 1);
    }

    renderer_draw((Renderer_RenderList *) &sprites_text, textEntities.count);

    renderer_draw(&sprites_explosion.renderList, explosions.count);
    renderer_draw(&sprites_smallEnemy.renderList, smallEnemies.count);
    renderer_draw(&sprites_mediumEnemy.renderList, mediumEnemies.count);
    renderer_draw(&sprites_largeEnemy.renderList, largeEnemies.count);
    renderer_draw(&sprites_enemyBullet.renderList, enemyBullets.count);
    renderer_draw(&sprites_playerBullet.renderList, playerBullets.count);
}
