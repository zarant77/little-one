#include "game.h"

static const float PLAYER_SIZE = 64.0f;
static const float PLAYER_SPEED = 220.0f;
static const float GRAVITY = 1400.0f;
static const float JUMP_VELOCITY = -620.0f;
static const float SMASH_VELOCITY = 1200.0f;
static const float GROUND_MARGIN = 48.0f;
static const float WORLD_SPEED = 120.0f;
static const float WORLD_SCROLL_WRAP = 10000.0f;

static float game_ground_y(const GameState* game) {
    return (float)game->screenHeight - GROUND_MARGIN;
}

static void game_clamp_player_x(GameState* game) {
    float max_x = (float)game->screenWidth - PLAYER_SIZE;

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
    float player_bottom = game->playerY + PLAYER_SIZE;

    if (game->playerY < 0.0f) {
        game->playerY = 0.0f;
        if (game->playerVelocityY < 0.0f) {
            game->playerVelocityY = 0.0f;
        }
    }

    if (player_bottom >= ground_y) {
        game->playerY = ground_y - PLAYER_SIZE;
        if (game->playerY < 0.0f) {
            game->playerY = 0.0f;
        }
        game->playerVelocityY = 0.0f;
        game->playerGrounded = 1;
        game->playerSmashing = 0;
        game->playerCanSmash = 0;
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
        game->playerY = game_ground_y(game) - PLAYER_SIZE;
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
    if (game == 0) {
        return;
    }

    game->worldScrollX += game->worldSpeed * dt;
    while (game->worldScrollX >= WORLD_SCROLL_WRAP) {
        game->worldScrollX -= WORLD_SCROLL_WRAP;
    }

    /* Screen-space Y grows downward, matching the framebuffer. Negative Y velocity jumps upward. */
    game->playerVelocityX = 0.0f;

    if (input != 0 && input->left) {
        game->playerVelocityX = -PLAYER_SPEED;
    }

    if (input != 0 && input->right) {
        game->playerVelocityX = PLAYER_SPEED;
    }

    if (input != 0 && input->actionPressed) {
        if (game->playerGrounded) {
            game->playerVelocityY = JUMP_VELOCITY;
            game->playerGrounded = 0;
            game->playerSmashing = 0;
            game->playerCanSmash = 1;
        } else if (game->playerCanSmash) {
            game->playerVelocityY = SMASH_VELOCITY;
            game->playerSmashing = 1;
            game->playerCanSmash = 0;
        }
    }

    game->playerVelocityY += GRAVITY * dt;

    game->playerX += game->playerVelocityX * dt;
    game->playerY += game->playerVelocityY * dt;

    game_clamp_player_x(game);
    game->playerGrounded = 0;
    game_clamp_player_to_ground(game);
}
