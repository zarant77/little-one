#include "obstacle_config.h"

static const ObstacleConfig OBSTACLE_CONFIGS[] = {
    // Stump
    {
        .id = "obstacle.stump", // Stable obstacle identifier
        .hurt_zone = {
            .x = 0,
            .y = 0,
            .radius = 100,
        },
        .visual = {
            .width = 250,                   // Render width
            .height = 250,                  // Render height
            .color = 0xff0000ff,            // Fallback rectangle color
            .sprite_id = SPRITE_STUMP,      // Generated sprite identifier
            .animationId = "predator_idle", // Future default animation
        },
    },
    // Rock
    {
        .id = "obstacle.rock", // Stable obstacle identifier
        .hurt_zone = {
            .x = 0,
            .y = 0,
            .radius = 100,
        },
        .visual = {
            .width = 250,                        // Render width
            .height = 250,                       // Render height
            .color = 0xe500d2ff,                 // Fallback rectangle color
            .sprite_id = SPRITE_ROCK,            // Generated sprite identifier
            .animationId = "predator_snap_idle", // Future default animation
        },
    },
    // Cactus
    {
        .id = "obstacle.cactus", // Stable obstacle identifier
        .hurt_zone = {
            .x = 0,
            .y = -40,
            .radius = 60,
        },
        .visual = {
            .width = 160,                   // Render width
            .height = 300,                  // Render height
            .color = 0xe500d2ff,            // Fallback rectangle color
            .sprite_id = SPRITE_CACTUS,     // Generated sprite identifier
            .animationId = "predator_sway", // Future default animation
        },
    },
};

static const int OBSTACLE_CONFIG_COUNT = sizeof(OBSTACLE_CONFIGS) / sizeof(OBSTACLE_CONFIGS[0]);

const ObstacleConfig *obstacle_config_get_all(int *count)
{
    if (count != 0)
    {
        *count = OBSTACLE_CONFIG_COUNT;
    }

    return OBSTACLE_CONFIGS;
}

const ObstacleConfig *obstacle_config_get(int index)
{
    if (index < 0 || index >= OBSTACLE_CONFIG_COUNT)
    {
        return 0;
    }

    return OBSTACLE_CONFIGS + index;
}
