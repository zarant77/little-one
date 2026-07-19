#ifndef LITTLE_ONE_POWERUP_CONFIG_H
#define LITTLE_ONE_POWERUP_CONFIG_H

#include "../entity/entity_config.h"
#include "../localization/localization.h"

typedef enum {
    POWERUP_HP = 0,
    POWERUP_BERSERK
} PowerupType;

typedef struct {
    const char* id;
    const char* display_name;
    LocalizedTextId display_name_text_id;
    PowerupType type;
    const char* sprite_id;
    int width;
    int height;
    HurtZone pickup_zone;
    float drop_chance;
    int duration_ms;
} PowerupConfig;

typedef struct {
    float berserk_world_speed_multiplier;
    float berserk_music_speed_multiplier;
    int berserk_player_scale_permille;
    int berserk_activation_punch_permille;
    int berserk_activation_punch_ms;
    int timed_indicator_flight_ms;
    float drop_forward_min;
    float drop_forward_max;
    float drop_flight_seconds;
    float drop_upward_velocity;
    float fall_gravity;
    int pickup_delay_ms;
    int ground_lifetime_ms;
    int idle_cycle_ms;
    int idle_bob_px;
    int idle_pulse_permille;
} PowerupTuning;

const PowerupConfig* powerup_config_get_all(int* count);
const PowerupConfig* powerup_config_pick(float roll_01);
const PowerupTuning* powerup_tuning_get(void);

#endif
