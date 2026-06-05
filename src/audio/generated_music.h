#ifndef LITTLE_ONE_GENERATED_MUSIC_H
#define LITTLE_ONE_GENERATED_MUSIC_H

#include <stddef.h>
#include <stdint.h>

#include "music_definition.h"

typedef enum {
    MUSIC_NONE = -1,
    MUSIC_BATCAT = 0,
    MUSIC_CARRAMBACAT,
    MUSIC_COMMANDOCAT,
    MUSIC_DARKCAT,
    MUSIC_GAME_OVER,
    MUSIC_GHOSTCAT,
    MUSIC_IRON_CAT,
    MUSIC_KOTAN_THE_DESTROYER,
    MUSIC_MAIN_THEME,
    MUSIC_PUNISHCAT,
    MUSIC_ROBOCAT,
    MUSIC_ROBOCOP,
    MUSIC_SAMURCAT,
    MUSIC_SILVERPAW,
    MUSIC_TERMICATOR,
    MUSIC_ZOMBOCAT,
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
