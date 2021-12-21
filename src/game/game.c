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
#define SCORE_TEXT_LENGTH 5 

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

//////////////////////////////////
//  Level transition constants
//////////////////////////////////

#define LEVEL_SPAWN_PROBABILITY_MULTIPLIER 1.3f
#define INITIAL_LEVEL_SCORE_THRESHOLD 50
#define LEVEL_WARP 0.2f
#define LEVEL_WARP_STAR_PROBABILITY_MULTIPLIER 16.0f
#define LEVEL_TITLE_BUFFER_SIZE 32 
#define BASE_TITLE_LENGTH 7
#define BASE_NEXT_LEVEL_TEXT_LENGTH 20

//////////////////////////////////
//  Game state
//////////////////////////////////

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
    enum {
        TITLE_SCREEN,
        LEVEL_TRANSITION,
        MAIN_GAME,
        GAME_OVER
    } state;
    Game_Input input;
    float tickTime;
    float animationTime;
    char scoreText[SCORE_TEXT_LENGTH];
} gameState = {
    .state = TITLE_SCREEN
};

static struct {
    Player player;
    Entities_List smallEnemies;
    Entities_List mediumEnemies;
    Entities_List largeEnemies;
    Entities_List playerBullets;
    Entities_List enemyBullets;
    Entities_List explosions;
    Entities_List stars;
    Entities_List text;
    Entities_List lives;
} entities = {
    .player = { .sprite = &sprites_player, .lives = PLAYER_NUM_LIVES },
    .smallEnemies = { .sprite = &sprites_smallEnemy },
    .mediumEnemies = { .sprite = &sprites_mediumEnemy },
    .largeEnemies = { .sprite = &sprites_largeEnemy },
    .playerBullets = { .sprite = &sprites_playerBullet },
    .enemyBullets = { .sprite = &sprites_enemyBullet },
    .explosions = { .sprite = &sprites_explosion },
    .stars = { .sprite = &sprites_whitePixel },
    .text = { .sprite = &sprites_text },
    .lives = { .sprite = &sprites_player }
};

static struct {
    struct {
        Data_Buffer music;
        Data_Buffer playerBullet;
        Data_Buffer enemyBullet;
        Data_Buffer explosion;
        Data_Buffer enemyHit;
    } sounds;
    uint8_t whitePixel[4];
} gameData = {
    .whitePixel = {255, 255, 255, 255}
};

static struct {
    int32_t level;
    int32_t scoreThreshold;
    float smallEnemySpawnProbability;
    float mediumEnemySpawnProbability;
    float largeEnemySpawnProbability;

    // For level transitions
    float starProbabilityMultiplier;
    float warpVy;
    int32_t nextLevelTextLength;
    char title[LEVEL_TITLE_BUFFER_SIZE];
    char nextLevelText[LEVEL_TITLE_BUFFER_SIZE];
} levelState = {
    .level = 1,
    .scoreThreshold = INITIAL_LEVEL_SCORE_THRESHOLD,
    .smallEnemySpawnProbability = SMALL_ENEMY_INITIAL_SPAWN_PROBABILITY,
    .mediumEnemySpawnProbability = MEDIUM_ENEMY_INITIAL_SPAWN_PROBABILITY,
    .largeEnemySpawnProbability = LARGE_ENEMY_INITIAL_SPAWN_PROBABILITY,
    .warpVy = 0.0f,
    .starProbabilityMultiplier = 1.0f
};


//////////////////////////////////
//  Data load helpers
//////////////////////////////////

static bool loadSound(const char* fileName, Data_Buffer* sound) {
    Data_Buffer soundData = { 0 };
    bool result = platform_loadFile(fileName, &soundData, false) && utils_wavToSound(&soundData, sound);
    data_freeBuffer(&soundData);
    return result;
}

static bool loadTexture(const char* fileName, uint32_t *texture) {
    Data_Buffer imageData = { 0 };
    Data_Image image = { 0 };

    bool result = platform_loadFile(fileName, &imageData, false) && utils_bmpToImage(&imageData, &image);
    
    *texture = renderer_createTexture(image.data, image.width, image.height);
    data_freeBuffer(&imageData);
    data_freeImage(&image);

    return result;
}

//////////////////////////////////
//  Level transition helpers
//////////////////////////////////

static void updateLevelTitle(void) {
    snprintf(levelState.title, LEVEL_TITLE_BUFFER_SIZE, "Level %d", levelState.level);
    levelState.nextLevelTextLength = snprintf(levelState.nextLevelText, LEVEL_TITLE_BUFFER_SIZE, "Level %d at %d points", levelState.level + 1, levelState.scoreThreshold);
}

static void transitionLevel(void) {
    gameState.state = LEVEL_TRANSITION;
    if (levelState.level > 1) {
        levelState.warpVy = LEVEL_WARP;
        levelState.starProbabilityMultiplier = LEVEL_WARP_STAR_PROBABILITY_MULTIPLIER;
    }
    entities_setAnimation(&entities.player.entity, 0, SPRITES_PLAYER_CENTER);
    entities.playerBullets.count = 0;
    updateLevelTitle();
    events_start(&events_levelTransitionSequence);
}

//////////////////////////////////
//  Player helpers
//////////////////////////////////

static void firePlayerBullet(float x, float y) {
    if (
        entities.player.bulletThrottle > 0.0f || 
        entities.player.deadTimer > 0.0f
    ) {
        return;
    }

    entities_spawn(&entities.playerBullets, &(Entities_InitOptions) {
        .x = x, 
        .y = y, 
        .vy = PLAYER_BULLET_VELOCITY
    });
    platform_playSound(&gameData.sounds.playerBullet, false);
    entities.player.bulletThrottle = PLAYER_BULLET_THROTTLE;
}

static bool checkPlayerBulletCollision(
    float bulletMin[2],
    float bulletMax[2],
    Entities_List* enemies,
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
                entities_spawn(&entities.explosions, &(Entities_InitOptions) {
                    .x = position[0] + explosionXOffset, 
                    .y = position[1] + explosionYOffset
                });
                platform_playSound(&gameData.sounds.explosion, false);
                enemies->dead[i] = true;
                entities.player.score += points;
            } else {
                platform_playSound(&gameData.sounds.enemyHit, false);
                enemies->whiteOut[i] = ENEMY_WHITEOUT_TIME;
            }
        }    
    } 

    return hit;
}

static bool checkPlayerCollision(float playerMin[2], float playerMax[2], Entities_List* list, PlayerCollisionExplosionOptions* opts) {
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
                entities_spawn(&entities.explosions, &(Entities_InitOptions) {
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
    float dx = entities.player.position[0] - x;
    float dy = entities.player.position[1] - y;
    float d = sqrtf(dx * dx + dy * dy);

    entities_spawn(&entities.enemyBullets, &(Entities_InitOptions) {
        .x = x,
        .y = y,
        .vx = (dx / d) * ENEMY_BULLET_SPEED,
        .vy = (dy / d) * ENEMY_BULLET_SPEED
    });

    platform_playSound(&gameData.sounds.enemyBullet, false);
}

//////////////////////////////////
//  General entity helpers
//////////////////////////////////

static void updateEntities(Entities_List* list, float dt, float killBuffer) {
    for (int32_t i = 0; i < list->count; ++i) {
        float* position = list->position + i * 2;
        float* velocity = list->velocity + i * 2;
        position[0] += velocity[0] * dt;
        position[1] += (velocity[1] + levelState.warpVy) * dt;

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
    if (gameState.animationTime > TIME_PER_ANIMATION) {
        entities_updateAnimations(&entities.player.entity);  
        entities_updateAnimations(&entities.playerBullets);  
        entities_updateAnimations(&entities.smallEnemies);
        entities_updateAnimations(&entities.mediumEnemies);
        entities_updateAnimations(&entities.largeEnemies);
        entities_updateAnimations(&entities.enemyBullets);  
        entities_updateAnimations(&entities.explosions);

        gameState.animationTime = 0.0f;
    }
}

static void filterDeadEntities(void) {
    entities_filterDead(&entities.playerBullets);  
    entities_filterDead(&entities.smallEnemies);
    entities_filterDead(&entities.mediumEnemies);
    entities_filterDead(&entities.largeEnemies);
    entities_filterDead(&entities.enemyBullets);  
    entities_filterDead(&entities.explosions);
    entities_filterDead(&entities.stars);
}

static void updateStars(float dt) {
    if (utils_randomRange(0.0f, 1.0f) < STAR_PROBABILITY * levelState.starProbabilityMultiplier * dt) {
        float t = utils_randomRange(0.0f, 1.0f);
        entities_spawn(&entities.stars, &(Entities_InitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_whitePixel.panelDims[0]), 
            .y = -sprites_whitePixel.panelDims[1], 
            .vy = utils_lerp(STARS_MIN_VELOCITY, STARS_MAX_VELOCITY, t),
            .transparency = utils_lerp(STARS_MIN_TRANSPARENCY, STARS_MAX_TRANSPARENCY, 1.0f - t)
        }); 
    }

    updateEntities(&entities.stars, dt, 0.0f);
}

//////////////////////////////////
//  HUD helpers
//////////////////////////////////

static void livesToEntities(void) {
    entities.lives.count = 0;
    for (int32_t i = 0; i < entities.player.lives; ++i) {
        entities_spawn(&entities.lives, &(Entities_InitOptions) { 
            .x = 12.5f + i * (entities.player.sprite->panelDims[0] * 0.55f + 0.5f),
            .y = GAME_HEIGHT - 32.0f, 
            .scale = 0.45f
        });
    }
}

static void updateScoreDisplay() {
    utils_uintToString(entities.player.score, gameState.scoreText, SCORE_TEXT_LENGTH);
    entities_fromText(&entities.text, gameState.scoreText, &(Entities_FromTextOptions) {
        .x = 10.0f,
        .y = GAME_HEIGHT - 20.0f, 
        .scale = 0.4f
    });
}

//////////////////////////////////
//  Entitity simulation functions
//////////////////////////////////

static void simWorld(float dt) {
    updateStars(dt);

    // Spawn new enemies
    if (utils_randomRange(0.0f, 1.0f) < levelState.smallEnemySpawnProbability * dt) {
        entities_spawn(&entities.smallEnemies, &(Entities_InitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_smallEnemy.panelDims[0]), 
            .y = -sprites_smallEnemy.panelDims[1], 
            .vy = SMALL_ENEMY_VELOCITY,
            .health = SMALL_ENEMY_HEALTH
        }); 
    }

    if (utils_randomRange(0.0f, 1.0f) < levelState.mediumEnemySpawnProbability * dt) {
        entities_spawn(&entities.mediumEnemies, &(Entities_InitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_mediumEnemy.panelDims[0]), 
            .y = -sprites_mediumEnemy.panelDims[1], 
            .vy = MEDIUM_ENEMY_VELOCITY,
            .health = MEDIUM_ENEMY_HEALTH
        });
    }

    if (utils_randomRange(0.0f, 1.0f) < levelState.largeEnemySpawnProbability * dt) {
        entities_spawn(&entities.largeEnemies, &(Entities_InitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_largeEnemy.panelDims[0]), 
            .y = -sprites_largeEnemy.panelDims[1], 
            .vy = LARGE_ENEMY_VELOCITY,
            .health = LARGE_ENEMY_HEALTH
        });
    }

    // Sim enemies and bullets
    updateEntities(&entities.smallEnemies, dt, 0.0f);
    updateEntities(&entities.mediumEnemies, dt, 0.0f);
    updateEntities(&entities.largeEnemies, dt, 0.0f);
    updateEntities(&entities.playerBullets, dt, 32.0f);
    updateEntities(&entities.enemyBullets, dt, 32.0f);

    // Check for player bullets hitting enemies
    Sprites_CollisionBox* playerBulletCollisionBox = &sprites_playerBullet.collisionBox;
    for (int32_t i = 0; i < entities.playerBullets.count; ++i) {
        float* position = entities.playerBullets.position + i * 2;
        float bulletMin[] = {
            position[0] + playerBulletCollisionBox->min[0],
            position[1] + playerBulletCollisionBox->min[1]
        };
        float bulletMax[] = {
            position[0] + playerBulletCollisionBox->max[0],
            position[1] + playerBulletCollisionBox->max[1]
        };
        
        if (checkPlayerBulletCollision(bulletMin, bulletMax, &entities.smallEnemies, SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET, SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET, SMALL_ENEMY_POINTS) ||
            checkPlayerBulletCollision(bulletMin, bulletMax, &entities.mediumEnemies, SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET, SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET, MEDIUM_ENEMY_POINTS) ||
            checkPlayerBulletCollision(bulletMin, bulletMax, &entities.largeEnemies, SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET, SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET, LARGE_ENEMY_POINTS)) {
            entities.playerBullets.dead[i] = true;
        }  
    }
}

static void simPlayer(float dt) {
    platform_getInput(&gameState.input);

    // Process player input
    Player* player = &entities.player;
    player->velocity[0] = PLAYER_VELOCITY * gameState.input.velocity[0];
    player->velocity[1] = -PLAYER_VELOCITY * gameState.input.velocity[1];

    if (gameState.input.shoot && !gameState.input.lastShoot) {
        firePlayerBullet(player->position[0] + SPRITES_PLAYER_BULLET_X_OFFSET, player->position[1] + SPRITES_PLAYER_BULLET_Y_OFFSET);
    }

    if (player->velocity[0] < -1.0f) {
        entities_setAnimation(&player->entity, 0, SPRITES_PLAYER_LEFT);
    } else if (player->velocity[0] < 0.0f) {
        entities_setAnimation(&player->entity, 0, SPRITES_PLAYER_CENTER_LEFT);
    } else if (player->velocity[0] > 1.0f) {
        entities_setAnimation(&player->entity, 0, SPRITES_PLAYER_RIGHT);
    } else if (player->velocity[0] > 0.0f) {
        entities_setAnimation(&player->entity, 0, SPRITES_PLAYER_CENTER_RIGHT);
    } else {
        entities_setAnimation(&player->entity, 0, SPRITES_PLAYER_CENTER);
    }

    player->position[0] += player->velocity[0] * dt;
    player->position[1] += player->velocity[1] * dt;

    if (player->position[0] < 0.0f) {
        player->position[0] = 0.0f;
    }

    if (player->position[0] + player->sprite->panelDims[0] > GAME_WIDTH) {
        player->position[0] = GAME_WIDTH - player->sprite->panelDims[0];
    }

    if (player->position[1] < 0.0f) {
        player->position[1] = 0.0f;
    }

    if (player->position[1] + player->sprite->panelDims[1] > GAME_HEIGHT) {
        player->position[1] = GAME_HEIGHT - player->sprite->panelDims[1];
    }

    // Fire enemy bullets
    // NOTE(Tarek): This logic is in simPlayer because bullets
    //   should only fire if player is alive
    for (int32_t i = 0; i < entities.smallEnemies.count; ++i) {
        if (utils_randomRange(0.0f, 1.0f) < SMALL_ENEMY_BULLET_PROBABILITY * dt) {
            float* position = entities.smallEnemies.position + i * 2;
            fireEnemyBullet(position[0] + SPRITES_SMALL_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_SMALL_ENEMY_BULLET_Y_OFFSET);
        }
    }

    for (int32_t i = 0; i < entities.mediumEnemies.count; ++i) {
        if (utils_randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_BULLET_PROBABILITY * dt) {
            float* position = entities.mediumEnemies.position + i * 2;
            fireEnemyBullet(position[0] + SPRITES_MEDIUM_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_MEDIUM_ENEMY_BULLET_Y_OFFSET);
        }
    }

    for (int32_t i = 0; i < entities.largeEnemies.count; ++i) {
        if (utils_randomRange(0.0f, 1.0f) < LARGE_ENEMY_BULLET_PROBABILITY * dt) {
            float* position = entities.largeEnemies.position + i * 2;
            fireEnemyBullet(position[0] + SPRITES_LARGE_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_LARGE_ENEMY_BULLET_Y_OFFSET);
        }
    }

    if (player->invincibleTimer > 0.0f) {
        // Player is invincible (grace period after dying)
        player->invincibleTimer -= dt;
        player->alpha[0] = PLAYER_INVINCIBLE_ALPHA;
    } else {
        // Player is alive and not invincible.
        // Check collisions with bullets/enemies.
        player->alpha[0] = 1.0f;
        Sprites_CollisionBox* playerCollisionBox = &sprites_player.collisionBox;
        float playerMin[] = {
            player->position[0] + playerCollisionBox->min[0],
            player->position[1] + playerCollisionBox->min[1]
        };
        float playerMax[] = {
            player->position[0] + playerCollisionBox->max[0],
            player->position[1] + playerCollisionBox->max[1]
        };

        bool bulletHit = checkPlayerCollision(playerMin, playerMax, &entities.enemyBullets, NULL);
        bool smallEnemyHit = checkPlayerCollision(playerMin, playerMax, &entities.smallEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_SMALL_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_SMALL_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool mediumEnemyHit = checkPlayerCollision(playerMin, playerMax, &entities.mediumEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_MEDIUM_ENEMY_EXPLOSION_Y_OFFSET
        });
        bool largeEnemyHit = checkPlayerCollision(playerMin, playerMax, &entities.largeEnemies, &(PlayerCollisionExplosionOptions) {
            .xOffset = SPRITES_LARGE_ENEMY_EXPLOSION_X_OFFSET,
            .yOffset = SPRITES_LARGE_ENEMY_EXPLOSION_Y_OFFSET
        });

        if (bulletHit || smallEnemyHit || mediumEnemyHit || largeEnemyHit) {
            entities_spawn(&entities.explosions, &(Entities_InitOptions) {
                .x = player->position[0] + SPRITES_PLAYER_EXPLOSION_X_OFFSET,
                .y = player->position[1] + SPRITES_PLAYER_EXPLOSION_Y_OFFSET 
            });

            platform_playSound(&gameData.sounds.explosion, false);
            player->position[0] = GAME_WIDTH / 2 - player->sprite->panelDims[0] / 2;
            player->position[1] = GAME_HEIGHT - player->sprite->panelDims[0] * 3.0f;
            player->deadTimer = PLAYER_DEAD_TIME;
            --player->lives;
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
    events_beforeFrame(&events_instructionSequence, dt);

    platform_getInput(&gameState.input);
    
    updateStars(dt);

    entities.text.count = 0;

    if (events_on(&events_titleControlSequence, EVENTS_TITLE)) {
        events_start(&events_titleSequence);
    }

    if (events_on(&events_titleControlSequence, EVENTS_SUBTITLE)) {
        events_start(&events_subtitleSequence);
    }

    if (events_on(&events_titleSequence, EVENTS_DISPLAY)) {
        entities_fromText(&entities.text, "space-shooter.c", &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 127.0f,
            .y = 62.0f, 
            .scale = 0.75f
        });
    }

    if (events_on(&events_titleSequence, EVENTS_FADE)) {
        entities_fromText(&entities.text, "space-shooter.c", &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 127.0f,
            .y = 62.0f, 
            .scale = 0.75f,
            .transparency = events_titleSequence.alpha
        });
    }

    if (events_on(&events_subtitleSequence, EVENTS_DISPLAY)) {
        entities_fromText(&entities.text, "by Tarek Sherif", &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 64.0f,
            .y = 85.0f, 
            .scale = 0.4f
        });
    }

    if (events_on(&events_subtitleSequence, EVENTS_FADE)) {
        entities_fromText(&entities.text, "by Tarek Sherif", &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 64.0f,
            .y = 85.0f, 
            .scale = 0.4f,
            .transparency = events_subtitleSequence.alpha
        });
    }

    if (events_on(&events_instructionSequence, EVENTS_DISPLAY)) {
        float yBase = 57.0f;

        const char* movementText = gameState.input.keyboard ? "Arrow keys to move" : "Left stick to move";
        float movementXOffset = gameState.input.keyboard ? 62.0f : 62.0f;
        entities_fromText(&entities.text, movementText, &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - movementXOffset,
            .y = yBase, 
            .scale = 0.3f
        });

        const char* shootText = gameState.input.keyboard ? "'Space' to shoot" : "'A' to shoot";
        float shootXOffset = gameState.input.keyboard ? 58.0f : 42.0f;
        entities_fromText(&entities.text, shootText, &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - shootXOffset,
            .y = yBase + 13.0f, 
            .scale = 0.3f
        });

        const char* fullscreenText = gameState.input.keyboard ? "'F' to toggle fullscreen" : "'Start' to toggle fullscreen";
        float fullscreenXOffset = gameState.input.keyboard ? 82.0f : 97.0f;
        entities_fromText(&entities.text, fullscreenText, &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - fullscreenXOffset,
            .y = yBase + 26.0f, 
            .scale = 0.3f
        });

        const char* quitText = gameState.input.keyboard ? "'ESC' to quit" : "'Back' to quit";
        float quitXOffset = gameState.input.keyboard ? 47.0f : 50.0f;
        entities_fromText(&entities.text, quitText, &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - quitXOffset,
            .y = yBase + 39.0f, 
            .scale = 0.3f
        });
    }

    if (events_titleSequence.complete && events_subtitleSequence.complete) {
        events_start(&events_instructionSequence);
        events_stop(&events_titleSequence);
        events_stop(&events_subtitleSequence);
    }

    if (gameState.input.shoot || events_instructionSequence.complete) {
        entities.text.count = 0;
        transitionLevel();
    }

    updateAnimations();
    filterDeadEntities();
}

static void levelTransition(float dt) {
    events_beforeFrame(&events_levelTransitionSequence, dt);
    entities.text.count = 0;

    // NOTE(Tarek): Not offsetting level because people probably won't get past level 9?
    if (events_on(&events_levelTransitionSequence, EVENTS_DISPLAY)) {
        entities_fromText(&entities.text, levelState.title, &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 62.0f,
            .y = 64.0f, 
            .scale = 0.75f
        });

        float xOffset = levelState.level == 1 ? 58.0f : 56.0f; // Thinner '1' needs to be further left ot look good
        xOffset += (levelState.nextLevelTextLength - BASE_NEXT_LEVEL_TEXT_LENGTH) * 3.0f;
        entities_fromText(&entities.text, levelState.nextLevelText, &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - xOffset,
            .y = 88.0f, 
            .scale = 0.25f
        });
    }

    livesToEntities();
    updateStars(dt);
    updateEntities(&entities.smallEnemies, dt, 0.0f);
    updateEntities(&entities.mediumEnemies, dt, 0.0f);
    updateEntities(&entities.largeEnemies, dt, 0.0f);
    updateEntities(&entities.playerBullets, dt, 32.0f);
    updateEntities(&entities.enemyBullets, dt, 32.0f);
    updateEntities(&entities.explosions, dt, 32.0f);

    if (events_levelTransitionSequence.complete) {
        entities.text.count = 0;
        levelState.warpVy = 0.0f;
        levelState.starProbabilityMultiplier = 1.0f;
        gameState.state = MAIN_GAME;
    }

    updateScoreDisplay();
    updateAnimations();
    filterDeadEntities();
}

static void mainGame(float dt) {
    entities.text.count = 0;
    
    simWorld(dt);
    livesToEntities();

    Player* player = &entities.player;

    if (player->bulletThrottle > 0.0f) {
        player->bulletThrottle -= dt; 
    }

    if (player->lives > 0) {
        if (player->deadTimer > 0.0f) {
            player->deadTimer -= dt;
            if (player->deadTimer < 0.0f) {
                player->invincibleTimer = PLAYER_INVINCIBLE_TIME;
                player->alpha[0] = PLAYER_INVINCIBLE_ALPHA;
            }
        } else {
            simPlayer(dt);           
        }

        if (player->score >= levelState.scoreThreshold) {
            ++levelState.level;
            levelState.scoreThreshold *= 2;
            levelState.smallEnemySpawnProbability *= LEVEL_SPAWN_PROBABILITY_MULTIPLIER;
            levelState.mediumEnemySpawnProbability *= LEVEL_SPAWN_PROBABILITY_MULTIPLIER;
            levelState.largeEnemySpawnProbability *= LEVEL_SPAWN_PROBABILITY_MULTIPLIER;
            transitionLevel();
        }
    } else {
        events_start(&events_gameOverSequence);
        gameState.state = GAME_OVER;
    }

    updateScoreDisplay();
    updateAnimations();
    filterDeadEntities();
}

static void gameOver(float dt) {
    events_beforeFrame(&events_gameOverSequence, dt);
    events_beforeFrame(&events_gameOverRestartSequence, dt);
    entities.text.count = 0;

    platform_getInput(&gameState.input);

    simWorld(dt);

    entities_fromText(&entities.text, "Game Over", &(Entities_FromTextOptions) {
        .x = GAME_WIDTH / 2.0f - 127.0f,
        .y = 68.0f,
        .scale = 1.2f
    });

    if (events_on(&events_gameOverSequence, EVENTS_RESTART)) {
        events_start(&events_gameOverRestartSequence);
    }

    if (events_on(&events_gameOverRestartSequence, EVENTS_DISPLAY)) {
        const char* text = gameState.input.keyboard ? "Press 'Space' to Restart" : "Press 'A' to Restart";
        float xOffset = gameState.input.keyboard ? 107.0f : 91.0f;
        entities_fromText(&entities.text, text, &(Entities_FromTextOptions) {
            .x = GAME_WIDTH / 2.0f - xOffset,
            .y = 104.0f,
            .scale = 0.4f
        }); 
    }

    updateScoreDisplay();
    updateAnimations();
    filterDeadEntities();

    if (events_gameOverRestartSequence.running && gameState.input.shoot) {
        entities.player.lives = PLAYER_NUM_LIVES;
        entities.player.score = 0;
        entities.player.deadTimer = 0.0f;
        entities.smallEnemies.count = 0;
        entities.mediumEnemies.count = 0;
        entities.largeEnemies.count = 0;
        entities.enemyBullets.count = 0;
        levelState.level = 1;
        levelState.scoreThreshold = INITIAL_LEVEL_SCORE_THRESHOLD;
        levelState.smallEnemySpawnProbability = SMALL_ENEMY_INITIAL_SPAWN_PROBABILITY;
        levelState.mediumEnemySpawnProbability = MEDIUM_ENEMY_INITIAL_SPAWN_PROBABILITY;
        levelState.largeEnemySpawnProbability = LARGE_ENEMY_INITIAL_SPAWN_PROBABILITY;
        events_stop(&events_gameOverSequence);
        events_stop(&events_gameOverRestartSequence);
        transitionLevel();
    }

}

//////////////////////////////////
//  Main state update function
//////////////////////////////////

static void simulate(float dt) {
    gameState.animationTime += dt;

    switch(gameState.state) {
        case TITLE_SCREEN: titleScreen(dt); break;
        case LEVEL_TRANSITION: levelTransition(dt); break;
        case MAIN_GAME: mainGame(dt); break;
        case GAME_OVER: gameOver(dt); break;
    }

    if (gameState.animationTime > TIME_PER_ANIMATION) {
        gameState.animationTime = 0.0f;
    }
}

//////////////////////////////////
//  Platform interface functions
//////////////////////////////////

bool game_init(void) {
    // Init subsystems
    utils_init();
    
    if (!renderer_init(GAME_WIDTH, GAME_HEIGHT)) {
        platform_userMessage("FATAL ERROR: Unable to initialize renderer.");
        return false;
    }

    // Load assets
    if (
        !loadTexture("assets/sprites/ship.bmp", &sprites_player.texture) ||
        !loadTexture("assets/sprites/enemy-small.bmp", &sprites_smallEnemy.texture) ||
        !loadTexture("assets/sprites/enemy-medium.bmp", &sprites_mediumEnemy.texture) ||
        !loadTexture("assets/sprites/enemy-big.bmp", &sprites_largeEnemy.texture) ||
        !loadTexture("assets/sprites/explosion.bmp", &sprites_explosion.texture) ||
        !loadTexture("assets/sprites/pixelspritefont32.bmp", &sprites_text.texture) ||
        !loadTexture("assets/sprites/laser-bolts.bmp", &sprites_playerBullet.texture)
    ) {
        platform_userMessage("FATAL ERROR: Unable to load textures.");
        return false;
    }

    sprites_whitePixel.texture = renderer_createTexture(gameData.whitePixel, 1, 1);

    // Shared texture    
    sprites_enemyBullet.texture = sprites_playerBullet.texture;

    if (!renderer_validate()) {
        platform_userMessage("FATAL ERROR: Unable to allocate textures.");
        return false;
    }

    if (
        !loadSound("assets/audio/music.wav", &gameData.sounds.music) ||
        !loadSound("assets/audio/Laser_002.wav", &gameData.sounds.playerBullet) ||
        !loadSound("assets/audio/Hit_Hurt2.wav", &gameData.sounds.enemyBullet) ||
        !loadSound("assets/audio/Explode1.wav", &gameData.sounds.explosion) ||
        !loadSound("assets/audio/Jump1.wav", &gameData.sounds.enemyHit)
    ) {
        platform_userMessage("Unable to load all audio.");
    }

    // Init game
    platform_playSound(&gameData.sounds.music, true);

    entities_spawn(&entities.player.entity, & (Entities_InitOptions) {
        .x = (GAME_WIDTH - entities.player.sprite->panelDims[0]) / 2,
        .y = GAME_HEIGHT - entities.player.sprite->panelDims[1] * 2.0f
    });

    entities_setAnimation(&entities.player.entity, 0, SPRITES_PLAYER_CENTER);

    for (int32_t i = 0; i < 40; ++i) {
        float t = utils_randomRange(0.0f, 1.0f);
        entities_spawn(&entities.stars, &(Entities_InitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_whitePixel.panelDims[0]), 
            .y = utils_randomRange(0.0f, GAME_HEIGHT - sprites_whitePixel.panelDims[1]), 
            .vy = utils_lerp(STARS_MIN_VELOCITY, STARS_MAX_VELOCITY, t),
            .transparency = utils_lerp(STARS_MIN_TRANSPARENCY, STARS_MAX_TRANSPARENCY, 1.0f - t)
        });
    }


    utils_uintToString(0, gameState.scoreText, SCORE_TEXT_LENGTH);
    updateLevelTitle();

    events_start(&events_titleControlSequence);

    return true;
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

    gameState.tickTime += elapsedTime;

    if (gameState.tickTime > TICK_DURATION) {
        while (gameState.tickTime > TICK_DURATION) {
            simulate(TICK_DURATION);    
            gameState.tickTime -= TICK_DURATION;
        }

        simulate(gameState.tickTime);
        gameState.tickTime = 0.0f;
    }
}

void game_resize(int width, int height) {
    renderer_resize(width, height);
    game_draw();
}

void game_draw(void) {
    renderer_beforeFrame();

    renderer_draw(&entities.stars.renderList);

    if (entities.player.deadTimer <= 0.0f) {
        renderer_draw(&entities.player.renderList);
    }

    renderer_draw(&entities.explosions.renderList);
    renderer_draw(&entities.smallEnemies.renderList);
    renderer_draw(&entities.mediumEnemies.renderList);
    renderer_draw(&entities.largeEnemies.renderList);
    renderer_draw(&entities.enemyBullets.renderList);
    renderer_draw(&entities.playerBullets.renderList);
    renderer_draw(&entities.text.renderList);
    renderer_draw(&entities.lives.renderList);
}

void game_close(void) {
    data_freeBuffer(&gameData.sounds.music);
    data_freeBuffer(&gameData.sounds.playerBullet);
    data_freeBuffer(&gameData.sounds.enemyBullet);
    data_freeBuffer(&gameData.sounds.explosion);
    data_freeBuffer(&gameData.sounds.enemyHit);
}
