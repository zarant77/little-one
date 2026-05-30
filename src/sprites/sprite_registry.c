#include "sprite_registry.h"

const SpriteDefinition* SPRITE_DEFINITIONS[] = {
        &PLAYER_SPRITE,
        &BOAR_SPRITE,
        &ORK_SPRITE,
        &RAT_SPRITE,
        &ROCK_SPRITE,
        &STUMP_SPRITE,
};

const size_t SPRITE_COUNT =
        sizeof(SPRITE_DEFINITIONS) / sizeof(SPRITE_DEFINITIONS[0]);
