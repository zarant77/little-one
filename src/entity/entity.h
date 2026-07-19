#ifndef LITTLE_ONE_ENTITY_H
#define LITTLE_ONE_ENTITY_H

#include "../config/powerup_config.h"
#include "../config/threat_config.h"
#include "entity_animation.h"

typedef enum {
    ENTITY_NONE,
    ENTITY_THREAT,
    ENTITY_POWERUP
} EntityType;

typedef struct {
    EntityType type;

    float x;
    float y;

    int active;
    int dead;

    const ThreatConfig* config;
    const PowerupConfig* powerup_config;
    float velocity_x;
    float velocity_y;
    float movement_origin_y;
    int grounded;
    int age_ms;
    int ground_age_ms;
    EntityAnimationState animation;
} Entity;

void entity_clear(Entity* entity);
void entity_spawn(Entity* entity, const ThreatConfig* config, float x, float y);
void entity_spawn_powerup(
        Entity* entity,
        const PowerupConfig* config,
        float x,
        float y,
        float velocity_x,
        float velocity_y
);
void entity_attack(Entity* entity);
void entity_kill(Entity* entity);
void entity_update(Entity* entity, float world_speed, float dt);
void entity_update_powerup(Entity* entity, float world_speed, float ground_y, float dt);
int entity_get_width(const Entity* entity);
int entity_get_height(const Entity* entity);
const HurtZone* entity_get_hurt_zone(const Entity* entity);

#endif
