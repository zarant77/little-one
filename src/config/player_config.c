#include "player_config.h"

static const PlayerConfig PLAYER_CONFIG = {
    .hp = 3,                  // Hit points
    .moveSpeed = 600.0f,      // Horizontal movement speed
    .jumpVelocity = -1800.0f, // Initial jump velocity
    .smashVelocity = 4000.0f, // Downward smash velocity
    .hurt_zone = {
        .x = 0,
        .y = 0,
        .radius = 50,
    },
    .boundary = {
        .x = 0,
        .y = 0,
        .width = 250,
        .height = 160,
    },
    .visual = {
        .width = 250,          // Render width
        .height = 160,         // Render height
        .color = 0xffffffff,   // Fallback rectangle color
        .sprite_id = SPRITE_PLAYER, // Generated sprite identifier
        .animationId = "idle", // Future default animation
    },
};

const PlayerConfig *player_config_get(void)
{
    return &PLAYER_CONFIG;
}
