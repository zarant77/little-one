#ifndef LITTLE_ONE_ENTITY_H
#define LITTLE_ONE_ENTITY_H

#include "../config/threat_config.h"
#include "entity_animation.h"

typedef enum {
    ENTITY_NONE,
    ENTITY_THREAT
} EntityType;

typedef struct {
    EntityType type;

    float x;
    float y;

    int active;
    int dead;

    const ThreatConfig* config;
    EntityAnimationState animation;
} Entity;

void entity_clear(Entity* entity);
void entity_spawn(Entity* entity, const ThreatConfig* config, float x, float y);
void entity_attack(Entity* entity);
void entity_kill(Entity* entity);
void entity_update(Entity* entity, float world_speed, float dt);
int entity_get_width(const Entity* entity);
int entity_get_height(const Entity* entity);
const HurtZone* entity_get_hurt_zone(const Entity* entity);

#endif
