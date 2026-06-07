#ifndef LITTLE_ONE_MUSIC_DEFINITION_H
#define LITTLE_ONE_MUSIC_DEFINITION_H

#include <stdint.h>

#include "sound_definition.h"

typedef struct {
    SoundWaveKind wave;
    int16_t volume;
    int16_t attack_ms;
    int16_t decay_ms;
    int16_t sustain;
    int16_t release_ms;
} MusicInstrument;

typedef struct {
    int16_t instrument;
    int16_t midi_note;
    int16_t start_tick;
    int16_t duration_ticks;
    int16_t volume;
} MusicNote;

typedef struct {
    uint8_t enabled;
    uint16_t start_tick;
    uint16_t end_tick;
} MusicLoop;

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

#ifndef PACKED_MUSIC_FORMAT_VERSION
#define PACKED_MUSIC_FORMAT_VERSION 3
#endif

#define MUSIC_INSTRUMENT_FLAG_BASS 1
#define MUSIC_INSTRUMENT_FLAG_LEAD 2
#define MUSIC_INSTRUMENT_FLAG_PAD 4
#define MUSIC_INSTRUMENT_FLAG_SPARK 8

#pragma pack(push, 1)

typedef struct {
    uint8_t instrument;
    uint8_t note;
    uint8_t volume;
    uint16_t start_tick;
    uint16_t duration_ticks;
} PackedMusicNote;

typedef struct {
    uint8_t wave;
    uint8_t volume;
    uint8_t attack_ms;
    uint8_t decay_ms;
    uint8_t flags;
} PackedMusicInstrument;

#pragma pack(pop)

typedef struct {
    uint16_t id;
    uint16_t bpm;
    uint16_t ticks_per_beat;
    uint16_t length_ticks;
    MusicLoop loop;

    const PackedMusicInstrument* instruments;
    uint8_t instrument_count;

    const PackedMusicNote* notes;
    uint16_t note_count;
} PackedMusicDefinition;

#endif
