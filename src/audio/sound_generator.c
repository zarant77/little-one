#include "sound_generator.h"

#include <stdint.h>
#include <stdlib.h>

enum {
    SOUND_PI_MILLIRADIANS = 3141,
    SOUND_TWO_PI_MILLIRADIANS = 6283,
    SOUND_HALF_PI_MILLIRADIANS = 1571,
    SOUND_TRIG_SCALE = 32767,
    SOUND_MAX_DURATION_MS = 2000
};

static int sound_clamp_int(int value, int min_value, int max_value)
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

static int sound_normalize_phase(int phase)
{
    while (phase >= SOUND_TWO_PI_MILLIRADIANS)
    {
        phase -= SOUND_TWO_PI_MILLIRADIANS;
    }
    while (phase < 0)
    {
        phase += SOUND_TWO_PI_MILLIRADIANS;
    }
    return phase;
}

static int sound_sin_q15(int phase)
{
    int normalized = sound_normalize_phase(phase);
    int64_t x;
    int64_t x2;
    int64_t result;

    if (normalized > SOUND_PI_MILLIRADIANS)
    {
        normalized -= SOUND_TWO_PI_MILLIRADIANS;
    }

    if (normalized > SOUND_HALF_PI_MILLIRADIANS)
    {
        normalized = SOUND_PI_MILLIRADIANS - normalized;
    }
    else if (normalized < -SOUND_HALF_PI_MILLIRADIANS)
    {
        normalized = -SOUND_PI_MILLIRADIANS - normalized;
    }

    x = ((int64_t)normalized * SOUND_TRIG_SCALE) / 1000;
    x2 = (x * x) / SOUND_TRIG_SCALE;
    result = x;
    result -= (((x * x2) / SOUND_TRIG_SCALE) / 6);
    result += (((((x * x2) / SOUND_TRIG_SCALE) * x2) / SOUND_TRIG_SCALE) / 120);
    result -= (((((((x * x2) / SOUND_TRIG_SCALE) * x2) / SOUND_TRIG_SCALE)
            * x2) / SOUND_TRIG_SCALE) / 5040);

    return sound_clamp_int((int)result, -SOUND_TRIG_SCALE, SOUND_TRIG_SCALE);
}

static int sound_triangle_q15(int phase)
{
    int normalized = sound_normalize_phase(phase);
    int value;

    if (normalized < SOUND_HALF_PI_MILLIRADIANS)
    {
        value = (normalized * SOUND_TRIG_SCALE) / SOUND_HALF_PI_MILLIRADIANS;
    }
    else if (normalized < SOUND_PI_MILLIRADIANS + SOUND_HALF_PI_MILLIRADIANS)
    {
        value = SOUND_TRIG_SCALE
                - ((normalized - SOUND_HALF_PI_MILLIRADIANS) * 2 * SOUND_TRIG_SCALE)
                / SOUND_PI_MILLIRADIANS;
    }
    else
    {
        value = -SOUND_TRIG_SCALE
                + ((normalized - SOUND_PI_MILLIRADIANS - SOUND_HALF_PI_MILLIRADIANS)
                * SOUND_TRIG_SCALE) / SOUND_HALF_PI_MILLIRADIANS;
    }

    return sound_clamp_int(value, -SOUND_TRIG_SCALE, SOUND_TRIG_SCALE);
}

static int sound_command_sample_count(const SoundCommand* command)
{
    int duration_ms;

    if (command == 0)
    {
        return 0;
    }

    duration_ms = sound_clamp_int(command->duration_ms, 0, SOUND_MAX_DURATION_MS);
    return (duration_ms * SOUND_GENERATOR_SAMPLE_RATE) / 1000;
}

static int sound_definition_sample_count(const SoundDefinition* definition)
{
    int sample_count = 0;

    if (definition == 0 || definition->command_count <= 0 || definition->commands == 0)
    {
        return 0;
    }

    for (int command_index = 0; command_index < definition->command_count; ++command_index)
    {
        sample_count += sound_command_sample_count(definition->commands + command_index);
    }

    return sample_count;
}

static int16_t sound_make_sample(
        SoundWaveKind wave,
        int phase,
        int volume,
        uint32_t* noise_state
)
{
    int amplitude;

    if (wave == SOUND_WAVE_SQUARE)
    {
        amplitude = sound_normalize_phase(phase) < SOUND_PI_MILLIRADIANS
                ? SOUND_TRIG_SCALE
                : -SOUND_TRIG_SCALE;
    }
    else if (wave == SOUND_WAVE_TRIANGLE)
    {
        amplitude = sound_triangle_q15(phase);
    }
    else if (wave == SOUND_WAVE_NOISE)
    {
        *noise_state = *noise_state * 1664525u + 1013904223u;
        amplitude = ((int)((*noise_state >> 16) & 0xffff) - 32768);
    }
    else
    {
        amplitude = sound_sin_q15(phase);
    }

    return (int16_t)((amplitude * volume) / 100);
}

static void sound_generate_command(
        const SoundCommand* command,
        int16_t* samples,
        int sample_offset,
        int sample_count,
        int* phase,
        uint32_t* noise_state
)
{
    int volume;
    int frequency_start;
    int frequency_end;

    if (command == 0 || samples == 0 || sample_count <= 0)
    {
        return;
    }

    volume = sound_clamp_int(command->volume, 0, 100);
    frequency_start = sound_clamp_int(command->frequency_start, 0, 20000);
    frequency_end = sound_clamp_int(command->frequency_end, 0, 20000);

    for (int sample_index = 0; sample_index < sample_count; ++sample_index)
    {
        int frequency;
        int phase_step;

        if (sample_count > 1)
        {
            frequency = frequency_start
                    + ((frequency_end - frequency_start) * sample_index)
                    / (sample_count - 1);
        }
        else
        {
            frequency = frequency_end;
        }

        samples[sample_offset + sample_index] = sound_make_sample(
                command->wave,
                *phase,
                volume,
                noise_state
        );

        phase_step = (frequency * SOUND_TWO_PI_MILLIRADIANS) / SOUND_GENERATOR_SAMPLE_RATE;
        *phase = sound_normalize_phase(*phase + phase_step);
    }
}

void sound_generator_release(GeneratedSound* sound)
{
    if (sound == 0)
    {
        return;
    }

    free(sound->samples);
    sound->id = 0;
    sound->sample_rate = 0;
    sound->sample_count = 0;
    sound->samples = 0;
}

int sound_generator_generate(const SoundDefinition* definition, GeneratedSound* sound)
{
    int total_samples;
    int sample_offset = 0;
    int phase = 0;
    uint32_t noise_state = 0x12345678u;
    int16_t* samples;

    if (definition == 0 || sound == 0 || definition->id == 0)
    {
        return 0;
    }

    total_samples = sound_definition_sample_count(definition);
    if (total_samples <= 0)
    {
        return 0;
    }

    sound_generator_release(sound);

    samples = (int16_t*)calloc((size_t)total_samples, sizeof(int16_t));
    if (samples == 0)
    {
        return 0;
    }

    for (int command_index = 0; command_index < definition->command_count; ++command_index)
    {
        int command_samples = sound_command_sample_count(definition->commands + command_index);
        sound_generate_command(
                definition->commands + command_index,
                samples,
                sample_offset,
                command_samples,
                &phase,
                &noise_state
        );
        sample_offset += command_samples;
    }

    sound->id = definition->id;
    sound->sample_rate = SOUND_GENERATOR_SAMPLE_RATE;
    sound->sample_count = total_samples;
    sound->samples = samples;

    return 1;
}
