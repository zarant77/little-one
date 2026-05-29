#include "game.h"

#include "entity_config.h"

static const float GRAVITY = 1400.0f;
static const float GROUND_MARGIN = 48.0f;
static const float WORLD_SPEED = 120.0f;
static const float WORLD_SCROLL_WRAP = 10000.0f;
static const float SPAWN_MIN_SECONDS = 1.5f;
static const float SPAWN_SECONDS_RANGE = 1.0f;
static const float SPAWN_RIGHT_PADDING = 16.0f;
static unsigned int game_random_state = 12345;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} GameRect;

static float game_player_width(void) {
    return (float)entity_config_get_player()->visual.width;
}

static float game_player_height(void) {
    return (float)entity_config_get_player()->visual.height;
}

static float game_ground_y(const GameState* game) {
    return (float)game->screenHeight - GROUND_MARGIN;
}

static float game_random_01(void) {
    game_random_state = game_random_state * 1103515245 + 12345;

    return (float)((game_random_state / 65536) % 32768) / 32767.0f;
}

static float game_next_spawn_time(void) {
    return SPAWN_MIN_SECONDS + game_random_01() * SPAWN_SECONDS_RANGE;
}

static void game_clamp_player_x(GameState* game) {
    float max_x = (float)game->screenWidth - game_player_width();

    if (max_x < 0.0f) {
        max_x = 0.0f;
    }

    if (game->playerX < 0.0f) {
        game->playerX = 0.0f;
    }

    if (game->playerX > max_x) {
        game->playerX = max_x;
    }
}

static void game_clamp_player_to_ground(GameState* game) {
    float ground_y = game_ground_y(game);
    float player_bottom = game->playerY + game_player_height();

    if (game->playerY < 0.0f) {
        game->playerY = 0.0f;
        if (game->playerVelocityY < 0.0f) {
            game->playerVelocityY = 0.0f;
        }
    }

    if (player_bottom >= ground_y) {
        game->playerY = ground_y - game_player_height();
        if (game->playerY < 0.0f) {
            game->playerY = 0.0f;
        }
        game->playerVelocityY = 0.0f;
        game->playerGrounded = 1;
        game->playerSmashing = 0;
        game->playerCanSmash = 0;
    }
}

static void game_clear_entities(GameState* game) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        entity_clear(game->entities + entity_index);
    }
}

static Entity* game_find_free_entity(GameState* game) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        if (!game->entities[entity_index].active) {
            return game->entities + entity_index;
        }
    }

    return 0;
}

static void game_spawn_entity(GameState* game) {
    Entity* entity = game_find_free_entity(game);
    float x = (float)game->screenWidth + SPAWN_RIGHT_PADDING;
    float y;

    if (entity == 0 || game->screenWidth <= 0 || game->screenHeight <= 0) {
        return;
    }

    if (game_random_01() < 0.5f) {
        const EnemyConfig* enemy_config = entity_config_get_enemy(0);
        if (enemy_config == 0) {
            return;
        }

        y = game_ground_y(game) - (float)enemy_config->visual.height;
        entity_spawn_enemy(entity, enemy_config, x, y);
    } else {
        const ObstacleConfig* obstacle_config = entity_config_get_obstacle(0);
        if (obstacle_config == 0) {
            return;
        }

        y = game_ground_y(game) - (float)obstacle_config->visual.height;
        entity_spawn_obstacle(entity, obstacle_config, x, y);
    }
}

static void game_update_spawn_timer(GameState* game, float dt) {
    game->spawnTimer -= dt;
    if (game->spawnTimer > 0.0f) {
        return;
    }

    game_spawn_entity(game);
    game->spawnTimer = game_next_spawn_time();
}

static void game_update_entities(GameState* game, float dt) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        Entity* entity = game->entities + entity_index;

        if (!entity->active) {
            continue;
        }

        entity_update(entity, game->worldSpeed, dt);
        if (entity->x + (float)entity_get_width(entity) < 0.0f) {
            entity_clear(entity);
        }
    }
}

static int game_rects_overlap(GameRect a, GameRect b) {
    return a.x < b.x + b.width
           && a.x + a.width > b.x
           && a.y < b.y + b.height
           && a.y + a.height > b.y;
}

static GameRect game_player_rect(const GameState* game) {
    GameRect rect;
    rect.x = game->playerX;
    rect.y = game->playerY;
    rect.width = game_player_width();
    rect.height = game_player_height();

    return rect;
}

static GameRect game_entity_rect(const Entity* entity) {
    GameRect rect;
    rect.x = entity->x;
    rect.y = entity->y;
    rect.width = (float)entity_get_width(entity);
    rect.height = (float)entity_get_height(entity);

    return rect;
}

static void game_handle_collisions(GameState* game, int player_was_smashing) {
    GameRect player_rect = game_player_rect(game);

    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        Entity* entity = game->entities + entity_index;

        if (!entity->active) {
            continue;
        }

        if (!game_rects_overlap(player_rect, game_entity_rect(entity))) {
            continue;
        }

        if (entity->type == ENTITY_ENEMY && player_was_smashing) {
            entity_clear(entity);
            continue;
        }

        game->gameOver = 1;
        return;
    }
}

void game_init(GameState* game) {
    if (game == 0) {
        return;
    }

    game->playerX = 0.0f;
    game->playerY = 0.0f;
    game->playerVelocityX = 0.0f;
    game->playerVelocityY = 0.0f;
    game->playerGrounded = 0;
    game->playerSmashing = 0;
    game->playerCanSmash = 0;
    game->screenWidth = 0;
    game->screenHeight = 0;
    game->worldScrollX = 0.0f;
    game->worldSpeed = WORLD_SPEED;
    game_clear_entities(game);
    game->spawnTimer = game_next_spawn_time();
    game->gameOver = 0;
}

void game_set_screen_size(GameState* game, float width, float height) {
    int old_screen_width;
    int old_screen_height;

    if (game == 0) {
        return;
    }

    old_screen_width = game->screenWidth;
    old_screen_height = game->screenHeight;

    game->screenWidth = (int)width;
    game->screenHeight = (int)height;

    if (old_screen_width <= 0 || old_screen_height <= 0) {
        game->playerX = 0.0f;
        game->playerY = game_ground_y(game) - game_player_height();
        game->playerVelocityX = 0.0f;
        game->playerVelocityY = 0.0f;
        game->playerGrounded = 1;
        game->playerSmashing = 0;
        game->playerCanSmash = 0;
    }

    game_clamp_player_x(game);
    game_clamp_player_to_ground(game);
}

void game_update(GameState* game, const InputState* input, float dt) {
    int player_was_smashing;

    if (game == 0) {
        return;
    }

    if (game->gameOver) {
        if (input != 0 && input->actionPressed) {
            int screen_width = game->screenWidth;
            int screen_height = game->screenHeight;

            game_init(game);
            game_set_screen_size(game, (float)screen_width, (float)screen_height);
        }

        return;
    }

    game->worldScrollX += game->worldSpeed * dt;
    while (game->worldScrollX >= WORLD_SCROLL_WRAP) {
        game->worldScrollX -= WORLD_SCROLL_WRAP;
    }

    /* Screen-space Y grows downward, matching the framebuffer. Negative Y velocity jumps upward. */
    game->playerVelocityX = 0.0f;

    if (input != 0 && input->left) {
        game->playerVelocityX = -entity_config_get_player()->moveSpeed;
    }

    if (input != 0 && input->right) {
        game->playerVelocityX = entity_config_get_player()->moveSpeed;
    }

    if (input != 0 && input->actionPressed) {
        if (game->playerGrounded) {
            game->playerVelocityY = entity_config_get_player()->jumpVelocity;
            game->playerGrounded = 0;
            game->playerSmashing = 0;
            game->playerCanSmash = 1;
        } else if (game->playerCanSmash) {
            game->playerVelocityY = entity_config_get_player()->smashVelocity;
            game->playerSmashing = 1;
            game->playerCanSmash = 0;
        }
    }

    game->playerVelocityY += GRAVITY * dt;

    game->playerX += game->playerVelocityX * dt;
    game->playerY += game->playerVelocityY * dt;

    player_was_smashing = game->playerSmashing;

    game_clamp_player_x(game);
    game->playerGrounded = 0;
    game_clamp_player_to_ground(game);

    game_update_spawn_timer(game, dt);
    game_update_entities(game, dt);
    game_handle_collisions(game, player_was_smashing);
}
