#ifndef LITTLE_ONE_GENERATED_MUSIC_H
#define LITTLE_ONE_GENERATED_MUSIC_H

#include <stddef.h>
#include <stdint.h>

#include "music_definition.h"

typedef enum
{
    MUSIC_ID_NONE = -1,
    MUSIC_ID_GAME_LOOP = 0,
    MUSIC_ID_GAME_OVER,
    MUSIC_ID_COUNT
} MusicId;

#define MUSIC_NONE MUSIC_ID_NONE
#define MUSIC_GAME_LOOP MUSIC_ID_GAME_LOOP
#define MUSIC_GAME_OVER MUSIC_ID_GAME_OVER

typedef struct
{
    const char *id;
    int32_t sample_rate;
    int32_t sample_count;
    int32_t loop_enabled;
    int32_t loop_start_sample;
    int32_t loop_end_sample;
    int32_t volume;
    int16_t *samples;
} GeneratedMusic;

extern const PackedMusicDefinition PACKED_MUSIC_DEFINITIONS[];
extern const size_t PACKED_MUSIC_COUNT;

#endif
