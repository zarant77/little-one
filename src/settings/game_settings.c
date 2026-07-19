#include "game_settings.h"

#include <stdio.h>

#define GAME_SETTINGS_SAVE_VERSION 2

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
    settings->help_seen = 0;
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

void game_settings_mark_help_seen(GameSettings* settings)
{
    if (settings == 0)
    {
        return;
    }

    settings->help_seen = 1;
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

int game_settings_load_from_path(const char* path, GameSettings* settings)
{
    FILE* file;
    int version;
    GameSettings loaded;

    if (path == 0 || settings == 0)
    {
        return 0;
    }

    file = fopen(path, "r");
    if (file == 0)
    {
        return 0;
    }

    game_settings_init(&loaded);
    if (fscanf(file, "little_one_settings %d\n", &version) != 1)
    {
        fclose(file);
        return 0;
    }

    if (version == 1)
    {
        if (fscanf(
                file,
                "music %d\nsfx %d\nlocale %d\n",
                &loaded.music_volume,
                &loaded.sfx_volume,
                &loaded.locale) != 3)
        {
            fclose(file);
            return 0;
        }

        /* Existing players already know the controls; only new installs
         * should receive the first-launch help popup. */
        loaded.help_seen = 1;
    }
    else if (version == GAME_SETTINGS_SAVE_VERSION)
    {
        if (fscanf(
                file,
                "music %d\nsfx %d\nlocale %d\nhelp_seen %d\n",
                &loaded.music_volume,
                &loaded.sfx_volume,
                &loaded.locale,
                &loaded.help_seen) != 4)
        {
            fclose(file);
            return 0;
        }
    }
    else
    {
        fclose(file);
        return 0;
    }

    fclose(file);
    loaded.music_volume = game_settings_clamp_volume(loaded.music_volume);
    loaded.sfx_volume = game_settings_clamp_volume(loaded.sfx_volume);
    loaded.locale = game_settings_normalize_locale(loaded.locale);
    loaded.help_seen = loaded.help_seen != 0;
    *settings = loaded;
    return 1;
}

int game_settings_save_to_path(const char* path, const GameSettings* settings)
{
    FILE* file;
    int result;

    if (path == 0 || settings == 0)
    {
        return 0;
    }

    file = fopen(path, "w");
    if (file == 0)
    {
        return 0;
    }

    result = fprintf(
            file,
            "little_one_settings %d\nmusic %d\nsfx %d\nlocale %d\nhelp_seen %d\n",
            GAME_SETTINGS_SAVE_VERSION,
            game_settings_clamp_volume(settings->music_volume),
            game_settings_clamp_volume(settings->sfx_volume),
            game_settings_normalize_locale(settings->locale),
            settings->help_seen != 0);

    fclose(file);
    return result > 0;
}
