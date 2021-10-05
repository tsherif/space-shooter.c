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
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "../../lib/simple-opengl-loader.h"
#include "../shared/buffer.h"
#include "../shared/platform-interface.h"
#include "utils.h"
#include "renderer.h"
#include "sprites.h"
#include "entities.h"
#include "events.h"

#define GAME_WIDTH 320
#define GAME_HEIGHT 180

#define TICK_DURATION 16.6667f

#define SHIP_VELOCITY 0.075f
#define SHIP_BULLET_VELOCITY (-0.2f)
#define SHIP_BULLET_THROTTLE 100.0f
#define SHIP_DEAD_TIME 2000.0f

#define SMALL_ENEMY_VELOCITY 0.03f
#define SMALL_ENEMY_SPAWN_PROBABILITY 0.0003f
#define SMALL_ENEMY_BULLET_PROBABILITY 0.0003f
#define SMALL_ENEMY_HEALTH 1
#define SMALL_ENEMY_POINTS 1

#define MEDIUM_ENEMY_VELOCITY 0.015f
#define MEDIUM_ENEMY_SPAWN_PROBABILITY 0.0001f
#define MEDIUM_ENEMY_BULLET_PROBABILITY 0.00075f
#define MEDIUM_ENEMY_HEALTH 5
#define MEDIUM_ENEMY_POINTS 5

#define LARGE_ENEMY_VELOCITY 0.006f
#define LARGE_ENEMY_SPAWN_PROBABILITY 0.00003f
#define LARGE_ENEMY_BULLET_PROBABILITY 0.003f
#define LARGE_ENEMY_HEALTH 10
#define LARGE_ENEMY_POINTS 25

#define ENEMY_BULLET_SPEED 0.05f

#define COLLISION_SCALE 0.7f

#define STAR_PROBABILITY 0.005f
#define STARS_MIN_VELOCITY 0.0015f
#define STARS_MAX_VELOCITY 0.015f
#define STARS_MIN_TRANSPARENCY 0.0f
#define STARS_MAX_TRANSPARENCY 0.9f
#define STARS_MIN_SCALE 1.0f
#define STARS_MAX_SCALE 4.0f

#define TIME_PER_ANIMATION 100.0f

typedef struct {
    MIXIN_STRUCT(EntitiesList, entity);
    float bulletThrottle;
    float deadTimer;
    int32_t score;
} Player;

typedef struct {
    float xOffset;
    float yOffset;
} PlayerCollisionExplosionOptions;

static BufferBuffer music;
static BufferBuffer shipBulletSound;
static BufferBuffer enemyBulletSound;
static BufferBuffer explosionSound;
static BufferBuffer enemyHit;

static Player player = { .sprite = &sprites_ship };
static EntitiesList smallEnemies = { .sprite = &sprites_smallEnemy };
static EntitiesList mediumEnemies = { .sprite = &sprites_mediumEnemy };
static EntitiesList largeEnemies = { .sprite = &sprites_largeEnemy };
static EntitiesList playerBullets = { .sprite = &sprites_playerBullet };
static EntitiesList enemyBullets = { .sprite = &sprites_enemyBullet };
static EntitiesList explosions = { .sprite = &sprites_explosion };
static EntitiesList stars = { .sprite = &sprites_whitePixel };
static EntitiesList textEntities = { .sprite = &sprites_text };

static enum {
    TITLE,
    MAIN_GAME
} gameState = TITLE;

#define SCORE_BUFFER_LENGTH 5 
static char scoreString[SCORE_BUFFER_LENGTH];

static uint8_t whitePixelData[4] = {255, 255, 255, 255};

static float tickTime = 0.0f;
static float animationTime = 0.0f;

enum {
    TITLE_START,
    TITLE_DISPLAY,
    TITLE_FADE,
    SUBTITLE_START,
    SUBTITLE_DISPLAY,
    SUBTITLE_FADE
};

static EventsSequence titleControls = {
    .events = {
        { 
            .delay = 1600.0f,
            .id =  TITLE_START
        },
        { 
            .delay = 1600.0f,
            .id =  SUBTITLE_START
        }
    },
    .count = 2
};

static EventsSequence titleSequence = {
    .events = {
        { 
            .duration = 2000.0f,
            .id =  TITLE_DISPLAY
        },
        {
            .duration = 2500.0f,
            .id = TITLE_FADE
        }
    },
    .count = 2
};

static EventsSequence subtitleSequence = {
    .events = {
        { 
            .duration = 2000.0f,
            .id =  SUBTITLE_DISPLAY
        },
        {
            .duration = 2500.0f,
            .id = SUBTITLE_FADE
        }
    },
    .count = 2
};

static void uintToString(uint32_t n, char* buffer, int32_t bufferLength) {
    buffer[bufferLength - 1] = '\0';
    int32_t i = bufferLength - 2;

    while (i >= 0) {
        buffer[i] = '0' + (n % 10);
        n /= 10;
        --i;
    }
}

static float lerp(float min, float max, float t) {
    return min + t * (max - min);
}

static float randomRange(float min, float max) {
    return lerp(min, max, (float) rand() / (RAND_MAX + 1));
}

static bool wavToSound(BufferBuffer* soundData, BufferBuffer* sound) {
    int32_t offset = 0;
    uint32_t chunkType = 0;
    uint32_t chunkSize = 0;
    uint32_t fileFormat = 0;

    chunkType = *(uint32_t *) soundData->data;
    offset +=  2 * sizeof(uint32_t);

    if (chunkType != 0x46464952) { // "RIFF" little-endian
        return false;
    }

    fileFormat = *(uint32_t *) (soundData->data + offset);
    offset += sizeof(uint32_t);

    if (fileFormat != 0x45564157) { // "WAVE" little-endian
        return false;
    }

    while (offset + 2 * sizeof(uint32_t) < soundData->size) {
        chunkType = *(uint32_t *) (soundData->data + offset);
        offset +=  sizeof(uint32_t);

        chunkSize = *(uint32_t *) (soundData->data + offset);
        offset +=  sizeof(uint32_t);

        if (offset + chunkSize > soundData->size) {
            return false;
        }

        if (chunkType == 0x61746164) {
            sound->size = chunkSize;
            sound->data = (uint8_t *) malloc(chunkSize);
            memcpy(sound->data, soundData->data + offset, chunkSize);
            break;
        }

        offset += chunkSize;
    }

    return true;
}

static void firePlayerBullet(float x, float y) {
    if (
        player.bulletThrottle > 0.0f || 
        player.deadTimer > 0.0f
    ) {
        return;
    }

    entities_spawn(&playerBullets, &(EntitiesInitOptions) {
        .x = x, 
        .y = y, 
        .vy = SHIP_BULLET_VELOCITY
    });
    platform_playSound(&shipBulletSound, false);
    player.bulletThrottle = SHIP_BULLET_THROTTLE;
}

static void fireEnemyBullet(float x, float y) {
    float shipCenterX = player.position[0] + player.sprite->panelDims[0] / 2.0f;
    float shipCenterY = player.position[1] + player.sprite->panelDims[1] / 2.0f;
    float dx = player.position[0] - x;
    float dy = player.position[1] - y;
    float d = sqrtf(dx * dx + dy * dy);

    entities_spawn(&enemyBullets, &(EntitiesInitOptions) {
        .x = x,
        .y = y,
        .vx = (dx / d) * ENEMY_BULLET_SPEED,
        .vy = (dy / d) * ENEMY_BULLET_SPEED
    });

    platform_playSound(&enemyBulletSound, false);
}

static void updateEntities(EntitiesList* list, float dt, float killBuffer) {
    for (int32_t i = 0; i < list->count; ++i) {
        float* position = list->position + i * 2;
        float* velocity = list->velocity + i * 2;
        position[0] += velocity[0] * dt;
        position[1] += velocity[1] * dt;

        if (list->whiteOut[i]) {
            list->whiteOut[i] = 0;
        }

        if (
            position[0] + list->sprite->panelDims[0] + killBuffer < 0 ||
            position[1] + list->sprite->panelDims[1] + killBuffer < 0 ||
            position[0] - killBuffer > GAME_WIDTH ||
            position[1] - killBuffer > GAME_HEIGHT
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

static bool checkPlayerBulletCollision(
    float bulletMin[2],
    float bulletMax[2],
    EntitiesList* enemies,
    float explosionXOffset,
    float explosionYOffset,
    int32_t points
) {
    bool hit = false;
    Sprites_CollisionBox* enemyCollisionBox = &enemies->sprite->collisionBox;
    for (int32_t i = 0; i < enemies->count; ++i) {
        float* position = enemies->position + i * 2;
        float enemyMin[] = {
            position[0] + enemyCollisionBox->min[0],
            position[1] + enemyCollisionBox->min[1]
        };
        float enemyMax[] = {
            position[0] + enemyCollisionBox->max[0],
            position[1] + enemyCollisionBox->max[1]
        };
        
        if (boxCollision(bulletMin, bulletMax, enemyMin, enemyMax)) {
            hit = true;
            --enemies->health[i];
            if (enemies->health[i] == 0) {
                entities_spawn(&explosions, &(EntitiesInitOptions) {
                    .x = position[0] + explosionXOffset, 
                    .y = position[1] + explosionYOffset
                });
                platform_playSound(&explosionSound, false);
                entities_kill(enemies, i);
                player.score += points;
            } else {
                platform_playSound(&enemyHit, false);
                enemies->whiteOut[i] = 1;
            }
        }    
    } 

    return hit;
}

static bool checkPlayerCollision(float playerMin[2], float playerMax[2], EntitiesList* list, PlayerCollisionExplosionOptions* opts) {
    bool playerHit = false;
    Sprites_CollisionBox* collisionBox = &list->sprite->collisionBox;
    for (int32_t i = 0; i < list->count; ++i) {
        float* position = list->position + i * 2;
        float entityMin[] = {
            position[0] + collisionBox->min[0],
            position[1] + collisionBox->min[1]
        };
        float entityMax[] = {
            position[0] + collisionBox->max[0],
            position[1] + collisionBox->max[1]
        };      
        if (boxCollision(playerMin, playerMax, entityMin, entityMax)) {
            playerHit = true;
            if (opts) {
                entities_spawn(&explosions, &(EntitiesInitOptions) {
                    .x = position[0] + opts->xOffset,
                    .y = position[1] + opts->yOffset 
                });     
            }
            entities_kill(list, i);
        }   
    }

    return playerHit;
}

void game_init(void) {
    srand((unsigned int) time(NULL));

    BufferBuffer soundData = { 0 };
    platform_loadBinFile("assets/audio/music.wav", &soundData);
    wavToSound(&soundData, &music);
    buffer_free(&soundData);

    platform_loadBinFile("assets/audio/Laser_002.wav", &soundData);
    wavToSound(&soundData, &shipBulletSound);
    buffer_free(&soundData);

    platform_loadBinFile("assets/audio/Hit_Hurt2.wav", &soundData);
    wavToSound(&soundData, &enemyBulletSound);
    buffer_free(&soundData);

    platform_loadBinFile("assets/audio/Explode1.wav", &soundData);
    wavToSound(&soundData, &explosionSound);
    buffer_free(&soundData);

    platform_loadBinFile("assets/audio/Jump1.wav", &soundData);
    wavToSound(&soundData, &enemyHit);
    buffer_free(&soundData);

    platform_playSound(&music, true);

    entities_spawn(&player.entity, & (EntitiesInitOptions) {
        .x = (GAME_WIDTH - player.sprite->panelDims[0]) / 2,
        .y = GAME_HEIGHT - player.sprite->panelDims[1] * 2.0f
    });

    for (int32_t i = 0; i < 40; ++i) {
        float t = randomRange(0.0f, 1.0f);
        entities_spawn(&stars, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_whitePixel.panelDims[0]), 
            .y = randomRange(0.0f, GAME_HEIGHT - sprites_whitePixel.panelDims[1]), 
            .vy = lerp(STARS_MIN_VELOCITY, STARS_MAX_VELOCITY, t),
            .transparency = lerp(STARS_MIN_TRANSPARENCY, STARS_MAX_TRANSPARENCY, 1.0f - t)
        });
    }

    renderer_init(GAME_WIDTH, GAME_HEIGHT);

    renderer_initBmpTexture("assets/sprites/ship.bmp", &player.texture);
    renderer_initBmpTexture("assets/sprites/enemy-small.bmp", &smallEnemies.texture);
    renderer_initBmpTexture("assets/sprites/enemy-medium.bmp", &mediumEnemies.texture);
    renderer_initBmpTexture("assets/sprites/enemy-big.bmp", &largeEnemies.texture);
    renderer_initBmpTexture("assets/sprites/explosion.bmp", &explosions.texture);
    renderer_initBmpTexture("assets/sprites/pixelspritefont32.bmp", &textEntities.texture);

    GLuint bulletTexture; // Shared between player and enemy bullets.
    renderer_initBmpTexture("assets/sprites/laser-bolts.bmp", &bulletTexture);

    playerBullets.texture = bulletTexture;
    enemyBullets.texture = bulletTexture;

    renderer_initDataTexture(whitePixelData, 1, 1, &stars.texture);

    entities_setAnimation(&player.entity, 0, SPRITES_SHIP_CENTER);

    uintToString(0, scoreString, SCORE_BUFFER_LENGTH);

    events_start(&titleControls);
}

static void updateStars(float dt) {
    if (randomRange(0.0f, 1.0f) < STAR_PROBABILITY * dt) {
        float t = randomRange(0.0f, 1.0f);
        entities_spawn(&stars, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_whitePixel.panelDims[0]), 
            .y = -sprites_whitePixel.panelDims[1], 
            .vy = lerp(STARS_MIN_VELOCITY, STARS_MAX_VELOCITY, t),
            .transparency = lerp(STARS_MIN_TRANSPARENCY, STARS_MAX_TRANSPARENCY, 1.0f - t)
        }); 
    }

    updateEntities(&stars, dt, 0.0f);
}

static void titleScreen(float dt) {
    events_beforeFrame(&titleControls, dt);
    events_beforeFrame(&titleSequence, dt);
    events_beforeFrame(&subtitleSequence, dt);
    
    updateStars(dt);

    textEntities.count = 0;

    for (int32_t i = 0; i < titleControls.triggeredCount; ++i) {
        EventsEvent* event = titleControls.triggeredEvents + i;
        if (event->id == TITLE_START) {
            events_start(&titleSequence);
        } 

        if (event->id == SUBTITLE_START) {
            events_start(&subtitleSequence);
        } 
    }

    for (int32_t i = 0; i < titleSequence.triggeredCount; ++i) {
        EventsEvent* event = titleSequence.triggeredEvents + i;
        if (event->id == TITLE_DISPLAY) {
            entities_fromText(&textEntities, "space-shooter.c", &(EntitiesFromTextOptions) {
                .x = GAME_WIDTH / 2.0f - 127.0f,
                .y = 62.0f, 
                .scale = 0.75f
            });
        } 

        if (event->id == TITLE_FADE) {
            entities_fromText(&textEntities, "space-shooter.c", &(EntitiesFromTextOptions) {
                .x = GAME_WIDTH / 2.0f - 127.0f,
                .y = 62.0f, 
                .scale = 0.75f,
                .transparency = event->alpha
            });
        } 
    }

    for (int32_t i = 0; i < subtitleSequence.triggeredCount; ++i) {
        EventsEvent* event = subtitleSequence.triggeredEvents + i;
        if (event->id == SUBTITLE_DISPLAY) {
            entities_fromText(&textEntities, "by Tarek Sherif", &(EntitiesFromTextOptions) {
                .x = GAME_WIDTH / 2.0f - 64.0f,
                .y = 85.0f, 
                .scale = 0.4f
            });
        } 

        if (event->id == SUBTITLE_FADE) {
            entities_fromText(&textEntities, "by Tarek Sherif", &(EntitiesFromTextOptions) {
                .x = GAME_WIDTH / 2.0f - 64.0f,
                .y = 85.0f, 
                .scale = 0.4f,
                .transparency = event->alpha
            });
        } 
    }

    if (titleSequence.complete && subtitleSequence.complete) {
        textEntities.count = 0;
        gameState = MAIN_GAME;
    }

    if (animationTime > TIME_PER_ANIMATION) {
        entities_updateAnimations(&player.entity);  
    }
}

static void mainGame(float dt) {
    updateStars(dt);

    if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_SPAWN_PROBABILITY * dt) {
        entities_spawn(&smallEnemies, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_smallEnemy.panelDims[0]), 
            .y = -sprites_smallEnemy.panelDims[1], 
            .vy = SMALL_ENEMY_VELOCITY,
            .health = SMALL_ENEMY_HEALTH
        }); 
    }

    if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_SPAWN_PROBABILITY * dt) {
        entities_spawn(&mediumEnemies, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_mediumEnemy.panelDims[0]), 
            .y = -sprites_mediumEnemy.panelDims[1], 
            .vy = MEDIUM_ENEMY_VELOCITY,
            .health = MEDIUM_ENEMY_HEALTH
        });
    }

    if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_SPAWN_PROBABILITY * dt) {
        entities_spawn(&largeEnemies, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_largeEnemy.panelDims[0]), 
            .y = -sprites_largeEnemy.panelDims[1], 
            .vy = LARGE_ENEMY_VELOCITY,
            .health = LARGE_ENEMY_HEALTH
        });
    }

    updateEntities(&smallEnemies, dt, 0.0f);
    updateEntities(&mediumEnemies, dt, 0.0f);
    updateEntities(&largeEnemies, dt, 0.0f);
    updateEntities(&playerBullets, dt, 32.0f);
    updateEntities(&enemyBullets, dt, 32.0f);

    Sprites_CollisionBox* playerBulletCollisionBox = &sprites_playerBullet.collisionBox;
    for (int32_t i = 0; i < playerBullets.count; ++i) {
        float* position = playerBullets.position + i * 2;
        float bulletMin[] = {
            position[0] + playerBulletCollisionBox->min[0],
            position[1] + playerBulletCollisionBox->min[1]
        };
        float bulletMax[] = {
            position[0] + playerBulletCollisionBox->max[0],
            position[1] + playerBulletCollisionBox->max[1]
        };
        
        if (checkPlayerBulletCollision(bulletMin, bulletMax, &smallEnemies, SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET, SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET, SMALL_ENEMY_POINTS) ||
            checkPlayerBulletCollision(bulletMin, bulletMax, &mediumEnemies, SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET, SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET, MEDIUM_ENEMY_POINTS) ||
            checkPlayerBulletCollision(bulletMin, bulletMax, &largeEnemies, SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET, SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET, LARGE_ENEMY_POINTS)) {
            entities_kill(&playerBullets, i);
        }  
    }

    if (player.bulletThrottle > 0.0f) {
        player.bulletThrottle -= dt; 
    }

    if (player.deadTimer > 0.0f) {
        player.deadTimer -= dt;
    } else {
        if (player.velocity[0] < -1.0f) {
            entities_setAnimation(&player.entity, 0, SPRITES_SHIP_LEFT);
        } else if (player.velocity[0] < 0.0f) {
            entities_setAnimation(&player.entity, 0, SPRITES_SHIP_CENTER_LEFT);
        } else if (player.velocity[0] > 1.0f) {
            entities_setAnimation(&player.entity, 0, SPRITES_SHIP_RIGHT);
        } else if (player.velocity[0] > 0.0f) {
            entities_setAnimation(&player.entity, 0, SPRITES_SHIP_CENTER_RIGHT);
        } else {
            entities_setAnimation(&player.entity, 0, SPRITES_SHIP_CENTER);
        }

        player.position[0] += player.velocity[0] * dt;
        player.position[1] += player.velocity[1] * dt;

        if (player.position[0] < 0.0f) {
            player.position[0] = 0.0f;
        }

        if (player.position[0] + player.sprite->panelDims[0] > GAME_WIDTH) {
            player.position[0] = GAME_WIDTH - player.sprite->panelDims[0];
        }

        if (player.position[1] < 0.0f) {
            player.position[1] = 0.0f;
        }

        if (player.position[1] + player.sprite->panelDims[1] > GAME_HEIGHT) {
            player.position[1] = GAME_HEIGHT - player.sprite->panelDims[1];
        }

        for (int32_t i = 0; i < smallEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_BULLET_PROBABILITY * dt) {
                float* position = smallEnemies.position + i * 2;
                fireEnemyBullet(position[0] + SPRITES_SMALL_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_SMALL_ENEMY_BULLET_Y_OFFSET);
            }
        }

        for (int32_t i = 0; i < mediumEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_BULLET_PROBABILITY * dt) {
                float* position = mediumEnemies.position + i * 2;
                fireEnemyBullet(position[0] + SPRITES_MEDIUM_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_MEDIUM_ENEMY_BULLET_Y_OFFSET);
            }
        }

        for (int32_t i = 0; i < largeEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_BULLET_PROBABILITY * dt) {
                float* position = largeEnemies.position + i * 2;
                fireEnemyBullet(position[0] + SPRITES_LARGE_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_LARGE_ENEMY_BULLET_Y_OFFSET);
            }
        }

        Sprites_CollisionBox* shipCollisionBox = &sprites_ship.collisionBox;
        float shipMin[] = {
            player.position[0] + shipCollisionBox->min[0],
            player.position[1] + shipCollisionBox->min[1]
        };
        float shipMax[] = {
            player.position[0] + shipCollisionBox->max[0],
            player.position[1] + shipCollisionBox->max[1]
        };


        bool bulletHit = checkPlayerCollision(shipMin, shipMax, &enemyBullets, NULL);
        bool smallEnemyHit = checkPlayerCollision(shipMin, shipMax, &smallEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool mediumEnemyHit = checkPlayerCollision(shipMin, shipMax, &mediumEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool largeEnemyHit = checkPlayerCollision(shipMin, shipMax, &largeEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET
        });

        if (bulletHit || smallEnemyHit || mediumEnemyHit || largeEnemyHit) {
            entities_spawn(&explosions, &(EntitiesInitOptions) {
                .x = player.position[0] + SPRITES_SHIP_EXPLOSION_X_OFFSET,
                .y = player.position[1] + SPRITES_SHIP_EXPLOSION_Y_OFFSET 
            });

            platform_playSound(&explosionSound, false);
            player.position[0] = GAME_WIDTH / 2 - player.sprite->panelDims[0] / 2;
            player.position[1] = GAME_HEIGHT - player.sprite->panelDims[0] * 3.0f;
            player.deadTimer = SHIP_DEAD_TIME;
                    
        }
    }

    uintToString(player.score, scoreString, SCORE_BUFFER_LENGTH);
    entities_fromText(&textEntities, scoreString, &(EntitiesFromTextOptions) {
        .x = 10.0f,
        .y = GAME_HEIGHT - 20.0f, 
        .scale = 0.4f,
        .reset = true
    });

    if (animationTime > TIME_PER_ANIMATION) {
        entities_updateAnimations(&player.entity);  
        entities_updateAnimations(&playerBullets);  
        entities_updateAnimations(&smallEnemies);
        entities_updateAnimations(&mediumEnemies);
        entities_updateAnimations(&largeEnemies);
        entities_updateAnimations(&enemyBullets);  
        entities_updateAnimations(&explosions);

        animationTime = 0.0f;
    }
}

void updateState(float dt) {
    animationTime += dt;

    if (gameState == TITLE) {
        titleScreen(dt);
    } else {
        mainGame(dt);  
    }

    if (animationTime > TIME_PER_ANIMATION) {
        animationTime = 0.0f;
    }
}

void game_update(float elapsedTime) {
    if (elapsedTime > 33.3f) {
        elapsedTime = 33.3f;
    }

    GameInput input;
    platform_getInput(&input);

    player.velocity[0] = SHIP_VELOCITY * input.velocity[0];
    player.velocity[1] = -SHIP_VELOCITY * input.velocity[1];

    if (input.shoot && gameState == MAIN_GAME) {
        firePlayerBullet(player.position[0] + SPRITES_SHIP_BULLET_X_OFFSET, player.position[1] + SPRITES_SHIP_BULLET_Y_OFFSET);
    }

    tickTime += elapsedTime;

    if (tickTime > TICK_DURATION) {
        while (tickTime > TICK_DURATION) {
            updateState(TICK_DURATION);    
            tickTime -= TICK_DURATION;
        }

        updateState(tickTime);
        tickTime = 0.0f;
    }
}

void game_resize(int width, int height) {
    renderer_resize(width, height);
    game_draw();
}

void game_draw(void) {
    renderer_beforeFrame();

    renderer_draw(&stars.renderList);

    if (player.deadTimer <= 0.0f) {
        renderer_draw(&player.renderList);
    }

    renderer_draw(&explosions.renderList);
    renderer_draw(&smallEnemies.renderList);
    renderer_draw(&mediumEnemies.renderList);
    renderer_draw(&largeEnemies.renderList);
    renderer_draw(&enemyBullets.renderList);
    renderer_draw(&playerBullets.renderList);
    renderer_draw(&textEntities.renderList);
}
