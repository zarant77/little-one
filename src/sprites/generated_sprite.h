#ifndef LITTLE_ONE_GENERATED_SPRITE_H
#define LITTLE_ONE_GENERATED_SPRITE_H

#include <stdint.h>

typedef enum
{
    SPRITE_NONE = -1,
    SPRITE_AGENT00CAT = 0,
    SPRITE_CARRAMBACAT,
    SPRITE_CATBUSTER,
    SPRITE_DARTHCAT,
    SPRITE_DOOMCAT,
    SPRITE_HARRYPURRTER,
    SPRITE_IMPAWSIBLECAT,
    SPRITE_INDICAT,
    SPRITE_JAWSCAT,
    SPRITE_PINKPAWTHER,
    SPRITE_ROBOCAT,
    SPRITE_ROCKPAW,
    SPRITE_SCARCAT,
    SPRITE_TERMICATOR,
    SPRITE_BOAR,
    SPRITE_ORK,
    SPRITE_RAT,
    SPRITE_BIRD,
    SPRITE_BAT,
    SPRITE_ROCK,
    SPRITE_CACTUS,
    SPRITE_STUMP,
    SPRITE_BG_MOUNTAINS,
    SPRITE_BG_FOREST,
    SPRITE_FG_BUSHES,
    SPRITE_GROUND_GRASS,
    SPRITE_ID_COUNT
} SpriteId;

typedef struct
{
    const char *id;
    int16_t width;
    int16_t height;
    int16_t pivot_x;
    int16_t pivot_y;
    uint32_t *pixels;
} GeneratedSprite;

void generated_sprite_initialize_all(void);
void generated_sprite_shutdown_all(void);

const GeneratedSprite *generated_sprite_get(SpriteId sprite_id);
const GeneratedSprite *generated_sprite_get_by_id(const char *sprite_id);

#endif
