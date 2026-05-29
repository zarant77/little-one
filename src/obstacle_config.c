#include "obstacle_config.h"

static const ObstacleConfig OBSTACLE_CONFIGS[] = {
    // Stump
    {
        .id = "obstacle.stump", // Stable obstacle identifier
        .visual = {
            .width = 80,                  // Render and collision width
            .height = 180,                // Render and collision height
            .color = 0xff0000ff,          // Fallback rectangle color
            .spriteId = "obstacle.stump", // Future sprite identifier
            .animationId = "idle",        // Future default animation
        },
    },
    // Rock
    {
        .id = "obstacle.rock", // Stable obstacle identifier
        .visual = {
            .width = 150,                // Render and collision width
            .height = 150,               // Render and collision height
            .color = 0xe500d2ff,         // Fallback rectangle color
            .spriteId = "obstacle.rock", // Future sprite identifier
            .animationId = "idle",       // Future default animation
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
