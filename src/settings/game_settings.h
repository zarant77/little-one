#ifndef LITTLE_ONE_SETTINGS_GAME_SETTINGS_H
#define LITTLE_ONE_SETTINGS_GAME_SETTINGS_H

typedef struct {
    int music_volume;
    int sfx_volume;
    int locale;
} GameSettings;

typedef enum {
    GAME_LOCALE_ENGLISH = 0,
    GAME_LOCALE_UKRAINIAN = 1
} GameLocale;

void game_settings_init(GameSettings* settings);
void game_settings_init_with_locale(GameSettings* settings, GameLocale locale);
int game_settings_clamp_volume(int value);
GameLocale game_settings_normalize_locale(int value);
void game_settings_set_music_volume(GameSettings* settings, int value);
void game_settings_set_sfx_volume(GameSettings* settings, int value);
void game_settings_set_locale(GameSettings* settings, GameLocale locale);
void game_settings_toggle_locale(GameSettings* settings);
int game_settings_load_from_path(const char* path, GameSettings* settings);
int game_settings_save_to_path(const char* path, const GameSettings* settings);

#endif
