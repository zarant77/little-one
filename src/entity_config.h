#ifndef LITTLE_ONE_ENTITY_CONFIG_H
#define LITTLE_ONE_ENTITY_CONFIG_H

#include <stdint.h>

typedef struct {
    int width;
    int height;
    uint32_t color;
    const char* spriteId;
    const char* animationId;
} EntityVisualConfig;

#endif
