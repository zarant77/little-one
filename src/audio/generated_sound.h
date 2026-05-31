#ifndef LITTLE_ONE_GENERATED_SOUND_H
#define LITTLE_ONE_GENERATED_SOUND_H

#include <stdint.h>

typedef enum {
    SOUND_NONE = -1,
    SOUND_JUMP = 0,
    SOUND_SMASH,
    SOUND_HIT,
    SOUND_DEATH,
    SOUND_ID_COUNT
} SoundId;

typedef struct {
    const char* id;
    int32_t sample_rate;
    int32_t sample_count;
    int16_t* samples;
} GeneratedSound;

#endif
