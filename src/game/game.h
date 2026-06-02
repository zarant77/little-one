#ifndef LITTLE_ONE_GAME_H
#define LITTLE_ONE_GAME_H

#include "../entity/entity.h"
#include "../feedback/screen_shake.h"
#include "../input/input.h"

#define MAX_ENTITIES 16

typedef struct {
    float playerX;
    float playerY;
    float playerVelocityX;
    float playerVelocityY;
    int playerGrounded;
    int playerSmashing;
    int playerCanSmash;
    EntityAnimationState playerAnimation;
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
    ScreenShake screenShake;
} GameState;

void game_init(GameState* game);
const EntityVisualConfig* game_player_visual_config(void);
const HurtZone* game_player_hurt_zone_config(void);
void game_set_screen_size(GameState* game, float width, float height);
void game_update(GameState* game, const InputState* input, float dt);

#endif
