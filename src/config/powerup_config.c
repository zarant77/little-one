#include "powerup_config.h"

/* Drop chances are absolute probabilities per killed enemy. Their sum may be
 * at most 1.0; the remaining probability means that no powerup is dropped. */
static const PowerupConfig POWERUP_CONFIGS[] = {
    {
        .id = "powerup.hp",
        .display_name = "HP",
        .display_name_text_id = LOCALIZED_TEXT_FULL_HP,
        .type = POWERUP_HP,
        .sprite_id = "powerup-hp",
        .width = 112,
        .height = 112,
        .pickup_zone = { 0, 0, 48 },
        .drop_chance = 0.05f,
        .duration_ms = 0,
    },
    {
        .id = "powerup.berserk",
        .display_name = "BERSERK",
        .display_name_text_id = LOCALIZED_TEXT_BERSERK,
        .type = POWERUP_BERSERK,
        .sprite_id = "powerup-berserk",
        .width = 112,
        .height = 112,
        .pickup_zone = { 0, 0, 48 },
        .drop_chance = 0.05f,
        .duration_ms = 15000,
    },
};

static const PowerupTuning POWERUP_TUNING = {
    .berserk_world_speed_multiplier = 2.0f,
    .berserk_music_speed_multiplier = 1.50f,
    .berserk_player_scale_permille = 1080,
    .berserk_activation_punch_permille = 120,
    .berserk_activation_punch_ms = 420,
    .timed_indicator_flight_ms = 700,
    .drop_forward_min = 300.0f,
    .drop_forward_max = 600.0f,
    .drop_flight_seconds = 1.0f,
    .drop_upward_velocity = -780.0f,
    .fall_gravity = 1850.0f,
    .pickup_delay_ms = 180,
    .ground_lifetime_ms = 15000,
    .idle_cycle_ms = 900,
    .idle_bob_px = 10,
    .idle_pulse_permille = 75,
};

static const int POWERUP_CONFIG_COUNT =
        sizeof(POWERUP_CONFIGS) / sizeof(POWERUP_CONFIGS[0]);

const PowerupConfig* powerup_config_get_all(int* count) {
    if (count != 0) {
        *count = POWERUP_CONFIG_COUNT;
    }
    return POWERUP_CONFIGS;
}

const PowerupConfig* powerup_config_pick(float roll_01) {
    float cursor = 0.0f;

    if (roll_01 < 0.0f) {
        roll_01 = 0.0f;
    } else if (roll_01 > 1.0f) {
        roll_01 = 1.0f;
    }

    for (int index = 0; index < POWERUP_CONFIG_COUNT; ++index) {
        float chance = POWERUP_CONFIGS[index].drop_chance;

        if (chance <= 0.0f) {
            continue;
        }
        cursor += chance;
        if (roll_01 < cursor) {
            return POWERUP_CONFIGS + index;
        }
    }

    return 0;
}

const PowerupTuning* powerup_tuning_get(void) {
    return &POWERUP_TUNING;
}
