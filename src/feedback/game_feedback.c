#include "game_feedback.h"

static uint32_t game_feedback_seed = 0x1234abcdU;

static uint32_t game_feedback_next_seed(void)
{
    game_feedback_seed = game_feedback_seed * 1103515245u + 12345u;
    return game_feedback_seed;
}

void game_feedback_smash_land(ScreenShake *shake)
{
    screen_shake_start(shake, 20, 240, game_feedback_next_seed());
}

void game_feedback_player_death(ScreenShake *shake)
{
    (void)shake;
}
