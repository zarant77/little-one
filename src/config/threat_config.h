#ifndef LITTLE_ONE_THREAT_CONFIG_H
#define LITTLE_ONE_THREAT_CONFIG_H

#include "../entity/entity_config.h"

typedef enum {
    THREAT_STATIC_OBSTACLE = 0,
    THREAT_GROUND_ENEMY,
    THREAT_FLYING_ENEMY
} ThreatType;

typedef enum {
    THREAT_TRAJECTORY_STRAIGHT = 0,
    THREAT_TRAJECTORY_SMOOTH_CURVE,
    THREAT_TRAJECTORY_JAGGED_ZIGZAG
} ThreatTrajectory;

typedef struct {
    ThreatTrajectory trajectory;
    int amplitude;
    int period_ms;
    int jitter_amplitude;
    int jitter_period_ms;
} ThreatMovementConfig;

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
    ThreatMovementConfig movement;
} ThreatConfig;

const ThreatConfig* threat_config_get_all(int* count);
const ThreatConfig* threat_config_get(int index);

#endif
