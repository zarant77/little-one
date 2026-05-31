#include "screen_shake.h"

static uint32_t screen_shake_next_random(ScreenShake* shake) {
    shake->random_state = shake->random_state * 1664525u + 1013904223u;
    return shake->random_state;
}

static int32_t screen_shake_random_offset(ScreenShake* shake, int32_t amplitude) {
    uint32_t range;
    uint32_t value;

    if (amplitude <= 0) {
        return 0;
    }

    range = (uint32_t)(amplitude * 2 + 1);
    value = screen_shake_next_random(shake) % range;

    return (int32_t)value - amplitude;
}

void screen_shake_start(
        ScreenShake* shake,
        int32_t strength,
        int32_t duration_ms,
        uint32_t seed
) {
    if (shake == 0) {
        return;
    }

    if (strength < 0) {
        strength = 0;
    }

    if (duration_ms < 0) {
        duration_ms = 0;
    }

    shake->strength = strength;
    shake->duration_ms = duration_ms;
    shake->remaining_ms = duration_ms;
    shake->random_state = seed != 0 ? seed : 1u;
    shake->offset_x = 0;
    shake->offset_y = 0;

    screen_shake_update(shake, 0);
}

void screen_shake_update(ScreenShake* shake, int32_t elapsed_ms) {
    int32_t amplitude;

    if (shake == 0) {
        return;
    }

    if (elapsed_ms < 0) {
        elapsed_ms = 0;
    }

    if (shake->remaining_ms > 0) {
        shake->remaining_ms -= elapsed_ms;
        if (shake->remaining_ms < 0) {
            shake->remaining_ms = 0;
        }
    }

    if (shake->strength <= 0 || shake->duration_ms <= 0 || shake->remaining_ms <= 0) {
        shake->offset_x = 0;
        shake->offset_y = 0;
        return;
    }

    amplitude = (shake->strength * shake->remaining_ms) / shake->duration_ms;
    shake->offset_x = screen_shake_random_offset(shake, amplitude);
    shake->offset_y = screen_shake_random_offset(shake, amplitude);
}

int32_t screen_shake_get_x(const ScreenShake* shake) {
    if (shake == 0) {
        return 0;
    }

    return shake->offset_x;
}

int32_t screen_shake_get_y(const ScreenShake* shake) {
    if (shake == 0) {
        return 0;
    }

    return shake->offset_y;
}
