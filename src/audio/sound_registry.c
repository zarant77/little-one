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

static GeneratedSound GENERATED_SOUND_CACHE[SOUND_ID_COUNT];
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

    for (size_t sound_index = 0; sound_index < PACKED_SOUND_COUNT && sound_index < SOUND_ID_COUNT; ++sound_index)
    {
        const SoundDefinition *definition = PACKED_SOUND_DEFINITIONS + sound_index;
        GeneratedSound *sound = GENERATED_SOUND_CACHE + sound_index;

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
        sound_generator_release(GENERATED_SOUND_CACHE + sound_index);
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

    sound = GENERATED_SOUND_CACHE + sound_id;
    if (sound->samples == 0)
    {
        return 0;
    }

    return sound;
}

const GeneratedSound *sound_registry_get_by_id(const char *sound_id)
{
    for (size_t sound_index = 0; sound_index < PACKED_SOUND_COUNT && sound_index < SOUND_ID_COUNT; ++sound_index)
    {
        if (sound_id_equals(GENERATED_SOUND_CACHE[sound_index].id, sound_id))
        {
            return GENERATED_SOUND_CACHE + sound_index;
        }
    }

    return 0;
}
