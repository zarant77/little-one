#include "sprite_registry.h"

const SpriteDefinition* SPRITE_DEFINITIONS[] = {
        &PLAYER_SPRITE,
        &BOAR_SPRITE,
        &ORK_SPRITE,
        &RAT_SPRITE,
        &ROCK_SPRITE,
        &STUMP_SPRITE,
        &BG_MOUNTAINS_SPRITE,
        &BG_FOREST_SPRITE,
        &FG_BUSHES_SPRITE,
        &GROUND_GRASS_SPRITE,
};

const size_t SPRITE_COUNT =
        sizeof(SPRITE_DEFINITIONS) / sizeof(SPRITE_DEFINITIONS[0]);
