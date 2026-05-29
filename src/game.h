#ifndef LITTLE_ONE_GAME_H
#define LITTLE_ONE_GAME_H

#include "entity.h"
#include "input.h"

#define MAX_ENTITIES 16

typedef struct {
    float playerX;
    float playerY;
    float playerVelocityX;
    float playerVelocityY;
    int playerGrounded;
    int playerSmashing;
    int playerCanSmash;
    int screenWidth;
    int screenHeight;
    float worldScrollX;
    float worldSpeed;
    Entity entities[MAX_ENTITIES];
    float spawnTimer;
    int gameOver;
    int score;
    int bestScore;
    int fps;
    int averageFrameMs;
    int activeEntityCount;
} GameState;

void game_init(GameState* game);
void game_set_screen_size(GameState* game, float width, float height);
void game_update(GameState* game, const InputState* input, float dt);

#endif
