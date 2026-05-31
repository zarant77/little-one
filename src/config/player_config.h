#ifndef LITTLE_ONE_PLAYER_CONFIG_H
#define LITTLE_ONE_PLAYER_CONFIG_H

#include "../entity/entity_config.h"

typedef struct {
    int hp;
    float moveSpeed;
    float jumpVelocity;
    float smashVelocity;
    HurtZone hurt_zone;
    EntityVisualConfig visual;
} PlayerConfig;

const PlayerConfig* player_config_get(void);

#endif
