#include "player_config.h"

static const PlayerConfig PLAYER_CONFIG = {
    .hp = 3,                  // Hit points
    .moveSpeed = 600.0f,      // Horizontal movement speed
    .jumpVelocity = -1800.0f, // Initial jump velocity
    .smashVelocity = 4000.0f, // Downward smash velocity
    .hurt_zone = {
        .x = 0,
        .y = 0,
        .radius = 70,
    },
    .attack_zone = {
        .x = 50,
        .y = 0,
        .width = 350,
        .height = 138,
    },
    .visual = {
        .width = 182,          // Render width
        .height = 138,         // Render height
        .color = 0xffffffff,   // Fallback rectangle color
        .sprite_id = SPRITE_PLAYER, // Generated character sprite identifier
        .animationId = "idle", // Future default animation
        .deathAnimationId = "player_death_fall",
    },
    .sword = {
        .idle = {
            .offset_x = 117,
            .offset_y = 117,
            .rotation_degrees = 60.0f,
        },
        .jump = {
            .offset_x = 117,
            .offset_y = 47,
            .rotation_degrees = 300.0f,
        },
        .attack = {
            .offset_x = 117,
            .offset_y = 117,
            .rotation_degrees = 90.0f,
        },
    },
    .eye_colors = {
        .source_color = 0x00ff00ff,
        .hp_3_color = 0xffffffff,
        .hp_2_color = 0xff8c00ff,
        .hp_1_color = 0xff0000ff,
    },
};

const PlayerConfig *player_config_get(void)
{
    return &PLAYER_CONFIG;
}
