#ifndef LITTLE_ONE_THREAT_CONFIG_H
#define LITTLE_ONE_THREAT_CONFIG_H

#include "../entity/entity_config.h"

typedef enum {
    THREAT_STATIC_OBSTACLE = 0,
    THREAT_GROUND_ENEMY,
    THREAT_FLYING_ENEMY
} ThreatType;

typedef struct {
    const char* id;
    ThreatType type;
    int hp;
    int scoreValue;
    float moveSpeed;
    float spawnYmin;
    float spawnYmax;
    HurtZone hurt_zone;
    EntityVisualConfig visual;
} ThreatConfig;

const ThreatConfig* threat_config_get_all(int* count);
const ThreatConfig* threat_config_get(int index);

#endif
