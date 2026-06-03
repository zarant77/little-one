#include "game_settings.h"

int game_settings_clamp_volume(int value)
{
    if (value < 0)
    {
        return 0;
    }

    if (value > 100)
    {
        return 100;
    }

    return value;
}

void game_settings_init(GameSettings* settings)
{
    if (settings == 0)
    {
        return;
    }

    settings->music_volume = 70;
    settings->sfx_volume = 80;
}

void game_settings_set_music_volume(GameSettings* settings, int value)
{
    if (settings == 0)
    {
        return;
    }

    settings->music_volume = game_settings_clamp_volume(value);
}

void game_settings_set_sfx_volume(GameSettings* settings, int value)
{
    if (settings == 0)
    {
        return;
    }

    settings->sfx_volume = game_settings_clamp_volume(value);
}
