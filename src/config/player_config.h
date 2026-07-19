#ifndef LITTLE_ONE_PLAYER_CONFIG_H
#define LITTLE_ONE_PLAYER_CONFIG_H

#include "../entity/entity_config.h"

/* Visual scale only. Player collision bounds remain configured separately. */
#define PLAYER_SPRITE_RENDER_SCALE 1.0f
#define PLAYER_SWORD_RENDER_SCALE 1.5f
#define PLAYER_SWORD_PIVOT_X 0.5f
#define PLAYER_SWORD_PIVOT_Y 1.0f

typedef struct {
    /* Position of the sword pivot relative to the player's top-left corner. */
    int offset_x;
    int offset_y;
    float rotation_degrees;
} PlayerSwordPoseConfig;

typedef struct {
    PlayerSwordPoseConfig idle;
    PlayerSwordPoseConfig jump;
    PlayerSwordPoseConfig attack;
} PlayerSwordConfig;

typedef struct {
    uint32_t source_color;
    uint32_t full_hp_color;
    uint32_t medium_hp_color;
    uint32_t low_hp_color;
} PlayerEyeColorConfig;

typedef struct {
    int max_hp;
    float moveSpeed;
    float jumpVelocity;
    float smashVelocity;
    HurtZone hurt_zone;
    CollisionBoundary attack_zone;
    EntityVisualConfig visual;
    PlayerSwordConfig sword;
    PlayerEyeColorConfig eye_colors;
} PlayerConfig;

const PlayerConfig* player_config_get(void);

#endif
