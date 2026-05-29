#ifndef LITTLE_ONE_OBSTACLE_CONFIG_H
#define LITTLE_ONE_OBSTACLE_CONFIG_H

#include "entity_config.h"

typedef struct {
    const char* id;
    EntityVisualConfig visual;
} ObstacleConfig;

const ObstacleConfig* obstacle_config_get_all(int* count);
const ObstacleConfig* obstacle_config_get(int index);

#endif
