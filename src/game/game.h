#ifndef LITTLE_ONE_GAME_H
#define LITTLE_ONE_GAME_H

#include "../config/foreground_decoration_config.h"
#include "../entity/entity.h"
#include "../feedback/screen_shake.h"
#include "../input/input.h"
#include "../settings/game_settings.h"
#include "../sprites/generated_sprite.h"

#define MAX_ENTITIES 16

typedef struct {
    int active;
    float x;
    float y;
    float scale;
    const GeneratedSprite* sprite;
    int width;
    int height;
} ForegroundDecoration;

typedef enum {
    GAME_UI_PLAYING = 0,
    GAME_UI_PAUSED = 1,
    GAME_UI_SETTINGS = 2
} GameUiState;

typedef struct {
    float playerX;
    float playerY;
    float playerVelocityX;
    float playerVelocityY;
    int playerGrounded;
    int playerSmashing;
    int playerCanSmash;
    int playerHp;
    int playerInvulnerableMs;
    EntityAnimationState playerAnimation;
    int screenWidth;
    int screenHeight;
    float worldScrollX;
    float worldSpeed;
    Entity entities[MAX_ENTITIES];
    float spawnTimer;
    ForegroundDecoration foregroundDecorations[FOREGROUND_MAX_INSTANCES];
    float foregroundSpawnGap;
    int gameOver;
    int score;
    int bestScore;
    int runTimeMs;
    int fps;
    int averageFrameMs;
    int activeEntityCount;
    ScreenShake screenShake;
    GameUiState uiState;
    GameSettings settings;
    int settingsInitialized;
} GameState;

void game_init(GameState* game);
void game_restart_run(GameState* game);
const EntityVisualConfig* game_player_visual_config(void);
const HurtZone* game_player_hurt_zone_config(void);
void game_set_screen_size(GameState* game, float width, float height);
void game_update(GameState* game, const InputState* input, float dt);

#endif
