#ifndef LITTLE_ONE_GENERATED_SPRITE_H
#define LITTLE_ONE_GENERATED_SPRITE_H

#include <stdint.h>

#define DRAW_COMMAND_SIZE 7

typedef enum {
    DRAW_RECT = 0,
    DRAW_CIRCLE = 1,
    DRAW_TRIANGLE = 2
} DrawKind;

typedef enum {
    SPRITE_NONE = -1,
    SPRITE_PLAYER = 0,
    SPRITE_BOAR,
    SPRITE_ORK,
    SPRITE_RAT,
    SPRITE_ROCK,
    SPRITE_STUMP,
    SPRITE_COUNT
} SpriteId;

typedef struct {
    const uint32_t* data;
    int command_count;
    int width;
    int height;
} GeneratedSprite;

extern const GeneratedSprite PLAYER_SPRITE;

const GeneratedSprite* generated_sprite_get(SpriteId sprite_id);

#endif
