#include "sound_registry.h"

#ifdef __ANDROID__
#include <android/log.h>
#include "../config.h"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) ((void)0)
#define LOGE(...) ((void)0)
#endif

#include "sound_generator.h"

static const SoundCommand JUMP_SOUND_COMMANDS[] = {
    {SOUND_WAVE_SQUARE, 420, 760, 90, 32},
    {SOUND_WAVE_TRIANGLE, 760, 520, 45, 22},
};

static const SoundCommand SMASH_SOUND_COMMANDS[] = {
    {SOUND_WAVE_NOISE, 180, 70, 130, 55},
    {SOUND_WAVE_SQUARE, 90, 55, 80, 30},
};

static const SoundCommand HIT_SOUND_COMMANDS[] = {
    {SOUND_WAVE_NOISE, 900, 260, 45, 45},
    {SOUND_WAVE_SQUARE, 240, 160, 50, 28},
};

static const SoundCommand DEATH_SOUND_COMMANDS[] = {
    {SOUND_WAVE_SQUARE, 360, 120, 180, 38},
    {SOUND_WAVE_TRIANGLE, 120, 55, 220, 30},
};

const SoundDefinition JUMP_SOUND = {
    "jump",
    JUMP_SOUND_COMMANDS,
    (int16_t)(sizeof(JUMP_SOUND_COMMANDS) / sizeof(JUMP_SOUND_COMMANDS[0]))};

const SoundDefinition SMASH_SOUND = {
    "smash",
    SMASH_SOUND_COMMANDS,
    (int16_t)(sizeof(SMASH_SOUND_COMMANDS) / sizeof(SMASH_SOUND_COMMANDS[0]))};

const SoundDefinition HIT_SOUND = {
    "hit",
    HIT_SOUND_COMMANDS,
    (int16_t)(sizeof(HIT_SOUND_COMMANDS) / sizeof(HIT_SOUND_COMMANDS[0]))};

const SoundDefinition DEATH_SOUND = {
    "death",
    DEATH_SOUND_COMMANDS,
    (int16_t)(sizeof(DEATH_SOUND_COMMANDS) / sizeof(DEATH_SOUND_COMMANDS[0]))};

const SoundDefinition *SOUND_DEFINITIONS[] = {
    &JUMP_SOUND,
    &SMASH_SOUND,
    &HIT_SOUND,
    &DEATH_SOUND,
};

const size_t SOUND_COUNT =
    sizeof(SOUND_DEFINITIONS) / sizeof(SOUND_DEFINITIONS[0]);

static GeneratedSound GENERATED_SOUNDS[SOUND_ID_COUNT];
static int sound_registry_initialized = 0;

static int sound_id_equals(const char *left, const char *right)
{
    int index = 0;

    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (left[index] != 0 && right[index] != 0)
    {
        if (left[index] != right[index])
        {
            return 0;
        }
        index += 1;
    }

    return left[index] == right[index];
}

void sound_registry_initialize_all(void)
{
    if (sound_registry_initialized)
    {
        return;
    }

    for (size_t sound_index = 0; sound_index < SOUND_COUNT && sound_index < SOUND_ID_COUNT; ++sound_index)
    {
        const SoundDefinition *definition = SOUND_DEFINITIONS[sound_index];
        GeneratedSound *sound = GENERATED_SOUNDS + sound_index;

        if (definition == 0)
        {
            LOGE("Generated sound %zu invalid: definition is null", sound_index);
            continue;
        }

        LOGI(
            "Generated sound %zu: id=%s command_count=%d",
            sound_index,
            definition->id != 0 ? definition->id : "(null)",
            definition->command_count);

        if (!sound_generator_generate(definition, sound))
        {
            LOGE(
                "Generated sound %zu (%s) failed",
                sound_index,
                definition->id != 0 ? definition->id : "(null)");
        }
    }

    sound_registry_initialized = 1;
}

void sound_registry_shutdown_all(void)
{
    for (size_t sound_index = 0; sound_index < SOUND_ID_COUNT; ++sound_index)
    {
        sound_generator_release(GENERATED_SOUNDS + sound_index);
    }

    sound_registry_initialized = 0;
}

const GeneratedSound *sound_registry_get(SoundId sound_id)
{
    const GeneratedSound *sound;

    if (sound_id < 0 || sound_id >= SOUND_ID_COUNT)
    {
        return 0;
    }

    sound = GENERATED_SOUNDS + sound_id;
    if (sound->samples == 0)
    {
        return 0;
    }

    return sound;
}

const GeneratedSound *sound_registry_get_by_id(const char *sound_id)
{
    for (size_t sound_index = 0; sound_index < SOUND_COUNT && sound_index < SOUND_ID_COUNT; ++sound_index)
    {
        if (sound_id_equals(GENERATED_SOUNDS[sound_index].id, sound_id))
        {
            return GENERATED_SOUNDS + sound_index;
        }
    }

    return 0;
}
