#include "entity_config.h"

static const PlayerConfig PLAYER_CONFIG = {
        1,
        220.0f,
        -620.0f,
        1200.0f,
        {
                64,
                64,
                "player",
                "idle",
        },
};

static const EnemyConfig ENEMY_CONFIGS[] = {
        {
                "enemy.basic",
                1,
                120.0f,
                1,
                0.0f,
                0.0f,
                {
                        48,
                        48,
                        "enemy.basic",
                        "walk",
                },
        },
};

static const ObstacleConfig OBSTACLE_CONFIGS[] = {
        {
                "obstacle.block",
                {
                        48,
                        64,
                        "obstacle.block",
                        "idle",
                },
        },
};

static const int ENEMY_CONFIG_COUNT = sizeof(ENEMY_CONFIGS) / sizeof(ENEMY_CONFIGS[0]);
static const int OBSTACLE_CONFIG_COUNT = sizeof(OBSTACLE_CONFIGS) / sizeof(OBSTACLE_CONFIGS[0]);

const PlayerConfig* entity_config_get_player(void) {
    return &PLAYER_CONFIG;
}

const EnemyConfig* entity_config_get_enemies(int* count) {
    if (count != 0) {
        *count = ENEMY_CONFIG_COUNT;
    }

    return ENEMY_CONFIGS;
}

const EnemyConfig* entity_config_get_enemy(int index) {
    if (index < 0 || index >= ENEMY_CONFIG_COUNT) {
        return 0;
    }

    return ENEMY_CONFIGS + index;
}

const ObstacleConfig* entity_config_get_obstacles(int* count) {
    if (count != 0) {
        *count = OBSTACLE_CONFIG_COUNT;
    }

    return OBSTACLE_CONFIGS;
}

const ObstacleConfig* entity_config_get_obstacle(int index) {
    if (index < 0 || index >= OBSTACLE_CONFIG_COUNT) {
        return 0;
    }

    return OBSTACLE_CONFIGS + index;
}
