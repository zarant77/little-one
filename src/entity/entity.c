#include "entity.h"

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
}

void entity_update(Entity* entity, float world_speed, float dt) {
    float speed;

    if (entity == 0 || !entity->active) {
        return;
    }

    speed = world_speed;
    if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
        speed += entity->enemyConfig->moveSpeed;
    }

    entity->x -= speed * dt;
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
