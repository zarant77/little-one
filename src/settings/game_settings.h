#ifndef LITTLE_ONE_SETTINGS_GAME_SETTINGS_H
#define LITTLE_ONE_SETTINGS_GAME_SETTINGS_H

typedef struct {
    int music_volume;
    int sfx_volume;
} GameSettings;

void game_settings_init(GameSettings* settings);
int game_settings_clamp_volume(int value);
void game_settings_set_music_volume(GameSettings* settings, int value);
void game_settings_set_sfx_volume(GameSettings* settings, int value);

#endif
