#ifndef LITTLE_ONE_ENEMY_CONFIG_H
#define LITTLE_ONE_ENEMY_CONFIG_H

#include "../entity/entity_config.h"

typedef struct {
    const char* id;
    int hp;
    int scoreValue;
    float moveSpeed;
    float yMin;
    float yMax;
    EntityVisualConfig visual;
} EnemyConfig;

const EnemyConfig* enemy_config_get_all(int* count);
const EnemyConfig* enemy_config_get(int index);

#endif
