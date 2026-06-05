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
    game_settings_init_with_locale(settings, GAME_LOCALE_ENGLISH);
}

void game_settings_init_with_locale(GameSettings* settings, GameLocale locale)
{
    if (settings == 0)
    {
        return;
    }

    settings->music_volume = 70;
    settings->sfx_volume = 80;
    settings->locale = game_settings_normalize_locale(locale);
}

GameLocale game_settings_normalize_locale(int value)
{
    if (value == GAME_LOCALE_UKRAINIAN)
    {
        return GAME_LOCALE_UKRAINIAN;
    }

    return GAME_LOCALE_ENGLISH;
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

void game_settings_set_locale(GameSettings* settings, GameLocale locale)
{
    if (settings == 0)
    {
        return;
    }

    settings->locale = game_settings_normalize_locale(locale);
}

void game_settings_toggle_locale(GameSettings* settings)
{
    if (settings == 0)
    {
        return;
    }

    if (game_settings_normalize_locale(settings->locale) == GAME_LOCALE_UKRAINIAN)
    {
        settings->locale = GAME_LOCALE_ENGLISH;
        return;
    }

    settings->locale = GAME_LOCALE_UKRAINIAN;
}
