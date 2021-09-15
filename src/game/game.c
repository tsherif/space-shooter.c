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
#include "events.h"

#define GAME_WIDTH 320
#define GAME_HEIGHT 180

#define SHIP_VELOCITY 0.5f
#define SHIP_BULLET_VELOCITY (-1.5f)
#define SHIP_BULLET_THROTTLE 20
#define SHIP_DEAD_COUNTER 500

#define SMALL_ENEMY_VELOCITY 0.2f
#define SMALL_ENEMY_SPAWN_PROBABILITY 0.002f
#define SMALL_ENEMY_BULLET_PROBABILITY 0.002f
#define SMALL_ENEMY_HEALTH 1
#define SMALL_ENEMY_POINTS 1

#define MEDIUM_ENEMY_VELOCITY 0.1f
#define MEDIUM_ENEMY_SPAWN_PROBABILITY 0.0006f
#define MEDIUM_ENEMY_BULLET_PROBABILITY 0.005f
#define MEDIUM_ENEMY_HEALTH 5
#define MEDIUM_ENEMY_POINTS 5

#define LARGE_ENEMY_VELOCITY 0.04f
#define LARGE_ENEMY_SPAWN_PROBABILITY 0.0002f
#define LARGE_ENEMY_BULLET_PROBABILITY 0.02f
#define LARGE_ENEMY_HEALTH 10
#define LARGE_ENEMY_POINTS 25

#define ENEMY_BULLET_SPEED 0.3f

#define COLLISION_SCALE 0.7f

typedef struct {
    MIXIN_STRUCT(EntitiesList, entity);
    uint16_t bulletThrottle;
    uint16_t deadCounter;
    uint16_t score;
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

static Player player = { .sprite = &sprites_ship };
static EntitiesList smallEnemies = { .sprite = &sprites_smallEnemy };
static EntitiesList mediumEnemies = { .sprite = &sprites_mediumEnemy };
static EntitiesList largeEnemies = { .sprite = &sprites_largeEnemy };
static EntitiesList playerBullets = { .sprite = &sprites_playerBullet };
static EntitiesList enemyBullets = { .sprite = &sprites_enemyBullet };
static EntitiesList explosions = { .sprite = &sprites_explosion };
static EntitiesList textEntities = { .sprite = &sprites_text };

static enum {
    TITLE,
    MAIN_GAME
} gameState = TITLE;

#define SCORE_BUFFER_LENGTH 5 
static char scoreString[SCORE_BUFFER_LENGTH];

static uint32_t gameTick = 0;


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
            .delay = 250,
            .id =  TITLE_START
        },
        { 
            .delay = 250,
            .id =  SUBTITLE_START
        }
    },
    .count = 2
};

static EventsSequence titleSequence = {
    .events = {
        { 
            .duration = 300,
            .id =  TITLE_DISPLAY
        },
        {
            .duration = 400,
            .id = TITLE_FADE
        }
    },
    .count = 2
};

static EventsSequence subtitleSequence = {
    .events = {
        { 
            .duration = 300,
            .id =  SUBTITLE_DISPLAY
        },
        {
            .duration = 400,
            .id = SUBTITLE_FADE
        }
    },
    .count = 2
};

static void uint16ToString(uint16_t n, char* buffer, int8_t bufferLength) {
    buffer[bufferLength - 1] = '\0';
    int8_t i = bufferLength - 2;

    while (i >= 0) {
        buffer[i] = '0' + (n % 10);
        n /= 10;
        --i;
    }
}

static float randomRange(float min, float max) {
    float range = max - min;
    return min + ((float) rand() / (RAND_MAX + 1)) * range;
}

static void firePlayerBullet(float x, float y) {
    if (
        player.bulletThrottle > 0 || 
        player.deadCounter > 0
    ) {
        return;
    }

    entities_spawn(&playerBullets, &(EntitiesInitOptions) {
        .x = x, 
        .y = y, 
        .vy = SHIP_BULLET_VELOCITY
    });
    platform_playSound(shipBulletSound, false);
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

    platform_playSound(enemyBulletSound, false);
}

static void updateEntities(EntitiesList* list, float killBuffer) {
    for (uint8_t i = 0; i < list->count; ++i) {
        float* position = list->position + i * 2;
        float* velocity = list->velocity + i * 2;
        position[0] += velocity[0];
        position[1] += velocity[1];

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
    uint8_t points
) {
    bool hit = false;
    Sprites_CollisionBox* enemyCollisionBox = &enemies->sprite->collisionBox;
    for (uint8_t i = 0; i < enemies->count; ++i) {
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
                platform_playSound(explosionSound, false);
                entities_kill(enemies, i);
                player.score += points;
            } else {
                platform_playSound(enemyHit, false);
                enemies->whiteOut[i] = 1;
            }
        }    
    } 

    return hit;
}

static bool checkPlayerCollision(float playerMin[2], float playerMax[2], EntitiesList* list, PlayerCollisionExplosionOptions* opts) {
    bool playerHit = false;
    Sprites_CollisionBox* collisionBox = &list->sprite->collisionBox;
    for (uint8_t i = 0; i < list->count; ++i) {
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

    music = platform_loadSound("assets/audio/music.wav");
    shipBulletSound = platform_loadSound("assets/audio/Laser_002.wav");
    enemyBulletSound = platform_loadSound("assets/audio/Hit_Hurt2.wav");
    explosionSound = platform_loadSound("assets/audio/Explode1.wav");
    enemyHit = platform_loadSound("assets/audio/Jump1.wav");

    platform_playSound(music, true);

    entities_spawn(&player.entity, & (EntitiesInitOptions) {
        .x = (GAME_WIDTH - player.sprite->panelDims[0]) / 2,
        .y = GAME_HEIGHT - player.sprite->panelDims[1] * 2.0f
    });

    renderer_init(GAME_WIDTH, GAME_HEIGHT);

    renderer_loadTexture("assets/sprites/ship.png", &player.texture);
    renderer_loadTexture("assets/sprites/enemy-small.png", &smallEnemies.texture);
    renderer_loadTexture("assets/sprites/enemy-medium.png", &mediumEnemies.texture);
    renderer_loadTexture("assets/sprites/enemy-big.png", &largeEnemies.texture);
    renderer_loadTexture("assets/sprites/explosion.png", &explosions.texture);
    renderer_loadTexture("assets/sprites/pixelspritefont32.png", &textEntities.texture);

    GLuint bulletTexture; // Shared between player and enemy bullets.
    renderer_loadTexture("assets/sprites/laser-bolts.png", &bulletTexture);
    playerBullets.texture = bulletTexture;
    enemyBullets.texture = bulletTexture;

    entities_setAnimation(&player.entity, 0, SPRITES_SHIP_CENTER);

    uint16ToString(0, scoreString, SCORE_BUFFER_LENGTH);

    events_start(&titleControls);
}

static void titleScreen(uint32_t tick) {
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
        EventsEvent* event = titleSequence.events + titleSequence.current;
        entities_fromText(&textEntities, "space-shooter.c", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 127.0f,
            .y = 62.0f, 
            .scale = 0.75f,
            .transparency = event->t
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
        EventsEvent* event = subtitleSequence.events + subtitleSequence.current;
        entities_fromText(&textEntities, "by Tarek Sherif", &(EntitiesFromTextOptions) {
            .x = GAME_WIDTH / 2.0f - 64.0f,
            .y = 85.0f, 
            .scale = 0.4f,
            .transparency = event->t
        });
    }

    if (titleSequence.complete && subtitleSequence.complete) {
        textEntities.count = 0;
        gameState = MAIN_GAME;
    }

    if (tick % 20 == 0) {
        entities_updateAnimations(&player.entity);  
    }

    events_update(&titleControls);
    events_update(&titleSequence);
    events_update(&subtitleSequence);
}


static void mainGame(uint32_t tick) {
    if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_SPAWN_PROBABILITY) {
        entities_spawn(&smallEnemies, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_smallEnemy.panelDims[0]), 
            .y = -sprites_smallEnemy.panelDims[1], 
            .vy = SMALL_ENEMY_VELOCITY,
            .health = SMALL_ENEMY_HEALTH
        }); 
    }

    if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_SPAWN_PROBABILITY) {
        entities_spawn(&mediumEnemies, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_mediumEnemy.panelDims[0]), 
            .y = -sprites_mediumEnemy.panelDims[1], 
            .vy = MEDIUM_ENEMY_VELOCITY,
            .health = MEDIUM_ENEMY_HEALTH
        });
    }

    if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_SPAWN_PROBABILITY) {
        entities_spawn(&largeEnemies, &(EntitiesInitOptions) {
            .x = randomRange(0.0f, GAME_WIDTH - sprites_largeEnemy.panelDims[0]), 
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

    if (player.bulletThrottle > 0) {
        --player.bulletThrottle; 
    }

    if (player.deadCounter > 0) {
        --player.deadCounter;
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

        player.position[0] += player.velocity[0];
        player.position[1] += player.velocity[1];

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

        for (uint8_t i = 0; i < smallEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < SMALL_ENEMY_BULLET_PROBABILITY) {
                float* position = smallEnemies.position + i * 2;
                fireEnemyBullet(position[0] + SPRITES_SMALL_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_SMALL_ENEMY_BULLET_Y_OFFSET);
            }
        }

        for (uint8_t i = 0; i < mediumEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < MEDIUM_ENEMY_BULLET_PROBABILITY) {
                float* position = mediumEnemies.position + i * 2;
                fireEnemyBullet(position[0] + SPRITES_MEDIUM_ENEMY_BULLET_X_OFFSET, position[1] + SPRITES_MEDIUM_ENEMY_BULLET_Y_OFFSET);
            }
        }

        for (uint8_t i = 0; i < largeEnemies.count; ++i) {
            if (randomRange(0.0f, 1.0f) < LARGE_ENEMY_BULLET_PROBABILITY) {
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

            platform_playSound(explosionSound, false);
            player.position[0] = GAME_WIDTH / 2 - player.sprite->panelDims[0] / 2;
            player.position[1] = GAME_HEIGHT - player.sprite->panelDims[0] * 3.0f;
            player.deadCounter = SHIP_DEAD_COUNTER;
                    
        }
    }

    uint16ToString(player.score, scoreString, SCORE_BUFFER_LENGTH);
    entities_fromText(&textEntities, scoreString, &(EntitiesFromTextOptions) {
        .x = 10.0f,
        .y = GAME_HEIGHT - 20.0f, 
        .scale = 0.4f,
        .reset = true
    });

    if (tick % 20 == 0) {
        entities_updateAnimations(&player.entity);  
        entities_updateAnimations(&playerBullets);  
        entities_updateAnimations(&smallEnemies);
        entities_updateAnimations(&mediumEnemies);
        entities_updateAnimations(&largeEnemies);
        entities_updateAnimations(&enemyBullets);  
        entities_updateAnimations(&explosions);  
    }
}

void game_update() {
    if (gameState == TITLE) {
        titleScreen(gameTick);
    } else {
        mainGame(gameTick);  
    }

    ++gameTick;
}

void game_resize(int width, int height) {
    renderer_resize(width, height);
    game_draw();
}

void game_keyboard(GameKeyboard* inputKeys) {
    if (inputKeys->left) {
        player.velocity[0] = -SHIP_VELOCITY;
    } else if (inputKeys->right) {
        player.velocity[0] = SHIP_VELOCITY;
    } else {
        player.velocity[0] = 0.0f;
    }

    if (inputKeys->up) {
        player.velocity[1] = -SHIP_VELOCITY;
    } else if (inputKeys->down) {
        player.velocity[1] = SHIP_VELOCITY;
    } else {
        player.velocity[1] = 0.0f;
    }

    if (inputKeys->space && gameState == MAIN_GAME) {
        firePlayerBullet(player.position[0] + SPRITES_SHIP_BULLET_X_OFFSET, player.position[1] + SPRITES_SHIP_BULLET_Y_OFFSET);
    }
}

void game_controller(GameController* controllerInput) {
    player.velocity[0] = SHIP_VELOCITY * controllerInput->leftStickX;
    player.velocity[1] = -SHIP_VELOCITY * controllerInput->leftStickY;

    if (controllerInput->aButton && gameState == MAIN_GAME) {
        firePlayerBullet(player.position[0] + SPRITES_SHIP_BULLET_X_OFFSET, player.position[1] + SPRITES_SHIP_BULLET_Y_OFFSET);
    }
}

void game_draw(void) {
    renderer_beforeFrame();

    if (player.deadCounter == 0) {
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
