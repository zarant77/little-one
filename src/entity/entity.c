#include "entity.h"

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

    return entity_animation_default_enemy_config();
}

void entity_clear(Entity* entity) {
    if (entity == 0) {
        return;
    }

    entity->type = ENTITY_NONE;
    entity->x = 0.0f;
    entity->y = 0.0f;
    entity->active = 0;
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
    entity->enemyConfig = config;
    entity->obstacleConfig = 0;
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
    entity_animation_set(
            &entity->animation,
            entity_animation_config_for_enemy(config),
            ENTITY_ANIM_WALK
    );
}

void entity_spawn_obstacle(Entity* entity, const ObstacleConfig* config, float x, float y) {
    if (entity == 0 || config == 0) {
        return;
    }

    entity->type = ENTITY_OBSTACLE;
    entity->x = x;
    entity->y = y;
    entity->active = 1;
    entity->enemyConfig = 0;
    entity->obstacleConfig = config;
    entity->animation.slot = ENTITY_ANIM_IDLE;
    entity->animation.clip = 0;
    entity->animation.time_ms = 0;
    entity_animation_set(
            &entity->animation,
            entity_animation_default_obstacle_config(),
            ENTITY_ANIM_IDLE
    );
}

void entity_update(Entity* entity, float world_speed, float dt) {
    float speed;

    if (entity == 0 || !entity->active) {
        return;
    }

    speed = world_speed;
    if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
        speed += entity->enemyConfig->moveSpeed;
        entity_animation_set(
                &entity->animation,
                entity_animation_config_for_enemy(entity->enemyConfig),
                speed != 0.0f ? ENTITY_ANIM_WALK : ENTITY_ANIM_IDLE
        );
    } else if (entity->type == ENTITY_OBSTACLE && entity->obstacleConfig != 0) {
        entity_animation_set(
                &entity->animation,
                entity_animation_default_obstacle_config(),
                ENTITY_ANIM_IDLE
        );
    }

    entity->x -= speed * dt;
    entity_animation_update(&entity->animation, (int32_t)(dt * 1000.0f));
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
    if (entity == 0 || !entity->active) {
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
