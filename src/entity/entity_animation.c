#include "entity_animation.h"

#include "../sprites/animations/animation_evaluate.h"

static const char *ENTITY_ANIM_GENERIC_CLIPS[ENTITY_ANIM_COUNT] = {
        [ENTITY_ANIM_IDLE] = "idle",
        [ENTITY_ANIM_WALK] = "walk",
        [ENTITY_ANIM_JUMP] = "jump",
        [ENTITY_ANIM_FALL] = "fall",
        [ENTITY_ANIM_SMASH] = "smash",
        [ENTITY_ANIM_LAND] = "land",
        [ENTITY_ANIM_DAMAGE] = "damage",
        [ENTITY_ANIM_DEATH] = "death",
};

static const EntityAnimationConfig PLAYER_ANIMATIONS = {
        .clips = {
                [ENTITY_ANIM_IDLE] = "idle",
                [ENTITY_ANIM_WALK] = "walk",
                [ENTITY_ANIM_JUMP] = "jump",
                [ENTITY_ANIM_FALL] = "fall",
                [ENTITY_ANIM_SMASH] = "smash",
                [ENTITY_ANIM_LAND] = "land",
                [ENTITY_ANIM_DAMAGE] = "damage",
                [ENTITY_ANIM_DEATH] = "death",
        },
};

static const EntityAnimationConfig BOAR_ANIMATIONS = {
        .clips = {
                [ENTITY_ANIM_IDLE] = "idle",
                [ENTITY_ANIM_WALK] = "boar_walk",
                [ENTITY_ANIM_DAMAGE] = "damage",
                [ENTITY_ANIM_DEATH] = "death",
        },
};

static const EntityAnimationConfig ORK_ANIMATIONS = {
        .clips = {
                [ENTITY_ANIM_IDLE] = "idle",
                [ENTITY_ANIM_WALK] = "ork_walk",
                [ENTITY_ANIM_DAMAGE] = "damage",
                [ENTITY_ANIM_DEATH] = "death",
        },
};

static const EntityAnimationConfig BIRD_ANIMATIONS = {
        .clips = {
                [ENTITY_ANIM_IDLE] = "fly",
                [ENTITY_ANIM_WALK] = "fly",
                [ENTITY_ANIM_DAMAGE] = "damage",
                [ENTITY_ANIM_DEATH] = "flying_death",
        },
};

static const EntityAnimationConfig DEFAULT_ENEMY_ANIMATIONS = {
        .clips = {
                [ENTITY_ANIM_IDLE] = "idle",
                [ENTITY_ANIM_WALK] = "walk",
                [ENTITY_ANIM_DAMAGE] = "damage",
                [ENTITY_ANIM_DEATH] = "death",
        },
};

static const EntityAnimationConfig DEFAULT_OBSTACLE_ANIMATIONS = {
        .clips = {
                [ENTITY_ANIM_IDLE] = "idle",
                [ENTITY_ANIM_DAMAGE] = "damage",
                [ENTITY_ANIM_DEATH] = "death",
        },
};

const EntityAnimationConfig *entity_animation_player_config(void) {
    return &PLAYER_ANIMATIONS;
}

const EntityAnimationConfig *entity_animation_boar_config(void) {
    return &BOAR_ANIMATIONS;
}

const EntityAnimationConfig *entity_animation_ork_config(void) {
    return &ORK_ANIMATIONS;
}

const EntityAnimationConfig *entity_animation_bird_config(void) {
    return &BIRD_ANIMATIONS;
}

const EntityAnimationConfig *entity_animation_default_enemy_config(void) {
    return &DEFAULT_ENEMY_ANIMATIONS;
}

const EntityAnimationConfig *entity_animation_default_obstacle_config(void) {
    return &DEFAULT_OBSTACLE_ANIMATIONS;
}

const AnimationClip *entity_animation_resolve(
        const EntityAnimationConfig *config,
        EntityAnimSlot slot
) {
    const AnimationClip *clip;

    if (slot < 0 || slot >= ENTITY_ANIM_COUNT) {
        return 0;
    }

    if (config != 0 && config->clips[slot] != 0) {
        clip = animation_find_clip(config->clips[slot]);
        if (clip != 0) {
            return clip;
        }
    }

    return animation_find_clip(ENTITY_ANIM_GENERIC_CLIPS[slot]);
}

void entity_animation_set(
        EntityAnimationState *state,
        const EntityAnimationConfig *config,
        EntityAnimSlot slot
) {
    if (state == 0) {
        return;
    }

    if (slot < 0 || slot >= ENTITY_ANIM_COUNT) {
        slot = ENTITY_ANIM_IDLE;
    }

    if (state->slot != slot) {
        state->time_ms = 0;
    }

    state->slot = slot;
    state->clip = entity_animation_resolve(config, slot);
}

void entity_animation_update(
        EntityAnimationState *state,
        int32_t dt_ms
) {
    if (state == 0) {
        return;
    }

    if (dt_ms < 0) {
        dt_ms = 0;
    }

    state->time_ms += dt_ms;
}

AnimationImpact entity_animation_get_impact(
        const EntityAnimationState *state
) {
    if (state == 0) {
        return animation_impact_default();
    }

    return animation_evaluate(state->clip, state->time_ms);
}
