#include "player_config.h"

static const PlayerConfig PLAYER_CONFIG = {
    .hp = 1,                  // Hit points
    .moveSpeed = 500.0f,      // Horizontal movement speed
    .jumpVelocity = -1600.0f, // Initial jump velocity
    .smashVelocity = 3200.0f, // Downward smash velocity
    .visual = {
        .width = 128,          // Render and collision width
        .height = 128,         // Render and collision height
        .color = 0xffffffff,   // Fallback rectangle color
        .spriteId = "player",  // Future sprite identifier
        .animationId = "idle", // Future default animation
    },
};

const PlayerConfig *player_config_get(void)
{
    return &PLAYER_CONFIG;
}
