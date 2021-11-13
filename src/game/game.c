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
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "../../lib/simple-opengl-loader.h"
#include "../shared/data.h"
#include "../shared/platform-interface.h"
#include "utils.h"
#include "renderer.h"
#include "sprites.h"
#include "entities.h"
#include "events.h"

//////////////////////////////////
//  Game constants
//////////////////////////////////

#define GAME_WIDTH 320
#define GAME_HEIGHT 180
#define TICK_DURATION 16.6667f
#define COLLISION_SCALE 0.7f   // Scale factor on collision boxes when detecting collisions
#define TIME_PER_ANIMATION 100.0f // Time per frame of a sprite animation

//////////////////////////////////
//  Player constants
//////////////////////////////////

#define PLAYER_VELOCITY 0.075f
#define PLAYER_BULLET_VELOCITY (-0.2f)
#define PLAYER_BULLET_THROTTLE 100.0f
#define PLAYER_DEAD_TIME 2000.0f
#define PLAYER_INVINCIBLE_TIME 2000.0f
#define PLAYER_INVINCIBLE_ALPHA 0.5f
#define PLAYER_NUM_LIVES 3

//////////////////////////////////
//  Enemy constants
//////////////////////////////////

#define SMALL_ENEMY_VELOCITY 0.03f
#define SMALL_ENEMY_BULLET_PROBABILITY 0.0003f
#define SMALL_ENEMY_HEALTH 1
#define SMALL_ENEMY_POINTS 1
#define SMALL_ENEMY_INITIAL_SPAWN_PROBABILITY 0.0003f

#define MEDIUM_ENEMY_VELOCITY 0.015f
#define MEDIUM_ENEMY_BULLET_PROBABILITY 0.00075f
#define MEDIUM_ENEMY_HEALTH 5
#define MEDIUM_ENEMY_POINTS 10
#define MEDIUM_ENEMY_INITIAL_SPAWN_PROBABILITY 0.0001f

#define LARGE_ENEMY_VELOCITY 0.006f
#define LARGE_ENEMY_BULLET_PROBABILITY 0.003f
#define LARGE_ENEMY_HEALTH 10
#define LARGE_ENEMY_POINTS 25
#define LARGE_ENEMY_INITIAL_SPAWN_PROBABILITY 0.00003f

#define ENEMY_BULLET_SPEED 0.05f
#define ENEMY_WHITEOUT_TIME 25.0f // Flash when hit by a bullet

//////////////////////////////////
//  Starfield constants
//////////////////////////////////

#define STAR_PROBABILITY 0.005f
#define STARS_MIN_VELOCITY 0.0015f
#define STARS_MAX_VELOCITY 0.015f
#define STARS_MIN_TRANSPARENCY 0.0f
#define STARS_MAX_TRANSPARENCY 0.9f
#define STARS_MIN_SCALE 1.0f
#define STARS_MAX_SCALE 4.0f

//////////////////////////////////
//  Level transition constants
//////////////////////////////////

#define LEVEL_SPAWN_PROBABILITY_MULTIPLIER 1.3f
#define INITIAL_LEVEL_SCORE_THRESHOLD 50
#define LEVEL_WARP 0.2f
#define LEVEL_WARP_STAR_PROBABILITY_MULTIPLIER 16.0f

//////////////////////////////////
//  Game state
//////////////////////////////////

static enum {
    TITLE,
    LEVEL_TRANSITION,
    MAIN_GAME,
    GAME_OVER
} gameState = TITLE;

typedef struct {
    ENTITIES_LIST_MIXIN(entity);
    float bulletThrottle;
    float deadTimer;
    float invincibleTimer;
    int32_t score;
    int32_t lives;
} Player;

typedef struct {
    float xOffset;
    float yOffset;
} PlayerCollisionExplosionOptions;

static struct {
    DataBuffer music;
    DataBuffer playerBullet;
    DataBuffer enemyBullet;
    DataBuffer explosion;
    DataBuffer enemyHit;
} sounds;

static Player player = { .sprite = &sprites_player, .lives = PLAYER_NUM_LIVES };
static EntitiesList smallEnemies = { .sprite = &sprites_smallEnemy };
static EntitiesList mediumEnemies = { .sprite = &sprites_mediumEnemy };
static EntitiesList largeEnemies = { .sprite = &sprites_largeEnemy };
static EntitiesList playerBullets = { .sprite = &sprites_playerBullet };
static EntitiesList enemyBullets = { .sprite = &sprites_enemyBullet };
static EntitiesList explosions = { .sprite = &sprites_explosion };
static EntitiesList stars = { .sprite = &sprites_whitePixel };
static EntitiesList textEntities = { .sprite = &sprites_text };
static EntitiesList livesEntities = { .sprite = &sprites_player };

#define SCORE_BUFFER_LENGTH 5 
static char scoreString[SCORE_BUFFER_LENGTH];

#define LEVEL_BUFFER_LENGTH 32 
static char levelString[LEVEL_BUFFER_LENGTH];

static uint8_t whitePixelData[4] = {255, 255, 255, 255};

static float tickTime = 0.0f;
static float animationTime = 0.0f;

//////////////////////////////////
//  Per-level state
//////////////////////////////////

static int32_t level = 1;
static int32_t levelScoreThreshold = INITIAL_LEVEL_SCORE_THRESHOLD;
static float smallEnemySpawnProbability = SMALL_ENEMY_INITIAL_SPAWN_PROBABILITY;
static float mediumEnemySpawnProbability = MEDIUM_ENEMY_INITIAL_SPAWN_PROBABILITY;
static float largeEnemySpawnProbability = LARGE_ENEMY_INITIAL_SPAWN_PROBABILITY;
static float levelWarpVy = 0.0f;
static float starProbabilityMultiplier = 1.0f;


//////////////////////////////////
//  Data load helpers
//////////////////////////////////

static void loadSound(const char* fileName, DataBuffer* sound) {
    DataBuffer soundData = { 0 };
    platform_loadFile(fileName, &soundData, false);
    utils_wavToSound(&soundData, sound);
    data_freeBuffer(&soundData);
}

static void loadTexture(const char* fileName, GLuint *texture) {
    DataBuffer imageData = { 0 };
    DataImage image = { 0 };

    platform_loadFile(fileName, &imageData, false);
    utils_bmpToImage(&imageData, &image);
    renderer_initTexture(texture, image.data, image.width, image.height);
    data_freeBuffer(&imageData);
    data_freeImage(&image);
}

//////////////////////////////////
//  Level transition helpers
//////////////////////////////////

static void updateLevelText(void) {
    snprintf(levelString, LEVEL_BUFFER_LENGTH, "Level %d", level);
}

static void transitionLevel(void) {
    gameState = LEVEL_TRANSITION;
    if (level > 1) {
        levelWarpVy = LEVEL_WARP;
        starProbabilityMultiplier = LEVEL_WARP_STAR_PROBABILITY_MULTIPLIER;
    }
    playerBullets.count = 0;
    updateLevelText();
    events_start(&events_levelTransitionSequence);
}

//////////////////////////////////
//  Player helpers
//////////////////////////////////

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
        .vy = PLAYER_BULLET_VELOCITY
    });
    platform_playSound(&sounds.playerBullet, false);
    player.bulletThrottle = PLAYER_BULLET_THROTTLE;
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
        
        if (utils_boxCollision(bulletMin, bulletMax, enemyMin, enemyMax, COLLISION_SCALE)) {
            hit = true;
            --enemies->health[i];
            if (enemies->health[i] == 0) {
                entities_spawn(&explosions, &(EntitiesInitOptions) {
                    .x = position[0] + explosionXOffset, 
                    .y = position[1] + explosionYOffset
                });
                platform_playSound(&sounds.explosion, false);
                enemies->dead[i] = true;
                player.score += points;
            } else {
                platform_playSound(&sounds.enemyHit, false);
                enemies->whiteOut[i] = ENEMY_WHITEOUT_TIME;
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
        if (utils_boxCollision(playerMin, playerMax, entityMin, entityMax, COLLISION_SCALE)) {
            playerHit = true;
            if (opts) {
                entities_spawn(&explosions, &(EntitiesInitOptions) {
                    .x = position[0] + opts->xOffset,
                    .y = position[1] + opts->yOffset 
                });     
            }
            list->dead[i] = true;
        }   
    }

    return playerHit;
}

static void fireEnemyBullet(float x, float y) {
    float dx = player.position[0] - x;
    float dy = player.position[1] - y;
    float d = sqrtf(dx * dx + dy * dy);

    entities_spawn(&enemyBullets, &(EntitiesInitOptions) {
        .x = x,
        .y = y,
        .vx = (dx / d) * ENEMY_BULLET_SPEED,
        .vy = (dy / d) * ENEMY_BULLET_SPEED
    });

    platform_playSound(&sounds.enemyBullet, false);
}

//////////////////////////////////
//  General entity helpers
//////////////////////////////////

static void updateEntities(EntitiesList* list, float dt, float killBuffer) {
    for (int32_t i = 0; i < list->count; ++i) {
        float* position = list->position + i * 2;
        float* velocity = list->velocity + i * 2;
        position[0] += velocity[0] * dt;
        position[1] += (velocity[1] + levelWarpVy) * dt;

        if (list->whiteOut[i] > 0.0f) {
            list->whiteOut[i] -= dt;
        }

        if (
            position[0] + list->sprite->panelDims[0] + killBuffer < 0 ||
            position[1] + list->sprite->panelDims[1] + killBuffer < 0 ||
            position[0] - killBuffer > GAME_WIDTH ||
            position[1] - killBuffer > GAME_HEIGHT
        ) {
            list->dead[i] = true;
        }
    }
}

static void updateAnimations(void) {
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

static void filterDeadEntities(void) {
    entities_filterDead(&playerBullets);  
    entities_filterDead(&smallEnemies);
    entities_filterDead(&mediumEnemies);
    entities_filterDead(&largeEnemies);
    entities_filterDead(&enemyBullets);  
    entities_filterDead(&explosions);
    entities_filterDead(&stars);
}

static void updateStars(float dt) {
    if (utils_randomRange(0.0f, 1.0f) < STAR_PROBABILITY * starProbabilityMultiplier * dt) {
        float t = utils_randomRange(0.0f, 1.0f);
        entities_spawn(&stars, &(EntitiesInitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_whitePixel.panelDims[0]), 
            .y = -sprites_whitePixel.panelDims[1], 
            .vy = utils_lerp(STARS_MIN_VELOCITY, STARS_MAX_VELOCITY, t),
            .transparency = utils_lerp(STARS_MIN_TRANSPARENCY, STARS_MAX_TRANSPARENCY, 1.0f - t)
        }); 
    }

    updateEntities(&stars, dt, 0.0f);
}

//////////////////////////////////
//  HUD helpers
//////////////////////////////////

static void livesToEntities(Player *player, EntitiesList* lives) {
    lives->count = 0;
    for (int32_t i = 0; i < player->lives; ++i) {
        entities_spawn(lives, &(EntitiesInitOptions) { 
            .x = 12.5f + i * (player->sprite->panelDims[0] * 0.55f + 0.5f),
            .y = GAME_HEIGHT - 32.0f, 
            .scale = 0.45f,
            .whiteOut = 999999.0f
        });
    }
}

static void updateScoreDisplay() {
    utils_uintToString(player.score, scoreString, SCORE_BUFFER_LENGTH);
    entities_fromText(&textEntities, scoreString, &(EntitiesFromTextOptions) {
        .x = 10.0f,
        .y = GAME_HEIGHT - 20.0f, 
        .scale = 0.4f
    });
}

//////////////////////////////////
//  Entitity simulation functions
//////////////////////////////////

static void simEnemies(float dt) {
    if (utils_randomRange(0.0f, 1.0f) < smallEnemySpawnProbability * dt) {
        entities_spawn(&smallEnemies, &(EntitiesInitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_smallEnemy.panelDims[0]), 
            .y = -sprites_smallEnemy.panelDims[1], 
            .vy = SMALL_ENEMY_VELOCITY,
            .health = SMALL_ENEMY_HEALTH
        }); 
    }

    if (utils_randomRange(0.0f, 1.0f) < mediumEnemySpawnProbability * dt) {
        entities_spawn(&mediumEnemies, &(EntitiesInitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_mediumEnemy.panelDims[0]), 
            .y = -sprites_mediumEnemy.panelDims[1], 
            .vy = MEDIUM_ENEMY_VELOCITY,
            .health = MEDIUM_ENEMY_HEALTH
        });
    }

    if (utils_randomRange(0.0f, 1.0f) < largeEnemySpawnProbability * dt) {
        entities_spawn(&largeEnemies, &(EntitiesInitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_largeEnemy.panelDims[0]), 
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
            playerBullets.dead[i] = true;
        }  
    }
}

static void simPlayer(float dt) {
    GameInput input = { 0 };
    platform_getInput(&input);

    player.velocity[0] = PLAYER_VELOCITY * input.velocity[0];
    player.velocity[1] = -PLAYER_VELOCITY * input.velocity[1];

    if (input.shoot) {
        firePlayerBullet(player.position[0] + SPRITES_PLAYER_BULLET_X_OFFSET, player.position[1] + SPRITES_PLAYER_BULLET_Y_OFFSET);
    }

    if (player.velocity[0] < -1.0f) {
        entities_setAnimation(&player.entity, 0, SPRITES_PLAYER_LEFT);
    } else if (player.velocity[0] < 0.0f) {
        entities_setAnimation(&player.entity, 0, SPRITES_PLAYER_CENTER_LEFT);
    } else if (player.velocity[0] > 1.0f) {
        entities_setAnimation(&player.entity, 0, SPRITES_PLAYER_RIGHT);
    } else if (player.velocity[0] > 0.0f) {
        entities_setAnimation(&player.entity, 0, SPRITES_PLAYER_CENTER_RIGHT);
    } else {
        entities_setAnimation(&player.entity, 0, SPRITES_PLAYER_CENTER);
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
        if (utils_randomRange(0.0f, 1.0f) < SMALL_ENEMY_BULLET_PROBABILITY * dt) {
            float* position = smallEnemies.position + i * 2;
            fireEnemyBullet(position[0] + SPRITES_SMALL_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_SMALL_ENEMY_BULLET_Y_OFFSET);
        }
    }

    for (int32_t i = 0; i < mediumEnemies.count; ++i) {
        if (utils_randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_BULLET_PROBABILITY * dt) {
            float* position = mediumEnemies.position + i * 2;
            fireEnemyBullet(position[0] + SPRITES_MEDIUM_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_MEDIUM_ENEMY_BULLET_Y_OFFSET);
        }
    }

    for (int32_t i = 0; i < largeEnemies.count; ++i) {
        if (utils_randomRange(0.0f, 1.0f) < LARGE_ENEMY_BULLET_PROBABILITY * dt) {
            float* position = largeEnemies.position + i * 2;
            fireEnemyBullet(position[0] + SPRITES_LARGE_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_LARGE_ENEMY_BULLET_Y_OFFSET);
        }
    }

    if (player.invincibleTimer > 0.0f) {
        player.invincibleTimer -= dt;
        player.alpha[0] = PLAYER_INVINCIBLE_ALPHA;
    } else {
        player.alpha[0] = 1.0f;
        Sprites_CollisionBox* playerCollisionBox = &sprites_player.collisionBox;
        float playerMin[] = {
            player.position[0] + playerCollisionBox->min[0],
            player.position[1] + playerCollisionBox->min[1]
        };
        float playerMax[] = {
            player.position[0] + playerCollisionBox->max[0],
            player.position[1] + playerCollisionBox->max[1]
        };


        bool bulletHit = checkPlayerCollision(playerMin, playerMax, &enemyBullets, NULL);
        bool smallEnemyHit = checkPlayerCollision(playerMin, playerMax, &smallEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool mediumEnemyHit = checkPlayerCollision(playerMin, playerMax, &mediumEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool largeEnemyHit = checkPlayerCollision(playerMin, playerMax, &largeEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET
        });

        if (bulletHit || smallEnemyHit || mediumEnemyHit || largeEnemyHit) {
            entities_spawn(&explosions, &(EntitiesInitOptions) {
                .x = player.position[0] + SPRITES_PLAYER_EXPLOSION_X_OFFSET,
                .y = player.position[1] + SPRITES_PLAYER_EXPLOSION_Y_OFFSET 
            });

            platform_playSound(&sounds.explosion, false);
            player.position[0] = GAME_WIDTH / 2 - player.sprite->panelDims[0] / 2;
            player.position[1] = GAME_HEIGHT - player.sprite->panelDims[0] * 3.0f;
            player.deadTimer = PLAYER_DEAD_TIME;
            --player.lives;
        } 
    }

       
}

//////////////////////////////////
//  Game state functions
//////////////////////////////////

static void titleScreen(float dt) {
    events_beforeFrame(&events_titleControlSequence, dt);
    events_beforeFrame(&events_titleSequence, dt);
    events_beforeFrame(&events_subtitleSequence, dt);
    
    updateStars(dt);

    textEntities.count = 0;

    if (events_on(&events_titleControlSequence, EVENTS_TITLE)) {
        events_start(&events_titleSequence);
    }

    if (events_on(&events_titleControlSequence, EVENTS_SUBTITLE)) {
        events_start(&events_subtitleSequence);
    }

    if (events_on(&events_titleSequence, EVENTS_DISPLAY)) {
        entities_fromText(&textEntities, "space-shooter.c", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 127.0f,
            .y = 62.0f, 
            .scale = 0.75f
        });
    }

    if (events_on(&events_titleSequence, EVENTS_FADE)) {
        entities_fromText(&textEntities, "space-shooter.c", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 127.0f,
            .y = 62.0f, 
            .scale = 0.75f,
            .transparency = events_titleSequence.alpha
        });
    }

    if (events_on(&events_subtitleSequence, EVENTS_DISPLAY)) {
        entities_fromText(&textEntities, "by Tarek Sherif", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 64.0f,
            .y = 85.0f, 
            .scale = 0.4f
        });
    }

    if (events_on(&events_subtitleSequence, EVENTS_FADE)) {
        entities_fromText(&textEntities, "by Tarek Sherif", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 64.0f,
            .y = 85.0f, 
            .scale = 0.4f,
            .transparency = events_subtitleSequence.alpha
        });
    }

    if (events_titleSequence.complete && events_subtitleSequence.complete) {
        textEntities.count = 0;
        transitionLevel();
    }

    updateAnimations();
}

static void levelTransition(float dt) {
    events_beforeFrame(&events_levelTransitionSequence, dt);
    textEntities.count = 0;


    if (events_on(&events_levelTransitionSequence, EVENTS_DISPLAY)) {
        entities_fromText(&textEntities, levelString, &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 62.0f,
            .y = 72.0f, 
            .scale = 0.75f
        });
    }

    livesToEntities(&player, &livesEntities);
    updateStars(dt);
    updateEntities(&smallEnemies, dt, 0.0f);
    updateEntities(&mediumEnemies, dt, 0.0f);
    updateEntities(&largeEnemies, dt, 0.0f);
    updateEntities(&playerBullets, dt, 32.0f);
    updateEntities(&enemyBullets, dt, 32.0f);
    updateEntities(&explosions, dt, 32.0f);

    if (events_levelTransitionSequence.complete) {
        textEntities.count = 0;
        levelWarpVy = 0.0f;
        starProbabilityMultiplier = 1.0f;
        gameState = MAIN_GAME;
    }

    updateScoreDisplay();
    updateAnimations();
}

static void mainGame(float dt) {
    textEntities.count = 0;
    
    updateStars(dt);
    simEnemies(dt);

    livesToEntities(&player, &livesEntities);

    if (player.bulletThrottle > 0.0f) {
        player.bulletThrottle -= dt; 
    }

    if (player.lives > 0) {
        if (player.deadTimer > 0.0f) {
            player.deadTimer -= dt;
            if (player.deadTimer < 0.0f) {
                player.invincibleTimer = PLAYER_INVINCIBLE_TIME;
                player.alpha[0] = PLAYER_INVINCIBLE_ALPHA;
            }
        } else {
            simPlayer(dt);           
        }

        if (player.score >= levelScoreThreshold) {
            ++level;
            levelScoreThreshold *= 2;
            smallEnemySpawnProbability *= LEVEL_SPAWN_PROBABILITY_MULTIPLIER;
            mediumEnemySpawnProbability *= LEVEL_SPAWN_PROBABILITY_MULTIPLIER;
            largeEnemySpawnProbability *= LEVEL_SPAWN_PROBABILITY_MULTIPLIER;
            transitionLevel();
        }
    } else {
        events_start(&events_gameOverSequence);
        gameState = GAME_OVER;
    }

    updateScoreDisplay();
    updateAnimations();
    filterDeadEntities();
}

static void gameOver(float dt) {
    events_beforeFrame(&events_gameOverSequence, dt);
    events_beforeFrame(&events_gameOverRestartSequence, dt);
    textEntities.count = 0;

    GameInput input = { 0 };
    platform_getInput(&input);

    updateStars(dt);
    simEnemies(dt);


    entities_fromText(&textEntities, "Game Over", &(EntitiesFromTextOptions) {
        .x = GAME_WIDTH / 2.0f - 127.0f,
        .y = 68.0f,
        .scale = 1.2f
    });

    if (events_on(&events_gameOverSequence, EVENTS_RESTART)) {
        events_start(&events_gameOverRestartSequence);
    }

    if (events_on(&events_gameOverRestartSequence, EVENTS_DISPLAY)) {
        entities_fromText(&textEntities, "Press 'Shoot' to Restart", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 107.0f,
            .y = 104.0f,
            .scale = 0.4f
        }); 
    }

    updateScoreDisplay();
    updateAnimations();
    filterDeadEntities();

    if (events_gameOverRestartSequence.running && input.shoot) {
        player.lives = PLAYER_NUM_LIVES;
        player.score = 0;
        player.deadTimer = 0.0f;
        smallEnemies.count = 0;
        mediumEnemies.count = 0;
        largeEnemies.count = 0;
        enemyBullets.count = 0;
        level = 1;
        levelScoreThreshold = INITIAL_LEVEL_SCORE_THRESHOLD;
        smallEnemySpawnProbability = SMALL_ENEMY_INITIAL_SPAWN_PROBABILITY;
        mediumEnemySpawnProbability = MEDIUM_ENEMY_INITIAL_SPAWN_PROBABILITY;
        largeEnemySpawnProbability = LARGE_ENEMY_INITIAL_SPAWN_PROBABILITY;
        events_stop(&events_gameOverSequence);
        events_stop(&events_gameOverRestartSequence);
        transitionLevel();
    }

}

//////////////////////////////////
//  Main state update function
//////////////////////////////////

static void simulate(float dt) {
    animationTime += dt;

    switch(gameState) {
        case TITLE: titleScreen(dt); break;
        case LEVEL_TRANSITION: levelTransition(dt); break;
        case MAIN_GAME: mainGame(dt); break;
        case GAME_OVER: gameOver(dt); break;
    }

    if (animationTime > TIME_PER_ANIMATION) {
        animationTime = 0.0f;
    }
}

//////////////////////////////////
//  Platform interface functions
//////////////////////////////////

void game_init(void) {
    // Init subsystems
    utils_init();
    renderer_init(GAME_WIDTH, GAME_HEIGHT);

    // Load assets
    loadSound("assets/audio/music.wav", &sounds.music);
    loadSound("assets/audio/Laser_002.wav", &sounds.playerBullet);
    loadSound("assets/audio/Hit_Hurt2.wav", &sounds.enemyBullet);
    loadSound("assets/audio/Explode1.wav", &sounds.explosion);
    loadSound("assets/audio/Jump1.wav", &sounds.enemyHit);

    loadTexture("assets/sprites/ship.bmp", &player.texture);
    loadTexture("assets/sprites/enemy-small.bmp", &smallEnemies.texture);
    loadTexture("assets/sprites/enemy-medium.bmp", &mediumEnemies.texture);
    loadTexture("assets/sprites/enemy-big.bmp", &largeEnemies.texture);
    loadTexture("assets/sprites/explosion.bmp", &explosions.texture);
    loadTexture("assets/sprites/pixelspritefont32.bmp", &textEntities.texture);
    loadTexture("assets/sprites/laser-bolts.bmp", &playerBullets.texture);

    // Shared textures    
    livesEntities.texture = player.texture;
    enemyBullets.texture = playerBullets.texture;

    renderer_initTexture(&stars.texture, whitePixelData, 1, 1);

    // Init game
    platform_playSound(&sounds.music, true);

    entities_spawn(&player.entity, & (EntitiesInitOptions) {
        .x = (GAME_WIDTH - player.sprite->panelDims[0]) / 2,
        .y = GAME_HEIGHT - player.sprite->panelDims[1] * 2.0f
    });

    entities_setAnimation(&player.entity, 0, SPRITES_PLAYER_CENTER);

    for (int32_t i = 0; i < 40; ++i) {
        float t = utils_randomRange(0.0f, 1.0f);
        entities_spawn(&stars, &(EntitiesInitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_whitePixel.panelDims[0]), 
            .y = utils_randomRange(0.0f, GAME_HEIGHT - sprites_whitePixel.panelDims[1]), 
            .vy = utils_lerp(STARS_MIN_VELOCITY, STARS_MAX_VELOCITY, t),
            .transparency = utils_lerp(STARS_MIN_TRANSPARENCY, STARS_MAX_TRANSPARENCY, 1.0f - t)
        });
    }


    utils_uintToString(0, scoreString, SCORE_BUFFER_LENGTH);
    updateLevelText();

    events_start(&events_titleControlSequence);
}

////////////////////////////////////////////////////////////
// Fixed time step + sim any time left at
// end of frame. 
// References:
// - https://www.gafferongames.com/post/fix_your_timestep/
// - https://www.youtube.com/watch?v=jTzIDmjkLQo
// - https://www.youtube.com/watch?v=fdAOPHgW7qM
////////////////////////////////////////////////////////////

void game_update(float elapsedTime) {
    if (elapsedTime > 33.3f) {
        elapsedTime = 33.3f;
    }

    tickTime += elapsedTime;

    if (tickTime > TICK_DURATION) {
        while (tickTime > TICK_DURATION) {
            simulate(TICK_DURATION);    
            tickTime -= TICK_DURATION;
        }

        simulate(tickTime);
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
    renderer_draw(&livesEntities.renderList);
}
