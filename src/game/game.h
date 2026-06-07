#ifndef LITTLE_ONE_GAME_H
#define LITTLE_ONE_GAME_H

#include <stdint.h>

#include "../config/foreground_decoration_config.h"
#include "../entity/entity.h"
#include "../feedback/screen_shake.h"
#include "../input/input.h"
#include "../settings/game_settings.h"
#include "../sprites/generated_sprite.h"
#include "../config/player_config.h"
#include "progression.h"

#define MAX_ENTITIES 16
#define MAX_FLOATING_TEXTS 12

typedef struct {
    int active;
    float x;
    float y;
    float velocity_y;
    int age_ms;
    int lifetime_ms;
    uint32_t color;
    char text[12];
} FloatingText;

typedef struct {
    int active;
    float x;
    float y;
    float scale;
    float alpha;
    const GeneratedSprite* sprite;
    int width;
    int height;
} ForegroundDecoration;

typedef enum {
    GAME_UI_PLAYING = 0,
    GAME_UI_PAUSED = 1,
    GAME_UI_SETTINGS = 2,
    GAME_UI_CAT_SELECT = 3,
    GAME_UI_CAT_UNLOCKED = 4,
    GAME_UI_CATS_COMPLETE = 5
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
    int hitstopMs;
    EntityAnimationState playerAnimation;
    int screenWidth;
    int screenHeight;
    double worldScrollX;
    float worldSpeed;
    Entity entities[MAX_ENTITIES];
    float spawnTimer;
    ForegroundDecoration foregroundDecorations[FOREGROUND_MAX_INSTANCES];
    float foregroundSpawnGap;
    FloatingText floatingTexts[MAX_FLOATING_TEXTS];
    int gameOver;
    int gameOverElapsedMs;
    int gameOverInputArmed;
    int score;
    int bestScore;
    int runTimeMs;
    int fps;
    int averageFrameMs;
    int activeEntityCount;
    int exitRequested;
    ScreenShake screenShake;
    GameUiState uiState;
    GameSettings settings;
    int settingsInitialized;
    ProgressionState progress;
    int progressInitialized;
    int progressDirty;
    int unlockedCatIndex;
    int catUnlockPresentationMs;
} GameState;

void game_init(GameState* game);
void game_restart_run(GameState* game);
int game_try_restart_after_game_over(GameState* game);
int game_start_selected_cat(GameState* game);
void game_show_cat_select(GameState* game);
void game_dismiss_cat_unlocked(GameState* game);
const PlayerConfig* game_player_config(const GameState* game);
const EntityVisualConfig* game_player_visual_config(void);
const HurtZone* game_player_hurt_zone_config(void);
void game_set_screen_size(GameState* game, float width, float height);
void game_update(GameState* game, const InputState* input, float dt);

#endif
