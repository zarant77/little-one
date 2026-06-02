#pragma once

#include <stdint.h>

#include "../sprites/animations/animation_definition.h"

typedef enum {
    ENTITY_ANIM_IDLE = 0,
    ENTITY_ANIM_WALK,
    ENTITY_ANIM_JUMP,
    ENTITY_ANIM_FALL,
    ENTITY_ANIM_SMASH,
    ENTITY_ANIM_LAND,
    ENTITY_ANIM_DAMAGE,
    ENTITY_ANIM_DEATH,
    ENTITY_ANIM_COUNT
} EntityAnimSlot;

typedef struct {
    const char *clips[ENTITY_ANIM_COUNT];
} EntityAnimationConfig;

typedef struct {
    EntityAnimSlot slot;
    const AnimationClip *clip;
    int32_t time_ms;
} EntityAnimationState;

const EntityAnimationConfig *entity_animation_player_config(void);
const EntityAnimationConfig *entity_animation_boar_config(void);
const EntityAnimationConfig *entity_animation_ork_config(void);
const EntityAnimationConfig *entity_animation_default_enemy_config(void);
const EntityAnimationConfig *entity_animation_default_obstacle_config(void);

const AnimationClip *entity_animation_resolve(
        const EntityAnimationConfig *config,
        EntityAnimSlot slot
);

void entity_animation_set(
        EntityAnimationState *state,
        const EntityAnimationConfig *config,
        EntityAnimSlot slot
);

void entity_animation_update(
        EntityAnimationState *state,
        int32_t dt_ms
);

AnimationImpact entity_animation_get_impact(
        const EntityAnimationState *state
);
