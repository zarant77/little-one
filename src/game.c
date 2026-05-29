#include "game.h"

static const float PLAYER_SPEED = 50.0f;
static const float PLAYER_SIZE = 64.0f;
static const float ACTION_DEBUG_MOVE = 32.0f;

static float game_screen_width = 0.0f;
static float game_screen_height = 0.0f;

void game_init(GameState* game) {
    if (game == 0) {
        return;
    }

    game->playerX = 0.0f;
    game->playerY = 0.0f;
}

void game_set_screen_size(GameState* game, float width, float height) {
    int should_center_player = game_screen_width <= 0.0f || game_screen_height <= 0.0f;

    game_screen_width = width;
    game_screen_height = height;

    if (game == 0) {
        return;
    }

    if (should_center_player) {
        game->playerY = (game_screen_height - PLAYER_SIZE) * 0.5f;
        if (game->playerY < 0.0f) {
            game->playerY = 0.0f;
        }
    }
}

void game_update(GameState* game, const InputState* input, float dt) {
    if (game == 0) {
        return;
    }

    if (input != 0 && input->left) {
        game->playerX -= PLAYER_SPEED * dt;
    }

    if (input != 0 && input->right) {
        game->playerX += PLAYER_SPEED * dt;
    }

    if (input != 0 && input->actionPressed) {
        game->playerY -= ACTION_DEBUG_MOVE;
        if (game->playerY < 0.0f) {
            game->playerY = 0.0f;
        }
    }

    if (game->playerX < 0.0f) {
        game->playerX = 0.0f;
    }

    if (game_screen_width > 0.0f && game->playerX >= game_screen_width) {
        game->playerX = 0.0f;
    }
}
