#ifndef LITTLE_ONE_SCREEN_SHAKE_H
#define LITTLE_ONE_SCREEN_SHAKE_H

#include <stdint.h>

typedef struct {
    int32_t strength;
    int32_t duration_ms;
    int32_t remaining_ms;
    uint32_t random_state;
    int32_t offset_x;
    int32_t offset_y;
} ScreenShake;

void screen_shake_start(
        ScreenShake* shake,
        int32_t strength,
        int32_t duration_ms,
        uint32_t seed
);
void screen_shake_update(ScreenShake* shake, int32_t elapsed_ms);
int32_t screen_shake_get_x(const ScreenShake* shake);
int32_t screen_shake_get_y(const ScreenShake* shake);

#endif
