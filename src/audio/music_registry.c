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

static const char* music_registry_id_name(MusicId music_id)
{
    switch (music_id)
    {
        case MUSIC_ID_GAME_LOOP: return "game_loop";
        case MUSIC_ID_GAME_OVER: return "game_over";
        default: return 0;
    }
}

static int music_registry_id_equals(const char *left, const char *right)
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

static const PackedMusicDefinition* music_registry_find_definition(MusicId music_id)
{
    if (music_id < 0 || music_id >= MUSIC_ID_COUNT)
    {
        return 0;
    }

    for (size_t music_index = 0; music_index < PACKED_MUSIC_COUNT; ++music_index)
    {
        const PackedMusicDefinition* definition = PACKED_MUSIC_DEFINITIONS + music_index;
        if ((MusicId)definition->id == music_id)
        {
            return definition;
        }
    }

    return 0;
}

static const GeneratedMusic* music_registry_generate(MusicId music_id)
{
    const PackedMusicDefinition* definition;
    GeneratedMusic* music;

    if (music_id < 0 || music_id >= MUSIC_ID_COUNT)
    {
        return 0;
    }

    music = GENERATED_MUSIC_CACHE + music_id;
    if (music->samples != 0 && music->sample_count > 0)
    {
        return music;
    }

    definition = music_registry_find_definition(music_id);
    if (definition == 0)
    {
        LOGE("Generated music definition missing: id=%d", (int)music_id);
        return 0;
    }

    LOGI(
        "Generating music: id=%s note_count=%u",
        music_registry_id_name(music_id) != 0 ? music_registry_id_name(music_id) : "(null)",
        (unsigned int)definition->note_count);

    if (!music_generator_generate_from_packed(definition, music))
    {
        LOGE(
            "Generated music failed: id=%s",
            music_registry_id_name(music_id) != 0 ? music_registry_id_name(music_id) : "(null)");
        return 0;
    }

    music->id = music_registry_id_name(music_id);
    return music;
}

void music_registry_initialize_all(void)
{
    if (music_registry_initialized)
    {
        return;
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
    if (music_id < 0 || music_id >= MUSIC_ID_COUNT)
    {
        return 0;
    }

    return music_registry_generate(music_id);
}

const GeneratedMusic *music_registry_get_by_id(const char *music_id)
{
    for (size_t music_index = 0; music_index < MUSIC_ID_COUNT; ++music_index)
    {
        const char* candidate_id = music_registry_id_name((MusicId)music_index);
        if (music_registry_id_equals(candidate_id, music_id))
        {
            return music_registry_generate((MusicId)music_index);
        }
    }

    return 0;
}
