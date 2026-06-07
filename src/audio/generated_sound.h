#ifndef LITTLE_ONE_GENERATED_SOUND_H
#define LITTLE_ONE_GENERATED_SOUND_H

#include <stddef.h>
#include <stdint.h>

#include "sound_definition.h"

typedef enum {
    SOUND_NONE = -1,
    SOUND_JUMP = 0,
    SOUND_SMASH,
    SOUND_DAMAGE,
    SOUND_DEATH,
    SOUND_ORK_DEATH,
    SOUND_BOAR_DEATH,
    SOUND_RAT_DEATH,
    SOUND_CAT_UNLOCK,
    SOUND_ID_COUNT
} SoundId;

typedef struct {
    const char* id;
    int32_t sample_rate;
    int32_t sample_count;
    int16_t* samples;
} GeneratedSound;

extern const SoundDefinition PACKED_SOUND_DEFINITIONS[];
extern const size_t PACKED_SOUND_COUNT;

#endif
