#include "entity.h"

#include "../sprites/animations/animation_evaluate.h"

static const EntityAnimationConfig* entity_animation_config(const Entity* entity) {
    if (entity == 0 || entity->config == 0) return entity_animation_default_enemy_config();
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
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
    entity_set_movement_animation(entity);
}

void entity_attack(Entity* entity) {
    if (entity == 0 || !entity->active || entity->dead || entity->config == 0) return;
    if (entity->animation.slot == ENTITY_ANIM_ATTACK) return;
    entity_set_animation(entity, ENTITY_ANIM_ATTACK, entity->config->visual.attackAnimationId);
}

void entity_kill(Entity* entity) {
    if (entity == 0 || !entity->active || entity->dead) return;
    entity->dead = 1;
    entity_set_animation(entity, ENTITY_ANIM_DEATH, entity->config->visual.deathAnimationId);
}

void entity_update(Entity* entity, float world_speed, float dt) {
    int32_t elapsed_ms;
    int attacking;
    if (entity == 0 || !entity->active || entity->config == 0) return;
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
    entity->x -= (world_speed + entity->config->moveSpeed) * dt;
    entity_animation_update(&entity->animation, elapsed_ms);
}

int entity_get_width(const Entity* entity) {
    return entity != 0 && entity->active && entity->config != 0 ? entity->config->visual.width : 0;
}

int entity_get_height(const Entity* entity) {
    return entity != 0 && entity->active && entity->config != 0 ? entity->config->visual.height : 0;
}

const HurtZone* entity_get_hurt_zone(const Entity* entity) {
    return entity != 0 && entity->active && !entity->dead && entity->config != 0
            ? &entity->config->hurt_zone : 0;
}
