#include "entity.h"

#include <stdint.h>

#include "../sprites/animations/animation_evaluate.h"

static int entity_smooth_wave_permille(int age_ms, int period_ms) {
    int half_period;
    int phase;
    int t;
    int wave;

    if (period_ms <= 1) return 0;
    half_period = period_ms / 2;
    if (half_period <= 0) return 0;
    phase = age_ms % period_ms;

    if (phase < half_period) {
        t = phase * 1000 / half_period;
        wave = 4 * t * (1000 - t) / 1000;
        return wave;
    }

    t = (phase - half_period) * 1000 / (period_ms - half_period);
    wave = 4 * t * (1000 - t) / 1000;
    return -wave;
}

static int entity_triangle_wave_permille(int age_ms, int period_ms) {
    int phase;
    int t;

    if (period_ms <= 1) return 0;
    phase = (age_ms + period_ms / 4) % period_ms;
    t = phase * 2000 / period_ms;
    return t < 1000 ? -1000 + t * 2 : 3000 - t * 2;
}

static int entity_jagged_jitter(int age_ms, int period_ms, int amplitude) {
    uint32_t value;
    int range;

    if (period_ms <= 0 || amplitude <= 0 || age_ms < period_ms) return 0;
    value = (uint32_t)(age_ms / period_ms) * 0x9e3779b9u;
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    range = amplitude * 2 + 1;
    return (int)(value % (uint32_t)range) - amplitude;
}

static float entity_trajectory_offset_y(const Entity* entity) {
    const ThreatMovementConfig* movement;
    int offset = 0;

    if (entity == 0 || entity->config == 0) return 0.0f;
    movement = &entity->config->movement;

    if (movement->trajectory == THREAT_TRAJECTORY_SMOOTH_CURVE) {
        offset = movement->amplitude
                * entity_smooth_wave_permille(entity->age_ms, movement->period_ms)
                / 1000;
    } else if (movement->trajectory == THREAT_TRAJECTORY_JAGGED_ZIGZAG) {
        offset = movement->amplitude
                * entity_triangle_wave_permille(entity->age_ms, movement->period_ms)
                / 1000;
        offset += entity_jagged_jitter(
                entity->age_ms,
                movement->jitter_period_ms,
                movement->jitter_amplitude
        );
    }

    return (float)offset;
}

static const EntityAnimationConfig* entity_animation_config(const Entity* entity) {
    if (entity == 0 || entity->type != ENTITY_THREAT || entity->config == 0) {
        return entity_animation_default_enemy_config();
    }
    if (entity->config->visual.sprite_id == SPRITE_BOAR) return entity_animation_boar_config();
    if (entity->config->visual.sprite_id == SPRITE_ORK) return entity_animation_ork_config();
    if (entity->config->type == THREAT_FLYING_ENEMY) return entity_animation_bird_config();
    if (entity->config->type == THREAT_STATIC_OBSTACLE) return entity_animation_default_obstacle_config();
    return entity_animation_default_enemy_config();
}

static void entity_set_animation(Entity* entity, EntityAnimSlot slot, const char* clip_id) {
    const AnimationClip* clip;
    entity_animation_set(&entity->animation, entity_animation_config(entity), slot);
    if (clip_id == 0) return;
    clip = animation_find_clip(clip_id);
    if (clip != 0) entity->animation.clip = clip;
}

static void entity_set_movement_animation(Entity* entity) {
    EntityAnimSlot slot = entity->config->type == THREAT_STATIC_OBSTACLE
            ? ENTITY_ANIM_IDLE : ENTITY_ANIM_WALK;
    entity_set_animation(entity, slot, entity->config->visual.animationId);
}

void entity_clear(Entity* entity) {
    if (entity == 0) return;
    entity->type = ENTITY_NONE;
    entity->x = entity->y = 0.0f;
    entity->active = entity->dead = 0;
    entity->config = 0;
    entity->powerup_config = 0;
    entity->velocity_x = 0.0f;
    entity->velocity_y = 0.0f;
    entity->movement_origin_y = 0.0f;
    entity->grounded = 0;
    entity->age_ms = 0;
    entity->ground_age_ms = 0;
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
}

void entity_spawn(Entity* entity, const ThreatConfig* config, float x, float y) {
    if (entity == 0 || config == 0) return;
    entity->type = ENTITY_THREAT;
    entity->x = x;
    entity->y = y;
    entity->active = 1;
    entity->dead = 0;
    entity->config = config;
    entity->powerup_config = 0;
    entity->velocity_x = 0.0f;
    entity->velocity_y = 0.0f;
    entity->movement_origin_y = y;
    entity->grounded = 0;
    entity->age_ms = 0;
    entity->ground_age_ms = 0;
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
    entity_set_movement_animation(entity);
}

void entity_spawn_powerup(
        Entity* entity,
        const PowerupConfig* config,
        float x,
        float y,
        float velocity_x,
        float velocity_y
) {
    if (entity == 0 || config == 0) return;

    entity_clear(entity);
    entity->type = ENTITY_POWERUP;
    entity->x = x;
    entity->y = y;
    entity->active = 1;
    entity->powerup_config = config;
    entity->velocity_x = velocity_x;
    entity->velocity_y = velocity_y;
    entity->movement_origin_y = y;
}

void entity_attack(Entity* entity) {
    if (entity == 0 || entity->type != ENTITY_THREAT
            || !entity->active || entity->dead || entity->config == 0) return;
    if (entity->animation.slot == ENTITY_ANIM_ATTACK) return;
    entity_set_animation(entity, ENTITY_ANIM_ATTACK, entity->config->visual.attackAnimationId);
}

void entity_kill(Entity* entity) {
    if (entity == 0 || entity->type != ENTITY_THREAT
            || !entity->active || entity->dead || entity->config == 0) return;
    entity->dead = 1;
    entity_set_animation(entity, ENTITY_ANIM_DEATH, entity->config->visual.deathAnimationId);
}

void entity_update(Entity* entity, float world_speed, float dt) {
    int32_t elapsed_ms;
    int attacking;
    if (entity == 0 || entity->type != ENTITY_THREAT
            || !entity->active || entity->config == 0) return;
    elapsed_ms = (int32_t)(dt * 1000.0f);
    if (entity->dead) {
        entity_animation_update(&entity->animation, elapsed_ms);
        if (entity->animation.clip == 0 || (!entity->animation.clip->loop
                && entity->animation.time_ms >= entity->animation.clip->duration_ms)) entity_clear(entity);
        return;
    }

    attacking = entity->animation.slot == ENTITY_ANIM_ATTACK;
    if (attacking && entity->animation.clip != 0 && !entity->animation.clip->loop
            && entity->animation.time_ms >= entity->animation.clip->duration_ms) {
        entity_set_movement_animation(entity);
        attacking = 0;
    }
    if (!attacking) entity_set_movement_animation(entity);
    entity->age_ms += elapsed_ms;
    entity->x -= (world_speed + entity->config->moveSpeed) * dt;
    entity->y = entity->movement_origin_y + entity_trajectory_offset_y(entity);
    entity_animation_update(&entity->animation, elapsed_ms);
}

void entity_update_powerup(Entity* entity, float world_speed, float ground_y, float dt) {
    const PowerupTuning* tuning;
    int elapsed_ms;

    if (entity == 0 || entity->type != ENTITY_POWERUP
            || !entity->active || entity->powerup_config == 0) return;

    tuning = powerup_tuning_get();
    elapsed_ms = (int)(dt * 1000.0f);
    if (elapsed_ms < 0) elapsed_ms = 0;
    entity->age_ms += elapsed_ms;

    if (entity->grounded) {
        entity->ground_age_ms += elapsed_ms;
        entity->x -= world_speed * dt;
        entity->y = ground_y - (float)entity->powerup_config->height;
        if (tuning->ground_lifetime_ms > 0
                && entity->ground_age_ms >= tuning->ground_lifetime_ms) {
            entity_clear(entity);
        }
        return;
    }

    entity->x += (entity->velocity_x - world_speed) * dt;
    entity->velocity_y += tuning->fall_gravity * dt;
    entity->y += entity->velocity_y * dt;
    if (entity->y + (float)entity->powerup_config->height >= ground_y) {
        entity->y = ground_y - (float)entity->powerup_config->height;
        entity->velocity_x = 0.0f;
        entity->velocity_y = 0.0f;
        entity->grounded = 1;
        entity->ground_age_ms = 0;
    }
}

int entity_get_width(const Entity* entity) {
    if (entity == 0 || !entity->active) return 0;
    if (entity->type == ENTITY_THREAT && entity->config != 0) {
        return entity->config->visual.width;
    }
    if (entity->type == ENTITY_POWERUP && entity->powerup_config != 0) {
        return entity->powerup_config->width;
    }
    return 0;
}

int entity_get_height(const Entity* entity) {
    if (entity == 0 || !entity->active) return 0;
    if (entity->type == ENTITY_THREAT && entity->config != 0) {
        return entity->config->visual.height;
    }
    if (entity->type == ENTITY_POWERUP && entity->powerup_config != 0) {
        return entity->powerup_config->height;
    }
    return 0;
}

const HurtZone* entity_get_hurt_zone(const Entity* entity) {
    if (entity == 0 || !entity->active || entity->dead) return 0;
    if (entity->type == ENTITY_THREAT && entity->config != 0) {
        return &entity->config->hurt_zone;
    }
    if (entity->type == ENTITY_POWERUP && entity->powerup_config != 0) {
        return &entity->powerup_config->pickup_zone;
    }
    return 0;
}
