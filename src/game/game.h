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

#define MAX_ENTITIES 24
#define MAX_FLOATING_TEXTS 12
#define MAX_TIMED_POWERUPS 4
#define PLAYER_LOST_HEART_LIFETIME_MS 1500

typedef struct {
    int active;
    float x;
    float y;
    float velocity_y;
    int age_ms;
    int lifetime_ms;
    uint32_t color;
    char text[32];
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

typedef struct {
    int active;
    const PowerupConfig* config;
    int duration_ms;
    int remaining_ms;
    int age_ms;
    float origin_x;
    float origin_y;
} TimedPowerupState;

typedef enum {
    GAME_UI_PLAYING = 0,
    GAME_UI_SETTINGS = 1,
    GAME_UI_MENU = 2,
    GAME_UI_HELP = 3
} GameUiState;

typedef struct {
    float playerX;
    float playerY;
    float playerVelocityX;
    float playerVelocityY;
    int playerGrounded;
    int playerSmashing;
    int playerAttackPoseMs;
    int playerCanSmash;
    int playerHp;
    int playerInvulnerableMs;
    int playerLostHeartActive;
    int playerLostHeartIndex;
    int playerLostHeartAgeMs;
    int playerLostHeartStartTimeMs;
    float playerLostHeartOriginX;
    float playerLostHeartOriginY;
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
    TimedPowerupState timedPowerups[MAX_TIMED_POWERUPS];
    int gameOver;
    int gameOverElapsedMs;
    int gameOverInputArmed;
    int score;
    int bestScore;
    int newRecord;
    int runTimeMs;
    int visualTimeMs;
    int fps;
    int averageFrameMs;
    int activeEntityCount;
    int exitRequested;
    int runStarted;
    ScreenShake screenShake;
    GameUiState uiState;
    GameUiState helpReturnState;
    GameSettings settings;
    int settingsInitialized;
    int settingsDirty;
    ProgressionState progress;
    int progressInitialized;
    int progressDirty;
} GameState;

void game_init(GameState* game);
void game_restart_run(GameState* game);
int game_try_restart_after_game_over(GameState* game);
int game_is_game_over_screen_visible(const GameState* game);
int game_start_run(GameState* game);
void game_show_menu(GameState* game);
const PlayerConfig* game_player_config(const GameState* game);
const EntityVisualConfig* game_player_visual_config(void);
const HurtZone* game_player_hurt_zone_config(void);
const TimedPowerupState* game_timed_powerup_get(
        const GameState* game,
        PowerupType type
);
void game_set_screen_size(GameState* game, float width, float height);
void game_update(GameState* game, const InputState* input, float dt);

#endif
