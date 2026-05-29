#ifndef LITTLE_ONE_GAME_H
#define LITTLE_ONE_GAME_H

#include "input.h"

typedef struct {
    float playerX;
    float playerY;
} GameState;

void game_init(GameState* game);
void game_set_screen_size(GameState* game, float width, float height);
void game_update(GameState* game, const InputState* input, float dt);

#endif
