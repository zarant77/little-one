#include "music_generator.h"

#include <stdint.h>
#include <stdlib.h>

#include "sound_generator.h"

#ifndef MUSIC_ENABLE_AMBIENCE
#define MUSIC_ENABLE_AMBIENCE 1
#endif

enum {
    MUSIC_PI_MILLIRADIANS = 3141,
    MUSIC_TWO_PI_MILLIRADIANS = 6283,
    MUSIC_HALF_PI_MILLIRADIANS = 1571,
    MUSIC_TRIG_SCALE = 32767,
    MUSIC_ENVELOPE_SCALE = 32767,
    MUSIC_CLICK_FADE_MS = 2,
    MUSIC_PRE_LIMITER_PEAK = 39320,
    MUSIC_SOFT_LIMITER_DRIVE_Q15 = 39320,
    MUSIC_FINAL_PEAK = 27852
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

static int32_t music_abs_i32(int32_t value)
{
    if (value == (int32_t)0x80000000)
    {
        return 0x7fffffff;
    }

    return value < 0 ? -value : value;
}

static int32_t music_clamp_i64_to_i32(int64_t value)
{
    if (value > 0x7fffffff)
    {
        return 0x7fffffff;
    }
    if (value < -2147483647 - 1)
    {
        return (int32_t)(-2147483647 - 1);
    }
    return (int32_t)value;
}

static uint32_t music_hash_u32(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

static int music_initial_phase(int midi_note, uint32_t start_tick, uint32_t instrument)
{
    uint32_t seed = (uint32_t)midi_note * 73856093u
            ^ start_tick * 19349663u
            ^ instrument * 83492791u;

    return (int)(music_hash_u32(seed) % MUSIC_TWO_PI_MILLIRADIANS);
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

static int music_sine_q15(int phase)
{
    int normalized = music_normalize_phase(phase);
    int sign = 1;
    int x;
    int64_t x_q15;
    int64_t x2;
    int64_t result;

    if (normalized > MUSIC_PI_MILLIRADIANS)
    {
        normalized -= MUSIC_PI_MILLIRADIANS;
        sign = -1;
    }

    if (normalized > MUSIC_HALF_PI_MILLIRADIANS)
    {
        normalized = MUSIC_PI_MILLIRADIANS - normalized;
    }

    x = normalized;
    x_q15 = ((int64_t)x * MUSIC_TRIG_SCALE) / MUSIC_HALF_PI_MILLIRADIANS;
    x2 = (x_q15 * x_q15) / MUSIC_TRIG_SCALE;
    result = x_q15;
    result -= (((x_q15 * x2) / MUSIC_TRIG_SCALE) / 6);
    result += (((((x_q15 * x2) / MUSIC_TRIG_SCALE) * x2) / MUSIC_TRIG_SCALE) / 120);
    result -= (((((((x_q15 * x2) / MUSIC_TRIG_SCALE) * x2) / MUSIC_TRIG_SCALE) * x2) / MUSIC_TRIG_SCALE) / 5040);

    if (result > MUSIC_TRIG_SCALE)
    {
        result = MUSIC_TRIG_SCALE;
    }

    return (int)(result * sign);
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
    if (wave == SOUND_WAVE_SINE)
    {
        return music_sine_q15(phase);
    }

    if (wave == SOUND_WAVE_TRIANGLE)
    {
        return music_triangle_q15(phase);
    }

    if (wave == SOUND_WAVE_NOISE)
    {
        *noise_state = *noise_state * 1664525u + 1013904223u;
        return (int)((*noise_state >> 16) & 0xffff) - 32768;
    }

    {
        int hard_square = music_normalize_phase(phase) < MUSIC_PI_MILLIRADIANS
                ? MUSIC_TRIG_SCALE
                : -MUSIC_TRIG_SCALE;
        int sine = music_sine_q15(phase);
        return (hard_square * 55 + sine * 45) / 100;
    }
}

static int music_ms_to_samples(int ms)
{
    if (ms <= 0)
    {
        return 0;
    }

    return (int)(((int64_t)ms * SOUND_GENERATOR_SAMPLE_RATE + 500) / 1000);
}

static int music_linear_fade_q15(int position, int fade_samples)
{
    if (fade_samples <= 0 || position >= fade_samples)
    {
        return MUSIC_ENVELOPE_SCALE;
    }

    if (position <= 0)
    {
        return 0;
    }

    return (int)(((int64_t)position * MUSIC_ENVELOPE_SCALE) / fade_samples);
}

static int music_note_envelope_q15(
        const MusicInstrument* instrument,
        int sample_offset,
        int note_sample_count
)
{
    int envelope = MUSIC_ENVELOPE_SCALE;
    int attack_samples;
    int fade_out_samples;
    int click_samples = music_ms_to_samples(MUSIC_CLICK_FADE_MS);
    int remaining_samples = note_sample_count - sample_offset;
    int value;

    if (instrument == 0 || note_sample_count <= 0)
    {
        return 0;
    }

    attack_samples = music_ms_to_samples(instrument->attack_ms);
    fade_out_samples = music_ms_to_samples(
            instrument->release_ms > 0 ? instrument->release_ms : instrument->decay_ms
    );

    value = music_linear_fade_q15(sample_offset, attack_samples);
    if (value < envelope)
    {
        envelope = value;
    }

    value = music_linear_fade_q15(sample_offset, click_samples);
    if (value < envelope)
    {
        envelope = value;
    }

    if (fade_out_samples > 0 && remaining_samples < fade_out_samples)
    {
        value = music_linear_fade_q15(remaining_samples, fade_out_samples);
        if (value < envelope)
        {
            envelope = value;
        }
    }

    if (remaining_samples < click_samples)
    {
        value = music_linear_fade_q15(remaining_samples, click_samples);
        if (value < envelope)
        {
            envelope = value;
        }
    }

    return music_clamp_int(envelope, 0, MUSIC_ENVELOPE_SCALE);
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

    return (int)((numerator + denominator / 2) / denominator);
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
    ticks_per_beat = music_clamp_int(definition->ticks_per_beat, 1, 65535);
    loop_ticks = music_clamp_int(definition->loop_ticks, 1, 4096);
    sample_count = music_tick_to_sample(loop_ticks, bpm, ticks_per_beat);

    return sample_count;
}

static int music_packed_definition_sample_count(const PackedMusicDefinition* definition)
{
    int bpm;
    int ticks_per_beat;
    int sample_count;

    if (definition == 0)
    {
        return 0;
    }

    bpm = music_clamp_int(definition->bpm, 1, 65535);
    ticks_per_beat = music_clamp_int(definition->ticks_per_beat, 1, 65535);
    sample_count = music_tick_to_sample(definition->length_ticks, bpm, ticks_per_beat);
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
    int phase;
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
    phase = music_initial_phase(note->midi_note, (uint32_t)note->start_tick, (uint32_t)note->instrument);
    noise_state = music_hash_u32(0x87654321u
            ^ (uint32_t)note->start_tick * 131u
            ^ (uint32_t)note->midi_note * 17u
            ^ (uint32_t)note->instrument * 911u);

    for (int sample_index = start_sample; sample_index < end_sample; ++sample_index)
    {
        int phase_step;
        int envelope = music_note_envelope_q15(
                instrument,
                sample_index - start_sample,
                end_sample - start_sample
        );
        int amplitude = music_wave_sample(instrument->wave, phase, &noise_state);
        int64_t sample;
        mix[sample_index] = music_clamp_i64_to_i32(
                (int64_t)mix[sample_index] + sample / (100 * MUSIC_ENVELOPE_SCALE)
        );

        phase_step = (int)(((int64_t)frequency_q8 * MUSIC_TWO_PI_MILLIRADIANS)
                / ((int64_t)SOUND_GENERATOR_SAMPLE_RATE * 256));
        phase = music_normalize_phase(phase + phase_step);
    }
}

static int music_packed_loop_is_valid(const PackedMusicDefinition* definition)
{
    if (definition->loop.enabled > 1)
    {
        return 0;
    }

    if (definition->loop.enabled == 0)
    {
        return 1;
    }

    return definition->loop.start_tick < definition->loop.end_tick
            && definition->loop.end_tick <= definition->length_ticks;
}

static int music_packed_definition_is_valid(const PackedMusicDefinition* definition)
{
    if (definition == 0
            || definition->id >= MUSIC_ID_COUNT
            || definition->bpm == 0
            || definition->ticks_per_beat == 0
            || definition->length_ticks == 0
            || definition->instrument_count == 0
            || definition->instruments == 0
            || definition->note_count == 0
            || definition->notes == 0
            || !music_packed_loop_is_valid(definition))
    {
        return 0;
    }

    return 1;
}

static void music_render_packed_voice(
        const PackedMusicInstrument* instrument,
        const PackedMusicNote* note,
        int frequency_q8,
        int phase,
        int gain_percent,
        int start_sample,
        int end_sample,
        int32_t* mix,
        uint32_t noise_state
)
{
    int volume;

    if (instrument == 0 || note == 0 || mix == 0 || end_sample <= start_sample)
    {
        return;
    }

    volume = music_clamp_int(instrument->volume, 0, 100)
            * music_clamp_int(note->volume, 0, 100) / 100;
    volume = volume * music_clamp_int(gain_percent, 0, 100) / 100;

    for (int sample_index = start_sample; sample_index < end_sample; ++sample_index)
    {
        int phase_step;
        MusicInstrument envelope_instrument;
        int amplitude;
        int envelope;
        int64_t sample;

        envelope_instrument.attack_ms = instrument->attack_ms;
        envelope_instrument.decay_ms = instrument->decay_ms;
        if (instrument->flags & MUSIC_INSTRUMENT_FLAG_BASS)
        {
            envelope_instrument.attack_ms = (int16_t)((envelope_instrument.attack_ms * 3) / 4);
        }
        else if (instrument->flags & MUSIC_INSTRUMENT_FLAG_PAD)
        {
            envelope_instrument.attack_ms = (int16_t)music_clamp_int(
                    (envelope_instrument.attack_ms * 5) / 4,
                    0,
                    255
            );
            envelope_instrument.decay_ms = (int16_t)music_clamp_int(
                    (envelope_instrument.decay_ms * 6) / 5,
                    0,
                    255
            );
        }
        else if (instrument->flags & MUSIC_INSTRUMENT_FLAG_SPARK)
        {
            envelope_instrument.decay_ms = (int16_t)((envelope_instrument.decay_ms * 3) / 4);
        }
        envelope_instrument.release_ms = envelope_instrument.decay_ms;
        envelope = music_note_envelope_q15(
                &envelope_instrument,
                sample_index - start_sample,
                end_sample - start_sample
        );
        amplitude = music_wave_sample((SoundWaveKind)instrument->wave, phase, &noise_state);
        sample = (int64_t)amplitude * volume * envelope;
        mix[sample_index] = music_clamp_i64_to_i32(
                (int64_t)mix[sample_index] + sample / (100 * MUSIC_ENVELOPE_SCALE)
        );

        phase_step = (int)(((int64_t)frequency_q8 * MUSIC_TWO_PI_MILLIRADIANS)
                / ((int64_t)SOUND_GENERATOR_SAMPLE_RATE * 256));
        phase = music_normalize_phase(phase + phase_step);
    }
}

static int music_render_packed_note(
        const PackedMusicDefinition* definition,
        const PackedMusicNote* note,
        int32_t* mix,
        int sample_count
)
{
    const PackedMusicInstrument* instrument;
    int start_sample;
    int end_sample;
    int frequency_q8;
    int phase;
    uint32_t noise_state;
    uint8_t flags;

    if (definition == 0 || note == 0 || mix == 0 || sample_count <= 0)
    {
        return 0;
    }

    if (note->instrument >= definition->instrument_count
            || note->note > 127
            || note->duration_ticks == 0
            || note->start_tick >= definition->length_ticks
            || (uint32_t)note->start_tick + note->duration_ticks > definition->length_ticks)
    {
        return 0;
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
        return 1;
    }

    frequency_q8 = music_midi_frequency_q8(note->note);
    phase = music_initial_phase(note->note, note->start_tick, note->instrument);
    noise_state = music_hash_u32(0x87654321u
            ^ (uint32_t)note->start_tick * 131u
            ^ (uint32_t)note->note * 17u
            ^ (uint32_t)note->instrument * 911u);
    flags = instrument->flags;

    if (flags & MUSIC_INSTRUMENT_FLAG_SPARK)
    {
        music_render_packed_voice(instrument, note, frequency_q8, phase, 58, start_sample, end_sample, mix, noise_state);
    }
    else if (flags & MUSIC_INSTRUMENT_FLAG_PAD)
    {
        music_render_packed_voice(instrument, note, frequency_q8, phase, 62, start_sample, end_sample, mix, noise_state);
    }
    else
    {
        music_render_packed_voice(instrument, note, frequency_q8, phase, 72, start_sample, end_sample, mix, noise_state);
    }

    if ((flags & MUSIC_INSTRUMENT_FLAG_BASS) && note->note >= 36)
    {
        music_render_packed_voice(
                instrument,
                note,
                music_midi_frequency_q8(note->note - 12),
                music_normalize_phase(phase + 227),
                24,
                start_sample,
                end_sample,
                mix,
                music_hash_u32(noise_state ^ 0x13572468u)
        );
    }
    else if (flags & MUSIC_INSTRUMENT_FLAG_LEAD)
    {
        music_render_packed_voice(
                instrument,
                note,
                (int)(((int64_t)frequency_q8 * 1003) / 1000),
                music_normalize_phase(phase + 113),
                18,
                start_sample,
                end_sample,
                mix,
                music_hash_u32(noise_state ^ 0x24681357u)
        );
    }

    return 1;
}

static int32_t music_mix_peak(const int32_t* mix, int sample_count)
{
    int32_t peak = 0;

    if (mix == 0 || sample_count <= 0)
    {
        return 0;
    }

    for (int sample_index = 0; sample_index < sample_count; ++sample_index)
    {
        int32_t value = music_abs_i32(mix[sample_index]);
        if (value > peak)
        {
            peak = value;
        }
    }

    return peak;
}

static void music_normalize_mix(int32_t* mix, int sample_count, int32_t target_peak)
{
    int32_t peak = music_mix_peak(mix, sample_count);

    if (mix == 0 || sample_count <= 0 || peak <= 0 || target_peak <= 0)
    {
        return;
    }

    for (int sample_index = 0; sample_index < sample_count; ++sample_index)
    {
        mix[sample_index] = music_clamp_i64_to_i32(
                ((int64_t)mix[sample_index] * target_peak) / peak
        );
    }
}

static void music_apply_ambience(int32_t* mix, int sample_count)
{
#if MUSIC_ENABLE_AMBIENCE
    int delay_1 = music_ms_to_samples(92);
    int delay_2 = music_ms_to_samples(181);

    if (mix == 0 || sample_count <= delay_1 || delay_1 <= 0 || delay_2 <= 0)
    {
        return;
    }

    for (int sample_index = sample_count - 1; sample_index >= delay_1; --sample_index)
    {
        int64_t wet = (int64_t)mix[sample_index - delay_1] * 7 / 100;
        if (sample_index >= delay_2)
        {
            wet += (int64_t)mix[sample_index - delay_2] * 4 / 100;
        }
        mix[sample_index] = music_clamp_i64_to_i32((int64_t)mix[sample_index] + wet);
    }
#else
    (void)mix;
    (void)sample_count;
#endif
}

static void music_apply_soft_limiter(int32_t* mix, int sample_count)
{
    if (mix == 0 || sample_count <= 0)
    {
        return;
    }

    for (int sample_index = 0; sample_index < sample_count; ++sample_index)
    {
        int64_t driven = ((int64_t)mix[sample_index] * MUSIC_SOFT_LIMITER_DRIVE_Q15)
                / MUSIC_ENVELOPE_SCALE;
        int64_t magnitude = driven < 0 ? -driven : driven;
        int64_t denominator = MUSIC_TRIG_SCALE + magnitude;

        if (denominator <= 0)
        {
            mix[sample_index] = 0;
            continue;
        }

        mix[sample_index] = music_clamp_i64_to_i32(
                (driven * MUSIC_TRIG_SCALE) / denominator
        );
    }
}

static void music_convert_mix_to_samples(int32_t* mix, int16_t* samples, int sample_count)
{
    if (mix == 0 || samples == 0 || sample_count <= 0)
    {
        return;
    }

    music_apply_ambience(mix, sample_count);
    music_normalize_mix(mix, sample_count, MUSIC_PRE_LIMITER_PEAK);
    music_apply_soft_limiter(mix, sample_count);
    music_normalize_mix(mix, sample_count, MUSIC_FINAL_PEAK);

    for (int sample_index = 0; sample_index < sample_count; ++sample_index)
    {
        samples[sample_index] = (int16_t)music_clamp_int(mix[sample_index], -32768, 32767);
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
    music->loop_enabled = 0;
    music->loop_start_sample = 0;
    music->loop_end_sample = 0;
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

    music_convert_mix_to_samples(mix, samples, sample_count);

    free(mix);

    music->id = definition->id;
    music->sample_rate = SOUND_GENERATOR_SAMPLE_RATE;
    music->sample_count = sample_count;
    music->loop_enabled = 0;
    music->loop_start_sample = 0;
    music->loop_end_sample = 0;
    music->samples = samples;
    return 1;
}

int music_generator_generate_from_packed(const PackedMusicDefinition* definition, GeneratedMusic* music)
{
    int sample_count;
    int32_t* mix;
    int16_t* samples;

    if (!music_packed_definition_is_valid(definition) || music == 0)
    {
        return 0;
    }

    sample_count = music_packed_definition_sample_count(definition);
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

    for (uint16_t note_index = 0; note_index < definition->note_count; ++note_index)
    {
        if (!music_render_packed_note(
                    definition,
                    definition->notes + note_index,
                    mix,
                    sample_count))
        {
            free(mix);
            free(samples);
            return 0;
        }
    }

    music_convert_mix_to_samples(mix, samples, sample_count);

    free(mix);

    music->id = 0;
    music->sample_rate = SOUND_GENERATOR_SAMPLE_RATE;
    music->sample_count = sample_count;
    music->loop_enabled = definition->loop.enabled != 0 ? 1 : 0;
    if (music->loop_enabled)
    {
        music->loop_start_sample = music_tick_to_sample(
                definition->loop.start_tick,
                definition->bpm,
                definition->ticks_per_beat
        );
        music->loop_end_sample = music_tick_to_sample(
                definition->loop.end_tick,
                definition->bpm,
                definition->ticks_per_beat
        );
        if (music->loop_start_sample < 0
                || music->loop_end_sample > sample_count
                || music->loop_start_sample >= music->loop_end_sample)
        {
            free(samples);
            music->loop_enabled = 0;
            music->loop_start_sample = 0;
            music->loop_end_sample = 0;
            return 0;
        }
    }
    else
    {
        music->loop_start_sample = 0;
        music->loop_end_sample = 0;
    }
    music->samples = samples;
    return 1;
}
