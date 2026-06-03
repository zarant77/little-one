#include "music_registry.h"

#ifdef __ANDROID__
#include <android/log.h>
#include "../config.h"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) ((void)0)
#define LOGE(...) ((void)0)
#endif

#include "music_generator.h"

static GeneratedMusic GENERATED_MUSIC_CACHE[MUSIC_ID_COUNT];
static int music_registry_initialized = 0;

static int music_id_equals(const char *left, const char *right)
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

void music_registry_initialize_all(void)
{
    if (music_registry_initialized)
    {
        return;
    }

    for (size_t music_index = 0; music_index < PACKED_MUSIC_COUNT && music_index < MUSIC_ID_COUNT; ++music_index)
    {
        const MusicDefinition *definition = PACKED_MUSIC_DEFINITIONS + music_index;
        GeneratedMusic *music = GENERATED_MUSIC_CACHE + music_index;

        LOGI(
            "Generated music %zu: id=%s note_count=%d",
            music_index,
            definition->id != 0 ? definition->id : "(null)",
            definition->note_count);

        if (!music_generator_generate(definition, music))
        {
            LOGE(
                "Generated music %zu (%s) failed",
                music_index,
                definition->id != 0 ? definition->id : "(null)");
        }
    }

    music_registry_initialized = 1;
}

void music_registry_shutdown_all(void)
{
    for (size_t music_index = 0; music_index < MUSIC_ID_COUNT; ++music_index)
    {
        music_generator_release(GENERATED_MUSIC_CACHE + music_index);
    }

    music_registry_initialized = 0;
}

const GeneratedMusic *music_registry_get(MusicId music_id)
{
    const GeneratedMusic *music;

    if (music_id < 0 || music_id >= MUSIC_ID_COUNT)
    {
        return 0;
    }

    music = GENERATED_MUSIC_CACHE + music_id;
    if (music->samples == 0)
    {
        return 0;
    }

    return music;
}

const GeneratedMusic *music_registry_get_by_id(const char *music_id)
{
    for (size_t music_index = 0; music_index < PACKED_MUSIC_COUNT && music_index < MUSIC_ID_COUNT; ++music_index)
    {
        if (music_id_equals(GENERATED_MUSIC_CACHE[music_index].id, music_id))
        {
            return GENERATED_MUSIC_CACHE + music_index;
        }
    }

    return 0;
}
