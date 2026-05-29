#ifndef LITTLE_ONE_ENTITY_CONFIG_H
#define LITTLE_ONE_ENTITY_CONFIG_H

typedef struct {
    int width;
    int height;
    const char* spriteId;
    const char* animationId;
} EntityVisualConfig;

typedef struct {
    int hp;
    float moveSpeed;
    float jumpVelocity;
    float smashVelocity;
    EntityVisualConfig visual;
} PlayerConfig;

typedef struct {
    const char* id;
    int hp;
    float moveSpeed;
    int scoreValue;
    float yMin;
    float yMax;
    EntityVisualConfig visual;
} EnemyConfig;

typedef struct {
    const char* id;
    EntityVisualConfig visual;
} ObstacleConfig;

const PlayerConfig* entity_config_get_player(void);

const EnemyConfig* entity_config_get_enemies(int* count);
const EnemyConfig* entity_config_get_enemy(int index);

const ObstacleConfig* entity_config_get_obstacles(int* count);
const ObstacleConfig* entity_config_get_obstacle(int index);

#endif
