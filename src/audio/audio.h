#ifndef LITTLE_ONE_AUDIO_H
#define LITTLE_ONE_AUDIO_H

void audio_init(void);
void audio_shutdown(void);
void audio_set_music_volume(int volume);
void audio_set_sfx_volume(int volume);
void audio_play_sound(const char* id);
void audio_play_music(const char* id);
void audio_stop_music(void);

#endif
