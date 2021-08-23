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

extern void entities_spawn(EntitiesList* list, Sprites_Sprite* sprite, EntitiesInitOptions* opts) {
    if (list->count == RENDERER_DRAWLIST_MAX) {
        return;
    }

    EntitiesEntity* entity = list->entities + list->count;

    entity->sprite = sprite;
    entity->position = sprite->positions + list->count * 2;
    entity->currentSpritePanel = sprite->currentSpritePanels + list->count * 2;
    entity->whiteOut = sprite->whiteOut + list->count;
    entity->scale = sprite->scale + list->count;
    entity->alpha = sprite->alpha + list->count;

    entities_init(entity, opts);

    ++list->count;
}

extern void entities_init(EntitiesEntity* entity, EntitiesInitOptions* opts) {
    entity->position[0] = opts->x; 
    entity->position[1] = opts->y;
    entity->velocity[0] = opts->vx;
    entity->velocity[1] = opts->vy;
    entity->currentAnimation = opts->currentAnimation;
    entity->animationTick = 0;
    entity->scale[0] = opts->scale > 0.0f ? opts->scale : 1.0f;
    entity->alpha[0] = 1.0f - opts->transparency;
    entity->health = opts->health;

    entities_updateAnimationPanel(entity);
}

extern void entities_kill(EntitiesList* list, uint8_t i) {
    if (i >= list->count) {
        return;
    }

    uint8_t last = list->count - 1;

    float* positionPointer = list->entities[i].position;
    float* currentSpritePanelPointer =  list->entities[i].currentSpritePanel;
    uint8_t* whiteOutPointer = list->entities[i].whiteOut;
    float* scalePointer = list->entities[i].scale;
    float* alphaPointer = list->entities[i].alpha;

    positionPointer[0] = list->entities[last].position[0];
    positionPointer[1] = list->entities[last].position[1];
    currentSpritePanelPointer[0] = list->entities[last].currentSpritePanel[0];
    currentSpritePanelPointer[1] = list->entities[last].currentSpritePanel[1];
    whiteOutPointer[0] = list->entities[last].whiteOut[0];
    scalePointer[0] = list->entities[last].scale[0];
    alphaPointer[0] = list->entities[last].alpha[0];

    list->entities[i] = list->entities[last];

    list->entities[i].position = positionPointer;
    list->entities[i].currentSpritePanel = currentSpritePanelPointer;
    list->entities[i].whiteOut = whiteOutPointer;
    list->entities[i].scale = scalePointer;
    list->entities[i].alpha = alphaPointer;

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

extern void entities_fromText(EntitiesList* list, Sprites_Sprite* sprite, const char* text, EntitiesFromTextOptions* opts) {
    uint8_t i = 0;

    if (opts->reset) {
        list->count = 0;
    }

    while (text[i] && list->count < RENDERER_DRAWLIST_MAX) {
        int8_t animationIndex = sprites_charToAnimationIndex(text[i]);

        if (animationIndex < 0) {
            ++i;
            continue;
        }

        entities_spawn(list, sprite, &(EntitiesInitOptions) { 
            .x = opts->x + i * sprite->panelDims[0] * opts->scale * SPRITES_TEXT_SPACING_SCALE,
            .y = opts->y,
            .scale = opts->scale,
            .transparency = opts->transparency,
            .currentAnimation = animationIndex
        });

        ++i;
    }

}