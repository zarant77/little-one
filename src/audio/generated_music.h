#ifndef LITTLE_ONE_GENERATED_MUSIC_H
#define LITTLE_ONE_GENERATED_MUSIC_H

#include <stddef.h>
#include <stdint.h>

#include "music_definition.h"

typedef enum {
    MUSIC_NONE = -1,
    MUSIC_GAME_OVER = 0,
    MUSIC_MAIN_THEME,
    MUSIC_ID_COUNT
} MusicId;

typedef struct {
    const char* id;
    int32_t sample_rate;
    int32_t sample_count;
    int16_t* samples;
} GeneratedMusic;

extern const MusicDefinition PACKED_MUSIC_DEFINITIONS[];
extern const size_t PACKED_MUSIC_COUNT;

#endif
