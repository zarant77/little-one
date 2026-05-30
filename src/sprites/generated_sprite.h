#ifndef LITTLE_ONE_GENERATED_SPRITE_H
#define LITTLE_ONE_GENERATED_SPRITE_H

#include <stdint.h>

typedef enum {
    SPRITE_NONE = -1,
    SPRITE_PLAYER = 0,
    SPRITE_BOAR,
    SPRITE_ORK,
    SPRITE_RAT,
    SPRITE_ROCK,
    SPRITE_STUMP,
    SPRITE_ID_COUNT
} SpriteId;

typedef struct {
    const char* id;
    int16_t width;
    int16_t height;
    int16_t pivot_x;
    int16_t pivot_y;
    uint32_t* pixels;
} GeneratedSprite;

void generated_sprite_initialize_all(void);

const GeneratedSprite* generated_sprite_get(SpriteId sprite_id);
const GeneratedSprite* generated_sprite_get_by_id(const char* sprite_id);

#endif
