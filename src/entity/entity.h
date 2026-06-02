#ifndef LITTLE_ONE_ENTITY_H
#define LITTLE_ONE_ENTITY_H

#include "../config/enemy_config.h"
#include "../config/obstacle_config.h"
#include "entity_animation.h"

typedef enum {
    ENTITY_NONE,
    ENTITY_ENEMY,
    ENTITY_OBSTACLE
} EntityType;

typedef struct {
    EntityType type;

    float x;
    float y;

    int active;

    const EnemyConfig* enemyConfig;
    const ObstacleConfig* obstacleConfig;
    EntityAnimationState animation;
} Entity;

void entity_clear(Entity* entity);
void entity_spawn_enemy(Entity* entity, const EnemyConfig* config, float x, float y);
void entity_spawn_obstacle(Entity* entity, const ObstacleConfig* config, float x, float y);
void entity_update(Entity* entity, float world_speed, float dt);
int entity_get_width(const Entity* entity);
int entity_get_height(const Entity* entity);
const HurtZone* entity_get_hurt_zone(const Entity* entity);

#endif
