# Cat Meow ↔ Little One Audio Format

## Purpose

Cat Meow exports compact procedural audio source files for Little One.

Little One generates PCM buffers from these audio definitions during startup and plays cached sounds during gameplay.

---

## Pipeline

```text
Cat Meow
    ↓
*.sound.json / *.music.json
    ↓
*.sound.c / *.music.c
    ↓
Little One
    ↓
GeneratedSound
    ↓
Runtime Playback
```

---

## Philosophy

Store instructions.

Generate samples once.

Play cached PCM.

No WAV.

No OGG.

No MP3.

No runtime parsing in Little One.

---

# Format Layers

The audio system uses two formats:

## 1. Editor Format

Used by Cat Meow.

```text
*.sound.json
*.music.json
```

Purpose:

- easy editing
- import/export
- readable data
- future UI compatibility

## 2. Runtime Format

Used by Little One.

```text
*.sound.c
*.music.c
```

Purpose:

- tiny compiled data
- no JSON parser
- no runtime parsing
- fast startup generation
- small APK size

---

# Shared Audio Rules

## Sample Format

Little One generates:

```text
22050 Hz
signed 16-bit
mono PCM
```

## Generated Sound

```c
typedef struct {
    const char *id;

    int sample_rate;
    int sample_count;

    int16_t *samples;
} GeneratedSound;
```

Generated sounds are cached.

Correct:

```c
audio_play_sound("jump");
```

Wrong:

```c
generate_sound_during_gameplay(...);
```

---

# Wave Types

```c
typedef enum {
    SOUND_WAVE_SQUARE   = 0,
    SOUND_WAVE_SINE     = 1,
    SOUND_WAVE_TRIANGLE = 2,
    SOUND_WAVE_NOISE    = 3
} SoundWaveKind;
```

---

# Sound Effects

Sound effects are short procedural audio clips.

Examples:

```text
jump
smash
hit
death
ui_click
```

---

## Sound Command

A sound effect is built from one or more commands.

```c
typedef struct {
    uint8_t wave;

    uint16_t frequency_start;
    uint16_t frequency_end;

    uint16_t duration_ms;

    uint8_t volume;
} SoundCommand;
```

Field meaning:

```text
wave            = oscillator type
frequency_start = starting frequency in Hz
frequency_end   = ending frequency in Hz
duration_ms     = command duration in milliseconds
volume          = 0..100
```

Frequency changes linearly from `frequency_start` to `frequency_end`.

---

## Sound Definition

```c
typedef struct {
    const char *id;

    const SoundCommand *commands;
    int16_t command_count;
} SoundDefinition;
```

---

## Runtime Sound Example

```c
#include "../audio/sound_definition.h"

static const SoundCommand COMMANDS[] = {
    { SOUND_WAVE_SQUARE, 500, 900, 90, 80 }
};

const SoundDefinition JUMP_SOUND = {
    .id = "jump",

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};
```

---

## Editor Sound JSON Example

```json
{
  "type": "sound",
  "id": "jump",
  "commands": [
    {
      "wave": "square",
      "frequencyStart": 500,
      "frequencyEnd": 900,
      "durationMs": 90,
      "volume": 80
    }
  ]
}
```

---

# Music

Music is longer procedural audio generated from instruments and notes.

Music should also be generated into a cached `GeneratedSound`.

For MVP, music is rendered into one PCM buffer and looped during playback.

---

## Music Instrument

```c
typedef struct {
    uint8_t wave;

    uint8_t volume;

    uint16_t attack_ms;
    uint16_t decay_ms;
} MusicInstrument;
```

Field meaning:

```text
wave      = oscillator type
volume    = 0..100
attack_ms = fade-in time
decay_ms  = fade-out time
```

---

## Music Note

```c
typedef struct {
    uint8_t instrument;

    uint8_t note;

    uint16_t start_tick;
    uint16_t duration_ticks;

    uint8_t volume;
} MusicNote;
```

Field meaning:

```text
instrument     = index in instrument array
note           = MIDI note number
start_tick     = note start position
duration_ticks = note length
volume         = 0..100
```

Notes use MIDI note numbers.

Examples:

```text
60 = C4
62 = D4
64 = E4
65 = F4
67 = G4
69 = A4
71 = B4
72 = C5
```

---

## Music Definition

```c
typedef struct {
    const char *id;

    uint16_t bpm;
    uint8_t ticks_per_beat;
    uint16_t length_ticks;

    const MusicInstrument *instruments;
    int16_t instrument_count;

    const MusicNote *notes;
    int16_t note_count;
} MusicDefinition;
```

---

## Runtime Music Example

```c
#include "../audio/music_definition.h"

static const MusicInstrument INSTRUMENTS[] = {
    { SOUND_WAVE_SQUARE,   70, 5, 40 },
    { SOUND_WAVE_TRIANGLE, 50, 8, 80 }
};

static const MusicNote NOTES[] = {
    { 0, 64,  0, 2, 80 },
    { 0, 67,  2, 2, 80 },
    { 0, 69,  4, 4, 80 },
    { 1, 52,  0, 8, 60 }
};

const MusicDefinition MAIN_THEME_MUSIC = {
    .id = "main_theme",

    .bpm = 120,
    .ticks_per_beat = 4,
    .length_ticks = 8,

    .instruments = INSTRUMENTS,
    .instrument_count = sizeof(INSTRUMENTS) / sizeof(INSTRUMENTS[0]),

    .notes = NOTES,
    .note_count = sizeof(NOTES) / sizeof(NOTES[0]),
};
```

---

## Editor Music JSON Example

```json
{
  "type": "music",
  "id": "main_theme",
  "bpm": 120,
  "ticksPerBeat": 4,
  "lengthTicks": 8,
  "instruments": [
    {
      "id": "lead",
      "wave": "square",
      "volume": 70,
      "attackMs": 5,
      "decayMs": 40
    },
    {
      "id": "bass",
      "wave": "triangle",
      "volume": 50,
      "attackMs": 8,
      "decayMs": 80
    }
  ],
  "notes": [
    {
      "instrument": 0,
      "note": 64,
      "startTick": 0,
      "durationTicks": 2,
      "volume": 80
    },
    {
      "instrument": 0,
      "note": 67,
      "startTick": 2,
      "durationTicks": 2,
      "volume": 80
    },
    {
      "instrument": 0,
      "note": 69,
      "startTick": 4,
      "durationTicks": 4,
      "volume": 80
    },
    {
      "instrument": 1,
      "note": 52,
      "startTick": 0,
      "durationTicks": 8,
      "volume": 60
    }
  ]
}
```

---

# Mixing Rules

Little One should support:

```text
music + sound effect
```

Minimum runtime channels:

```text
1 music channel
1 sound effect channel
```

MVP behavior:

- one music track can loop
- one sound effect can play over music
- if another sound effect starts, it may replace the current sound effect
- advanced mixing can be added later

---

# Registries

Cat Meow exports individual files.

Little One owns the registries.

---

## Sound Registry Example

```c
extern const SoundDefinition JUMP_SOUND;
extern const SoundDefinition SMASH_SOUND;
extern const SoundDefinition HIT_SOUND;
extern const SoundDefinition DEATH_SOUND;

const SoundDefinition *SOUND_DEFINITIONS[] = {
    &JUMP_SOUND,
    &SMASH_SOUND,
    &HIT_SOUND,
    &DEATH_SOUND,
};
```

---

## Music Registry Example

```c
extern const MusicDefinition MAIN_THEME_MUSIC;

const MusicDefinition *MUSIC_DEFINITIONS[] = {
    &MAIN_THEME_MUSIC,
};
```

---

# Suggested File Layout

```text
src/audio/
    sound_definition.h
    music_definition.h
    generated_sound.h

    sound_generator.c
    sound_generator.h

    music_generator.c
    music_generator.h

    sound_registry.c
    sound_registry.h

    music_registry.c
    music_registry.h

src/audio/definitions/sounds/
    jump.sound.c
    smash.sound.c
    hit.sound.c
    death.sound.c

src/audio/definitions/music/
    main_theme.music.c
```

Cat Meow project files:

```text
sounds/
    jump.sound.json
    smash.sound.json
    hit.sound.json
    death.sound.json

music/
    main_theme.music.json
```

---

# Naming Rules

Allowed audio IDs:

```text
a-z
0-9
_
-
```

Examples:

```text
jump
smash
hit
death
ui_click
main_theme
menu_theme
forest_loop
```

Suggested C symbols:

```text
JUMP_SOUND
SMASH_SOUND
MAIN_THEME_MUSIC
FOREST_LOOP_MUSIC
```

---

# Export Requirements

Cat Meow exports:

## For editor use

```text
*.sound.json
*.music.json
```

## For Little One

```text
*.sound.c
*.music.c
```

Each runtime file must contain:

- include statement
- static command/note/instrument arrays
- one public `const SoundDefinition` or `const MusicDefinition`

---

# Runtime Rules

Correct:

```c
audio_play_sound("jump");
audio_play_music("main_theme");
```

Wrong:

```c
parse_json_audio_file(...);
load_wav_file(...);
generate_music_every_frame(...);
```

Little One must never parse Cat Meow JSON files.

JSON is for the editor only.

C definitions are for the game only.

---

# Design Goal

Tiny source data.

Tiny APK.

Fast startup.

Fast playback.

No audio assets.

No runtime parsers.

Procedural sound for a procedural little warrior.
