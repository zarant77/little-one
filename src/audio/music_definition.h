#ifndef LITTLE_ONE_MUSIC_DEFINITION_H
#define LITTLE_ONE_MUSIC_DEFINITION_H

#include <stdint.h>

#include "sound_definition.h"

typedef struct {
    SoundWaveKind wave;
    int16_t volume;
} MusicInstrument;

typedef struct {
    int16_t instrument;
    int16_t midi_note;
    int16_t start_tick;
    int16_t duration_ticks;
    int16_t volume;
} MusicNote;

typedef struct {
    const char* id;
    int16_t bpm;
    int16_t ticks_per_beat;
    int16_t loop_ticks;
    const MusicInstrument* instruments;
    int16_t instrument_count;
    const MusicNote* notes;
    int16_t note_count;
} MusicDefinition;

#endif
