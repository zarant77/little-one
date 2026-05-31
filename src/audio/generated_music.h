#ifndef LITTLE_ONE_GENERATED_MUSIC_H
#define LITTLE_ONE_GENERATED_MUSIC_H

#include <stdint.h>

typedef enum {
    MUSIC_NONE = -1,
    MUSIC_MAIN_THEME = 0,
    MUSIC_ID_COUNT
} MusicId;

typedef struct {
    const char* id;
    int32_t sample_rate;
    int32_t sample_count;
    int16_t* samples;
} GeneratedMusic;

#endif
