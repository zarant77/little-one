#include "entity.h"

#include "../sprites/animations/animation_evaluate.h"

static const EntityAnimationConfig* entity_animation_config_for_enemy(const EnemyConfig* config) {
    if (config == 0) {
        return entity_animation_default_enemy_config();
    }

    if (config->visual.sprite_id == SPRITE_BOAR) {
        return entity_animation_boar_config();
    }

    if (config->visual.sprite_id == SPRITE_ORK) {
        return entity_animation_ork_config();
    }

    if (config->visual.sprite_id == SPRITE_BIRD || config->visual.sprite_id == SPRITE_BAT) {
        return entity_animation_bird_config();
    }

    return entity_animation_default_enemy_config();
}

static void entity_animation_set_enemy_movement(Entity* entity, EntityAnimSlot slot) {
    const AnimationClip* clip;

    entity_animation_set(
            &entity->animation,
            entity_animation_config_for_enemy(entity->enemyConfig),
            slot
    );

    if (entity->enemyConfig->visual.sprite_id != SPRITE_BAT ||
        entity->enemyConfig->visual.animationId == 0) {
        return;
    }

    clip = animation_find_clip(entity->enemyConfig->visual.animationId);
    if (clip != 0) {
        entity->animation.clip = clip;
    }
}

static void entity_animation_set_obstacle_idle(Entity* entity) {
    const AnimationClip* clip;

    entity_animation_set(
            &entity->animation,
            entity_animation_default_obstacle_config(),
            ENTITY_ANIM_IDLE
    );

    if (entity->obstacleConfig->visual.animationId == 0) {
        return;
    }

    clip = animation_find_clip(entity->obstacleConfig->visual.animationId);
    if (clip != 0) {
        entity->animation.clip = clip;
    }
}

static void entity_animation_set_death(Entity* entity) {
    const char* death_animation_id = 0;
    const AnimationClip* clip;

    if (entity->enemyConfig != 0) {
        death_animation_id = entity->enemyConfig->visual.deathAnimationId;
    } else if (entity->obstacleConfig != 0) {
        death_animation_id = entity->obstacleConfig->visual.deathAnimationId;
    }

    entity_animation_set(
            &entity->animation,
            entity->enemyConfig != 0
                    ? entity_animation_config_for_enemy(entity->enemyConfig)
                    : entity_animation_default_obstacle_config(),
            ENTITY_ANIM_DEATH
    );

    if (death_animation_id == 0) {
        return;
    }

    clip = animation_find_clip(death_animation_id);
    if (clip != 0) {
        entity->animation.clip = clip;
    }
}

void entity_clear(Entity* entity) {
    if (entity == 0) {
        return;
    }

    entity->type = ENTITY_NONE;
    entity->x = 0.0f;
    entity->y = 0.0f;
    entity->active = 0;
    entity->dead = 0;
    entity->enemyConfig = 0;
    entity->obstacleConfig = 0;
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
}

void entity_spawn_enemy(Entity* entity, const EnemyConfig* config, float x, float y) {
    if (entity == 0 || config == 0) {
        return;
    }

    entity->type = ENTITY_ENEMY;
    entity->x = x;
    entity->y = y;
    entity->active = 1;
    entity->dead = 0;
    entity->enemyConfig = config;
    entity->obstacleConfig = 0;
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
    entity_animation_set_enemy_movement(entity, ENTITY_ANIM_WALK);
}

void entity_spawn_obstacle(Entity* entity, const ObstacleConfig* config, float x, float y) {
    if (entity == 0 || config == 0) {
        return;
    }

    entity->type = ENTITY_OBSTACLE;
    entity->x = x;
    entity->y = y;
    entity->active = 1;
    entity->dead = 0;
    entity->enemyConfig = 0;
    entity->obstacleConfig = config;
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
    entity_animation_set_obstacle_idle(entity);
}

void entity_kill(Entity* entity) {
    if (entity == 0 || !entity->active || entity->dead) {
        return;
    }

    entity->dead = 1;
    entity_animation_set_death(entity);
}

void entity_update(Entity* entity, float world_speed, float dt) {
    float speed;
    int32_t elapsed_ms;

    if (entity == 0 || !entity->active) {
        return;
    }

    elapsed_ms = (int32_t)(dt * 1000.0f);
    if (entity->dead) {
        entity_animation_update(&entity->animation, elapsed_ms);
        if (entity->animation.clip == 0
                || (!entity->animation.clip->loop
                    && entity->animation.time_ms >= entity->animation.clip->duration_ms)) {
            entity_clear(entity);
        }
        return;
    }

    speed = world_speed;
    if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
        speed += entity->enemyConfig->moveSpeed;
        entity_animation_set_enemy_movement(
                entity,
                speed != 0.0f ? ENTITY_ANIM_WALK : ENTITY_ANIM_IDLE
        );
    } else if (entity->type == ENTITY_OBSTACLE && entity->obstacleConfig != 0) {
        entity_animation_set_obstacle_idle(entity);
    }

    entity->x -= speed * dt;
    entity_animation_update(&entity->animation, elapsed_ms);
}

int entity_get_width(const Entity* entity) {
    if (entity == 0 || !entity->active) {
        return 0;
    }

    if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
        return entity->enemyConfig->visual.width;
    }

    if (entity->type == ENTITY_OBSTACLE && entity->obstacleConfig != 0) {
        return entity->obstacleConfig->visual.width;
    }

    return 0;
}

int entity_get_height(const Entity* entity) {
    if (entity == 0 || !entity->active) {
        return 0;
    }

    if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
        return entity->enemyConfig->visual.height;
    }

    if (entity->type == ENTITY_OBSTACLE && entity->obstacleConfig != 0) {
        return entity->obstacleConfig->visual.height;
    }

    return 0;
}

const HurtZone* entity_get_hurt_zone(const Entity* entity) {
    if (entity == 0 || !entity->active || entity->dead) {
        return 0;
    }

    if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
        return &entity->enemyConfig->hurt_zone;
    }

    if (entity->type == ENTITY_OBSTACLE && entity->obstacleConfig != 0) {
        return &entity->obstacleConfig->hurt_zone;
    }

    return 0;
}
