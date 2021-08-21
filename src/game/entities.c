#include "entities.h"

extern void entities_updateAnimationPanel(EntitiesEntity* entity) {
    uint8_t* panel = entity->sprite->animations[entity->currentAnimation].frames[entity->animationTick];

    entity->currentSpritePanel[0] = panel[0];
    entity->currentSpritePanel[1] = panel[1];
}

extern void entities_setAnimation(EntitiesEntity* entity, uint8_t animation) {
    if (entity->currentAnimation == animation) {
        return;
    }

    entity->currentAnimation = animation;
    entity->animationTick = 0;
    entities_updateAnimationPanel(entity);
}

extern void entities_updateAnimations(EntitiesList* list) {
    for (uint8_t i = 0; i < list->count; ++i) {
        uint8_t numFrames = list->entities[i].sprite->animations[list->entities[i].currentAnimation].numFrames;
        ++list->entities[i].animationTick;
        if (list->entities[i].animationTick == numFrames) {
            Sprites_AnimationEndBehavior endBehavior = list->entities[i].sprite->animations[list->entities[i].currentAnimation].endBehavior;

            if (endBehavior == SPRITES_ANIMATION_END_KILL) {
                entities_kill(list, i);
                continue;
            } else {
                list->entities[i].animationTick = 0; 
            }
        }

        entities_updateAnimationPanel(&list->entities[i]);
    }
}

extern void entities_spawn(EntitiesList* list, Sprites_Sprite* sprite, EntitiesSpawnOptions* opts) {
    if (list->count == RENDERER_DRAWLIST_MAX) {
        return;
    }

    EntitiesEntity* newEntity = list->entities + list->count;

    newEntity->position = sprite->positions + list->count * 2;
    newEntity->currentSpritePanel = sprite->currentSpritePanels + list->count * 2;
    newEntity->whiteOut = sprite->whiteOut + list->count;
    newEntity->scale = sprite->scale + list->count;

    newEntity->position[0] = opts->x; 
    newEntity->position[1] = opts->y;
    newEntity->velocity[0] = opts->vx;
    newEntity->velocity[1] = opts->vy;
    newEntity->sprite = sprite;
    newEntity->currentAnimation = opts->currentAnimation;
    newEntity->animationTick = 0;
    newEntity->scale[0] = opts->scale;
    newEntity->health = opts->health;

    entities_updateAnimationPanel(newEntity);

    ++list->count;
}

extern void entities_kill(EntitiesList* list, uint8_t i) {
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

extern void updateEntityAnimations(EntitiesList* list) {
    for (uint8_t i = 0; i < list->count; ++i) {
        uint8_t numFrames = list->entities[i].sprite->animations[list->entities[i].currentAnimation].numFrames;
        ++list->entities[i].animationTick;
        if (list->entities[i].animationTick == numFrames) {
            Sprites_AnimationEndBehavior endBehavior = list->entities[i].sprite->animations[list->entities[i].currentAnimation].endBehavior;

            if (endBehavior == SPRITES_ANIMATION_END_KILL) {
                entities_kill(list, i);
                continue;
            } else {
                list->entities[i].animationTick = 0; 
            }
        }

        entities_updateAnimationPanel(&list->entities[i]);
    }
}

extern void entities_fromText(EntitiesList* list, Sprites_Sprite* sprite, float x, float y, const char* text, float scale) {
    uint8_t i = 0;

    while (text[i] && list->count < RENDERER_DRAWLIST_MAX) {
        int8_t animationIndex = sprites_charToAnimationIndex(text[i]);

        if (animationIndex < 0) {
            ++i;
            continue;
        }

        entities_spawn(list, sprite, &(EntitiesSpawnOptions) { 
            .x = x + i * sprite->panelDims[0] * scale * SPRITES_TEXT_SPACING_SCALE,
            .y = y,
            .scale = scale,
            .currentAnimation = animationIndex
        });

        ++i;
    }

}