#include "music_generator.h"

#include <stdint.h>
#include <stdlib.h>

#include "sound_generator.h"

enum {
    MUSIC_PI_MILLIRADIANS = 3141,
    MUSIC_TWO_PI_MILLIRADIANS = 6283,
    MUSIC_HALF_PI_MILLIRADIANS = 1571,
    MUSIC_TRIG_SCALE = 32767,
    MUSIC_MAX_SECONDS = 16
};

static int music_clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static int music_normalize_phase(int phase)
{
    while (phase >= MUSIC_TWO_PI_MILLIRADIANS)
    {
        phase -= MUSIC_TWO_PI_MILLIRADIANS;
    }
    while (phase < 0)
    {
        phase += MUSIC_TWO_PI_MILLIRADIANS;
    }
    return phase;
}

static int music_triangle_q15(int phase)
{
    int normalized = music_normalize_phase(phase);
    int value;

    if (normalized < MUSIC_HALF_PI_MILLIRADIANS)
    {
        value = (normalized * MUSIC_TRIG_SCALE) / MUSIC_HALF_PI_MILLIRADIANS;
    }
    else if (normalized < MUSIC_PI_MILLIRADIANS + MUSIC_HALF_PI_MILLIRADIANS)
    {
        value = MUSIC_TRIG_SCALE
                - ((normalized - MUSIC_HALF_PI_MILLIRADIANS) * 2 * MUSIC_TRIG_SCALE)
                / MUSIC_PI_MILLIRADIANS;
    }
    else
    {
        value = -MUSIC_TRIG_SCALE
                + ((normalized - MUSIC_PI_MILLIRADIANS - MUSIC_HALF_PI_MILLIRADIANS)
                * MUSIC_TRIG_SCALE) / MUSIC_HALF_PI_MILLIRADIANS;
    }

    return music_clamp_int(value, -MUSIC_TRIG_SCALE, MUSIC_TRIG_SCALE);
}

static int music_wave_sample(SoundWaveKind wave, int phase, uint32_t* noise_state)
{
    if (wave == SOUND_WAVE_TRIANGLE)
    {
        return music_triangle_q15(phase);
    }

    if (wave == SOUND_WAVE_NOISE)
    {
        *noise_state = *noise_state * 1664525u + 1013904223u;
        return (int)((*noise_state >> 16) & 0xffff) - 32768;
    }

    return music_normalize_phase(phase) < MUSIC_PI_MILLIRADIANS
            ? MUSIC_TRIG_SCALE
            : -MUSIC_TRIG_SCALE;
}

static int music_midi_frequency_q8(int midi_note)
{
    static const int octave_four_freq_q8[] = {
            66978, 70960, 75178, 79648, 84383, 89401,
            94718, 100350, 106317, 112639, 119337, 126433
    };
    int note = music_clamp_int(midi_note, 0, 127);
    int octave_delta = note / 12 - 5;
    int frequency_q8 = octave_four_freq_q8[note % 12];

    while (octave_delta > 0)
    {
        frequency_q8 *= 2;
        octave_delta -= 1;
    }
    while (octave_delta < 0)
    {
        frequency_q8 /= 2;
        octave_delta += 1;
    }

    return frequency_q8;
}

static int music_tick_to_sample(int tick, int bpm, int ticks_per_beat)
{
    int64_t numerator = (int64_t)tick * 60 * SOUND_GENERATOR_SAMPLE_RATE;
    int64_t denominator = (int64_t)bpm * ticks_per_beat;

    if (denominator <= 0)
    {
        return 0;
    }

    return (int)(numerator / denominator);
}

static int music_definition_sample_count(const MusicDefinition* definition)
{
    int bpm;
    int ticks_per_beat;
    int loop_ticks;
    int sample_count;

    if (definition == 0)
    {
        return 0;
    }

    bpm = music_clamp_int(definition->bpm, 30, 240);
    ticks_per_beat = music_clamp_int(definition->ticks_per_beat, 1, 96);
    loop_ticks = music_clamp_int(definition->loop_ticks, 1, 4096);
    sample_count = music_tick_to_sample(loop_ticks, bpm, ticks_per_beat);

    if (sample_count > SOUND_GENERATOR_SAMPLE_RATE * MUSIC_MAX_SECONDS)
    {
        sample_count = SOUND_GENERATOR_SAMPLE_RATE * MUSIC_MAX_SECONDS;
    }

    return sample_count;
}

static void music_render_note(
        const MusicDefinition* definition,
        const MusicNote* note,
        int32_t* mix,
        int sample_count
)
{
    const MusicInstrument* instrument;
    int start_sample;
    int end_sample;
    int frequency_q8;
    int phase = 0;
    int volume;
    uint32_t noise_state;

    if (definition == 0 || note == 0 || mix == 0 || sample_count <= 0)
    {
        return;
    }

    if (note->instrument < 0 || note->instrument >= definition->instrument_count)
    {
        return;
    }

    instrument = definition->instruments + note->instrument;
    start_sample = music_tick_to_sample(
            note->start_tick,
            definition->bpm,
            definition->ticks_per_beat
    );
    end_sample = music_tick_to_sample(
            note->start_tick + note->duration_ticks,
            definition->bpm,
            definition->ticks_per_beat
    );

    if (start_sample < 0)
    {
        start_sample = 0;
    }
    if (end_sample > sample_count)
    {
        end_sample = sample_count;
    }
    if (end_sample <= start_sample)
    {
        return;
    }

    frequency_q8 = music_midi_frequency_q8(note->midi_note);
    volume = music_clamp_int(instrument->volume, 0, 100)
            * music_clamp_int(note->volume, 0, 100) / 100;
    noise_state = (uint32_t)(0x87654321u + (uint32_t)note->start_tick * 131u);

    for (int sample_index = start_sample; sample_index < end_sample; ++sample_index)
    {
        int phase_step;
        int amplitude = music_wave_sample(instrument->wave, phase, &noise_state);
        mix[sample_index] += (amplitude * volume) / 100;

        phase_step = (int)(((int64_t)frequency_q8 * MUSIC_TWO_PI_MILLIRADIANS)
                / ((int64_t)SOUND_GENERATOR_SAMPLE_RATE * 256));
        phase = music_normalize_phase(phase + phase_step);
    }
}

void music_generator_release(GeneratedMusic* music)
{
    if (music == 0)
    {
        return;
    }

    free(music->samples);
    music->id = 0;
    music->sample_rate = 0;
    music->sample_count = 0;
    music->samples = 0;
}

int music_generator_generate(const MusicDefinition* definition, GeneratedMusic* music)
{
    int sample_count;
    int32_t* mix;
    int16_t* samples;

    if (definition == 0
            || music == 0
            || definition->id == 0
            || definition->instrument_count <= 0
            || definition->instruments == 0
            || definition->note_count <= 0
            || definition->notes == 0)
    {
        return 0;
    }

    sample_count = music_definition_sample_count(definition);
    if (sample_count <= 0)
    {
        return 0;
    }

    music_generator_release(music);

    mix = (int32_t*)calloc((size_t)sample_count, sizeof(int32_t));
    samples = (int16_t*)calloc((size_t)sample_count, sizeof(int16_t));
    if (mix == 0 || samples == 0)
    {
        free(mix);
        free(samples);
        return 0;
    }

    for (int note_index = 0; note_index < definition->note_count; ++note_index)
    {
        music_render_note(definition, definition->notes + note_index, mix, sample_count);
    }

    for (int sample_index = 0; sample_index < sample_count; ++sample_index)
    {
        samples[sample_index] = (int16_t)music_clamp_int(mix[sample_index], -32768, 32767);
    }

    free(mix);

    music->id = definition->id;
    music->sample_rate = SOUND_GENERATOR_SAMPLE_RATE;
    music->sample_count = sample_count;
    music->samples = samples;
    return 1;
}
