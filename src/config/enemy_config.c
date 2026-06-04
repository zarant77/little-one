#include "enemy_config.h"

static const EnemyConfig ENEMY_CONFIGS[] = {
    // Boar
    {
        .id = "enemy.boar",  // Stable enemy identifier
        .hp = 1,             // Hit points
        .scoreValue = 1,     // Score awarded on kill
        .moveSpeed = 600.0f, // Horizontal movement speed
        .yMin = 0.0f,        // Minimum spawn Y offset from ground
        .yMax = 0.0f,        // Maximum spawn Y offset from ground
        .hurt_zone = {
            .x = 0,
            .y = 16,
            .radius = 54,
        },
        .visual = {
            .width = 200,             // Render width
            .height = 200,            // Render height
            .color = 0xc06e00ff,      // Fallback rectangle color
            .sprite_id = SPRITE_BOAR, // Generated sprite identifier
            .animationId = "walk",    // Future default animation
        },
    },
    // Ork
    {
        .id = "enemy.ork",   // Stable enemy identifier
        .hp = 1,             // Hit points
        .scoreValue = 1,     // Score awarded on kill
        .moveSpeed = 300.0f, // Horizontal movement speed
        .yMin = 0.0f,        // Minimum spawn Y offset from ground
        .yMax = 0.0f,        // Maximum spawn Y offset from ground
        .hurt_zone = {
            .x = 0,
            .y = 4,
            .radius = 68,
        },
        .visual = {
            .width = 250,            // Render width
            .height = 250,           // Render height
            .color = 0x00aa00ff,     // Fallback rectangle color
            .sprite_id = SPRITE_ORK, // Generated sprite identifier
            .animationId = "walk",   // Future default animation
        },
    },
    // Rat
    {
        .id = "enemy.rat",   // Stable enemy identifier
        .hp = 1,             // Hit points
        .scoreValue = 1,     // Score awarded on kill
        .moveSpeed = 900.0f, // Horizontal movement speed
        .yMin = 0.0f,        // Minimum spawn Y offset from ground
        .yMax = 0.0f,        // Maximum spawn Y offset from ground
        .hurt_zone = {
            .x = 0,
            .y = 10,
            .radius = 26,
        },
        .visual = {
            .width = 200,            // Render width
            .height = 100,            // Render height
            .color = 0x666666ff,     // Fallback rectangle color
            .sprite_id = SPRITE_RAT, // Generated sprite identifier
            .animationId = "walk",   // Future default animation
        },
    },
};

static const int ENEMY_CONFIG_COUNT = sizeof(ENEMY_CONFIGS) / sizeof(ENEMY_CONFIGS[0]);

const EnemyConfig *enemy_config_get_all(int *count)
{
    if (count != 0)
    {
        *count = ENEMY_CONFIG_COUNT;
    }

    return ENEMY_CONFIGS;
}

const EnemyConfig *enemy_config_get(int index)
{
    if (index < 0 || index >= ENEMY_CONFIG_COUNT)
    {
        return 0;
    }

    return ENEMY_CONFIGS + index;
}
