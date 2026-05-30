#ifndef LITTLE_ONE_SPRITE_DEFINITION_H
#define LITTLE_ONE_SPRITE_DEFINITION_H

#include <stdint.h>

#include "sprite_command.h"

typedef struct {
    const char* id;

    int16_t width;
    int16_t height;

    int16_t pivot_x;
    int16_t pivot_y;

    const SpriteCommand* commands;
    int16_t command_count;
} SpriteDefinition;

#endif
