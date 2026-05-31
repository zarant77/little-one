#ifndef LITTLE_ONE_SOUND_DEFINITION_H
#define LITTLE_ONE_SOUND_DEFINITION_H

#include <stdint.h>

typedef enum {
    SOUND_WAVE_SQUARE,
    SOUND_WAVE_SINE,
    SOUND_WAVE_TRIANGLE,
    SOUND_WAVE_NOISE
} SoundWaveKind;

typedef struct {
    SoundWaveKind wave;
    int16_t frequency_start;
    int16_t frequency_end;
    int16_t duration_ms;
    int16_t volume;
} SoundCommand;

typedef struct {
    const char* id;
    const SoundCommand* commands;
    int16_t command_count;
} SoundDefinition;

#endif
