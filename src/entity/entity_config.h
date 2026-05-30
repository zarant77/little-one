#ifndef LITTLE_ONE_ENTITY_CONFIG_H
#define LITTLE_ONE_ENTITY_CONFIG_H

#include <stdint.h>

#include "../sprites/generated_sprite.h"

typedef struct {
    int width;
    int height;
    uint32_t color;
    SpriteId sprite_id;
    const char* animationId;
} EntityVisualConfig;

#endif
