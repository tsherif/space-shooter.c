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

#define GAME_WIDTH 320
#define GAME_HEIGHT 180

#define TICK_DURATION 16.6667f

#define SHIP_VELOCITY 0.075f
#define SHIP_BULLET_VELOCITY (-0.2f)
#define SHIP_BULLET_THROTTLE 100.0f
#define SHIP_DEAD_TIME 2000.0f
#define SHIP_NUM_LIVES 3

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
#define ENEMY_WHITEOUT_TIME 25.0f

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
    ENTITIES_LIST_MIXIN(entity);
    float bulletThrottle;
    float deadTimer;
    int32_t score;
    int32_t lives;
} Player;

typedef struct {
    float xOffset;
    float yOffset;
} PlayerCollisionExplosionOptions;

static DataBuffer music;
static DataBuffer shipBulletSound;
static DataBuffer enemyBulletSound;
static DataBuffer explosionSound;
static DataBuffer enemyHit;

static Player player = { .sprite = &sprites_ship, .lives = SHIP_NUM_LIVES };
static EntitiesList smallEnemies = { .sprite = &sprites_smallEnemy };
static EntitiesList mediumEnemies = { .sprite = &sprites_mediumEnemy };
static EntitiesList largeEnemies = { .sprite = &sprites_largeEnemy };
static EntitiesList playerBullets = { .sprite = &sprites_playerBullet };
static EntitiesList enemyBullets = { .sprite = &sprites_enemyBullet };
static EntitiesList explosions = { .sprite = &sprites_explosion };
static EntitiesList stars = { .sprite = &sprites_whitePixel };
static EntitiesList textEntities = { .sprite = &sprites_text };
static EntitiesList livesEntities = { .sprite = &sprites_ship };

static enum {
    TITLE,
    MAIN_GAME,
    GAME_OVER
} gameState = TITLE;

#define SCORE_BUFFER_LENGTH 5 
static char scoreString[SCORE_BUFFER_LENGTH];

static uint8_t whitePixelData[4] = {255, 255, 255, 255};

static float tickTime = 0.0f;
static float animationTime = 0.0f;

enum {
    NO_EVENT,
    TITLE_START,
    TITLE_DISPLAY,
    TITLE_FADE,
    SUBTITLE_START,
    SUBTITLE_DISPLAY,
    SUBTITLE_FADE,
    GAME_OVER_RESTART,
    GAME_OVER_RESTART_DISPLAY
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

static EventsSequence gameOverSequence = {
    .events = {
        { 
            .delay = 1000.0f,
            .id =  GAME_OVER_RESTART
        }
    },
    .count = 1,
};

static EventsSequence gameOverRestartSequence = {
    .events = {
        { 
            .duration = 1000.0f,
            .id =  GAME_OVER_RESTART_DISPLAY
        },
        { 
            .duration = 1000.0f
        }
    },
    .count = 2,
    .loop = true
};

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
                platform_playSound(&explosionSound, false);
                enemies->dead[i] = true;
                player.score += points;
            } else {
                platform_playSound(&enemyHit, false);
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

static void loadSound(const char* fileName, DataBuffer* sound) {
    DataBuffer soundData = { 0 };
    platform_loadBinFile(fileName, &soundData);
    utils_wavToSound(&soundData, sound);
    data_freeBuffer(&soundData);
}

static void loadTexture(const char* fileName, GLuint *texture) {
    DataBuffer imageData = { 0 };
    DataImage image = { 0 };

    platform_loadBinFile(fileName, &imageData);
    utils_bmpToImage(&imageData, &image);
    renderer_initTexture(texture, image.data, image.width, image.height);
    data_freeBuffer(&imageData);
    data_freeImage(&image);
}

static void updateStars(float dt) {
    if (utils_randomRange(0.0f, 1.0f) < STAR_PROBABILITY * dt) {
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

static void updateScoreDisplay() {
    utils_uintToString(player.score, scoreString, SCORE_BUFFER_LENGTH);
    entities_fromText(&textEntities, scoreString, &(EntitiesFromTextOptions) {
        .x = 10.0f,
        .y = GAME_HEIGHT - 20.0f, 
        .scale = 0.4f
    });
}

static void titleScreen(float dt) {
    events_beforeFrame(&titleControls, dt);
    events_beforeFrame(&titleSequence, dt);
    events_beforeFrame(&subtitleSequence, dt);
    
    updateStars(dt);

    textEntities.count = 0;

    if (events_on(&titleControls, TITLE_START)) {
        events_start(&titleSequence);
    }

    if (events_on(&titleControls, SUBTITLE_START)) {
        events_start(&subtitleSequence);
    }

    if (events_on(&titleSequence, TITLE_DISPLAY)) {
        entities_fromText(&textEntities, "space-shooter.c", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 127.0f,
            .y = 62.0f, 
            .scale = 0.75f
        });
    }

    if (events_on(&titleSequence, TITLE_FADE)) {
        entities_fromText(&textEntities, "space-shooter.c", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 127.0f,
            .y = 62.0f, 
            .scale = 0.75f,
            .transparency = titleSequence.triggeredEvent->alpha
        });
    }

    if (events_on(&subtitleSequence, SUBTITLE_DISPLAY)) {
        entities_fromText(&textEntities, "by Tarek Sherif", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 64.0f,
            .y = 85.0f, 
            .scale = 0.4f
        });
    }

    if (events_on(&subtitleSequence, SUBTITLE_FADE)) {
        entities_fromText(&textEntities, "by Tarek Sherif", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 64.0f,
            .y = 85.0f, 
            .scale = 0.4f,
            .transparency = subtitleSequence.triggeredEvent->alpha
        });
    }

    if (titleSequence.complete && subtitleSequence.complete) {
        textEntities.count = 0;
        gameState = MAIN_GAME;
    }

    updateAnimations();
}

static void simEnemies(float dt) {
    if (utils_randomRange(0.0f, 1.0f) < SMALL_ENEMY_SPAWN_PROBABILITY * dt) {
        entities_spawn(&smallEnemies, &(EntitiesInitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_smallEnemy.panelDims[0]), 
            .y = -sprites_smallEnemy.panelDims[1], 
            .vy = SMALL_ENEMY_VELOCITY,
            .health = SMALL_ENEMY_HEALTH
        }); 
    }

    if (utils_randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_SPAWN_PROBABILITY * dt) {
        entities_spawn(&mediumEnemies, &(EntitiesInitOptions) {
            .x = utils_randomRange(0.0f, GAME_WIDTH - sprites_mediumEnemy.panelDims[0]), 
            .y = -sprites_mediumEnemy.panelDims[1], 
            .vy = MEDIUM_ENEMY_VELOCITY,
            .health = MEDIUM_ENEMY_HEALTH
        });
    }

    if (utils_randomRange(0.0f, 1.0f) < LARGE_ENEMY_SPAWN_PROBABILITY * dt) {
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

    player.velocity[0] = SHIP_VELOCITY * input.velocity[0];
    player.velocity[1] = -SHIP_VELOCITY * input.velocity[1];

    if (input.shoot) {
        firePlayerBullet(player.position[0] + SPRITES_SHIP_BULLET_X_OFFSET, player.position[1] + SPRITES_SHIP_BULLET_Y_OFFSET);
    }

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
        --player.lives;
    }   
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
        } else {
            simPlayer(dt);           
        }
    } else {
        events_start(&gameOverSequence);
        gameState = GAME_OVER;
    }

    updateScoreDisplay();
    updateAnimations();
    filterDeadEntities();
}

static void gameOver(float dt) {
    events_beforeFrame(&gameOverSequence, dt);
    events_beforeFrame(&gameOverRestartSequence, dt);
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

    if (events_on(&gameOverSequence, GAME_OVER_RESTART)) {
        events_start(&gameOverRestartSequence);
    }

    if (events_on(&gameOverRestartSequence, GAME_OVER_RESTART_DISPLAY)) {
        entities_fromText(&textEntities, "Press 'Shoot' to Restart", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 107.0f,
            .y = 104.0f,
            .scale = 0.4f
        }); 
    }

    updateScoreDisplay();
    updateAnimations();
    filterDeadEntities();

    if (gameOverRestartSequence.running && input.shoot) {
        gameState = MAIN_GAME;
        player.lives = SHIP_NUM_LIVES;
        player.score = 0;
        player.deadTimer = 0.0f;
        smallEnemies.count = 0;
        mediumEnemies.count = 0;
        largeEnemies.count = 0;
        enemyBullets.count = 0;
        events_stop(&gameOverSequence);
        events_stop(&gameOverRestartSequence);
    }

}

static void updateState(float dt) {
    animationTime += dt;

    switch(gameState) {
        case TITLE: titleScreen(dt); break;
        case MAIN_GAME: mainGame(dt); break;
        case GAME_OVER: gameOver(dt); break;
    }

    if (animationTime > TIME_PER_ANIMATION) {
        animationTime = 0.0f;
    }
}

void game_init(void) {
    // Init subsystems
    utils_init();
    renderer_init(GAME_WIDTH, GAME_HEIGHT);

    // Load assets
    loadSound("assets/audio/music.wav", &music);
    loadSound("assets/audio/Laser_002.wav", &shipBulletSound);
    loadSound("assets/audio/Hit_Hurt2.wav", &enemyBulletSound);
    loadSound("assets/audio/Explode1.wav", &explosionSound);
    loadSound("assets/audio/Jump1.wav", &enemyHit);

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
    platform_playSound(&music, true);

    entities_spawn(&player.entity, & (EntitiesInitOptions) {
        .x = (GAME_WIDTH - player.sprite->panelDims[0]) / 2,
        .y = GAME_HEIGHT - player.sprite->panelDims[1] * 2.0f
    });

    entities_setAnimation(&player.entity, 0, SPRITES_SHIP_CENTER);

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

    events_start(&titleControls);
}

void game_update(float elapsedTime) {
    if (elapsedTime > 33.3f) {
        elapsedTime = 33.3f;
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
    renderer_draw(&livesEntities.renderList);
}
