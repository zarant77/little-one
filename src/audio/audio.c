#include "audio.h"

#ifdef __ANDROID__
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#include <stddef.h>
#include <stdint.h>

#include "../config.h"
#include "music_registry.h"
#include "sound_registry.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)

static SLObjectItf audio_engine_object = 0;
static SLEngineItf audio_engine = 0;
static SLObjectItf audio_output_mix_object = 0;
static SLObjectItf audio_player_object = 0;
static SLPlayItf audio_player = 0;
static SLAndroidSimpleBufferQueueItf audio_queue = 0;
static int audio_initialized = 0;
static const GeneratedMusic* audio_music = 0;
static int audio_music_cursor = 0;
static int audio_music_rate_permille = 1000;
static int audio_music_rate_accumulator = 0;
static const GeneratedSound* audio_sound = 0;
static int audio_sound_cursor = 0;
static int audio_next_mix_buffer = 0;
static int audio_music_volume = 70;
static int audio_sfx_volume = 70;

enum {
    AUDIO_MIX_BUFFER_SAMPLES = 512,
    AUDIO_MIX_BUFFER_COUNT = 2
};

static int16_t audio_mix_buffers[AUDIO_MIX_BUFFER_COUNT][AUDIO_MIX_BUFFER_SAMPLES];

static int audio_check_result(SLresult result, const char* step)
{
    if (result == SL_RESULT_SUCCESS)
    {
        return 1;
    }

    LOGE("Audio init failed at %s: result=%u", step, (unsigned int)result);
    return 0;
}

static void audio_destroy_object(SLObjectItf* object)
{
    if (object != 0 && *object != 0)
    {
        (*(*object))->Destroy(*object);
        *object = 0;
    }
}

static int16_t audio_clamp_sample(int value)
{
    if (value < -32768)
    {
        return -32768;
    }
    if (value > 32767)
    {
        return 32767;
    }
    return (int16_t)value;
}

static int audio_clamp_volume(int volume)
{
    if (volume < 0)
    {
        return 0;
    }

    if (volume > 100)
    {
        return 100;
    }

    return volume;
}

static void audio_advance_music_cursor(void)
{
    int advance;

    if (audio_music == 0 || audio_music->sample_count <= 0)
    {
        return;
    }

    audio_music_rate_accumulator += audio_music_rate_permille;
    advance = audio_music_rate_accumulator / 1000;
    audio_music_rate_accumulator %= 1000;
    audio_music_cursor += advance;

    if (audio_music->loop_enabled
            && audio_music->loop_end_sample > audio_music->loop_start_sample)
    {
        int loop_length = audio_music->loop_end_sample - audio_music->loop_start_sample;

        while (audio_music_cursor >= audio_music->loop_end_sample)
        {
            audio_music_cursor = audio_music->loop_start_sample
                    + (audio_music_cursor - audio_music->loop_end_sample) % loop_length;
        }
    }
    else
    {
        while (audio_music_cursor >= audio_music->sample_count)
        {
            audio_music_cursor -= audio_music->sample_count;
        }
    }
}

static void audio_fill_mix_buffer(int buffer_index)
{
    int16_t* buffer;

    if (buffer_index < 0 || buffer_index >= AUDIO_MIX_BUFFER_COUNT)
    {
        return;
    }

    buffer = audio_mix_buffers[buffer_index];
    for (int sample_index = 0; sample_index < AUDIO_MIX_BUFFER_SAMPLES; ++sample_index)
    {
        int mixed = 0;

        if (audio_music != 0
                && audio_music->samples != 0
                && audio_music->sample_count > 0)
        {
            int track_volume = audio_clamp_volume(audio_music->volume);
            mixed += (audio_music->samples[audio_music_cursor]
                    * audio_music_volume
                    * track_volume) / 10000;
            audio_advance_music_cursor();
        }

        if (audio_sound != 0
                && audio_sound->samples != 0
                && audio_sound->sample_count > 0)
        {
            mixed += (audio_sound->samples[audio_sound_cursor] * audio_sfx_volume) / 100;
            audio_sound_cursor += 1;
            if (audio_sound_cursor >= audio_sound->sample_count)
            {
                audio_sound = 0;
                audio_sound_cursor = 0;
            }
        }

        buffer[sample_index] = audio_clamp_sample(mixed);
    }
}

static void audio_enqueue_mix_buffer(int buffer_index)
{
    if (audio_queue == 0)
    {
        return;
    }

    audio_fill_mix_buffer(buffer_index);
    (*audio_queue)->Enqueue(
            audio_queue,
            audio_mix_buffers[buffer_index],
            (SLuint32)(AUDIO_MIX_BUFFER_SAMPLES * sizeof(int16_t))
    );
}

static void audio_buffer_queue_callback(
        SLAndroidSimpleBufferQueueItf queue,
        void* context
)
{
    (void)queue;
    (void)context;

    audio_enqueue_mix_buffer(audio_next_mix_buffer);
    audio_next_mix_buffer = (audio_next_mix_buffer + 1) % AUDIO_MIX_BUFFER_COUNT;
}

void audio_shutdown(void)
{
    if (audio_player != 0)
    {
        (*audio_player)->SetPlayState(audio_player, SL_PLAYSTATE_STOPPED);
    }
    if (audio_queue != 0)
    {
        (*audio_queue)->Clear(audio_queue);
    }

    audio_music = 0;
    audio_music_cursor = 0;
    audio_music_rate_permille = 1000;
    audio_music_rate_accumulator = 0;
    audio_sound = 0;
    audio_sound_cursor = 0;
    audio_next_mix_buffer = 0;
    audio_queue = 0;
    audio_player = 0;
    audio_engine = 0;
    audio_destroy_object(&audio_player_object);
    audio_destroy_object(&audio_output_mix_object);
    audio_destroy_object(&audio_engine_object);
    music_registry_shutdown_all();
    sound_registry_shutdown_all();
    audio_initialized = 0;
}

void audio_init(void)
{
    SLDataLocator_AndroidSimpleBufferQueue queue_locator;
    SLDataFormat_PCM pcm_format;
    SLDataSource audio_source;
    SLDataLocator_OutputMix output_locator;
    SLDataSink audio_sink;
    const SLInterfaceID player_interfaces[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    const SLboolean player_required[] = { SL_BOOLEAN_TRUE };
    SLresult result;

    if (audio_initialized)
    {
        return;
    }

    sound_registry_initialize_all();
    music_registry_initialize_all();

    result = slCreateEngine(&audio_engine_object, 0, 0, 0, 0, 0);
    if (!audio_check_result(result, "create engine"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_engine_object)->Realize(audio_engine_object, SL_BOOLEAN_FALSE);
    if (!audio_check_result(result, "realize engine"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_engine_object)->GetInterface(
            audio_engine_object,
            SL_IID_ENGINE,
            &audio_engine
    );
    if (!audio_check_result(result, "get engine interface"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_engine)->CreateOutputMix(
            audio_engine,
            &audio_output_mix_object,
            0,
            0,
            0
    );
    if (!audio_check_result(result, "create output mix"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_output_mix_object)->Realize(audio_output_mix_object, SL_BOOLEAN_FALSE);
    if (!audio_check_result(result, "realize output mix"))
    {
        audio_shutdown();
        return;
    }

    queue_locator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    queue_locator.numBuffers = AUDIO_MIX_BUFFER_COUNT;

    pcm_format.formatType = SL_DATAFORMAT_PCM;
    pcm_format.numChannels = 1;
    pcm_format.samplesPerSec = SL_SAMPLINGRATE_22_05;
    pcm_format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm_format.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm_format.channelMask = SL_SPEAKER_FRONT_CENTER;
    pcm_format.endianness = SL_BYTEORDER_LITTLEENDIAN;

    audio_source.pLocator = &queue_locator;
    audio_source.pFormat = &pcm_format;

    output_locator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    output_locator.outputMix = audio_output_mix_object;
    audio_sink.pLocator = &output_locator;
    audio_sink.pFormat = 0;

    result = (*audio_engine)->CreateAudioPlayer(
            audio_engine,
            &audio_player_object,
            &audio_source,
            &audio_sink,
            1,
            player_interfaces,
            player_required
    );
    if (!audio_check_result(result, "create audio player"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_player_object)->Realize(audio_player_object, SL_BOOLEAN_FALSE);
    if (!audio_check_result(result, "realize audio player"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_player_object)->GetInterface(
            audio_player_object,
            SL_IID_PLAY,
            &audio_player
    );
    if (!audio_check_result(result, "get play interface"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_player_object)->GetInterface(
            audio_player_object,
            SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
            &audio_queue
    );
    if (!audio_check_result(result, "get buffer queue"))
    {
        audio_shutdown();
        return;
    }

    result = (*audio_queue)->RegisterCallback(
            audio_queue,
            audio_buffer_queue_callback,
            0
    );
    if (!audio_check_result(result, "register buffer queue callback"))
    {
        audio_shutdown();
        return;
    }

    audio_next_mix_buffer = 0;
    audio_enqueue_mix_buffer(0);
    audio_enqueue_mix_buffer(1);

    result = (*audio_player)->SetPlayState(audio_player, SL_PLAYSTATE_PLAYING);
    if (!audio_check_result(result, "start player"))
    {
        audio_shutdown();
        return;
    }

    audio_initialized = 1;
    LOGI("Audio init success");
}

void audio_pause(void)
{
    if (audio_player != 0)
    {
        (*audio_player)->SetPlayState(audio_player, SL_PLAYSTATE_PAUSED);
    }
}

void audio_resume(void)
{
    if (audio_player != 0)
    {
        (*audio_player)->SetPlayState(audio_player, SL_PLAYSTATE_PLAYING);
    }
}

void audio_set_music_volume(int volume)
{
    audio_music_volume = audio_clamp_volume(volume);
}

void audio_set_music_speed(float multiplier)
{
    int permille = (int)(multiplier * 1000.0f + 0.5f);

    if (permille < 250)
    {
        permille = 250;
    }
    else if (permille > 4000)
    {
        permille = 4000;
    }
    audio_music_rate_permille = permille;
}

void audio_set_sfx_volume(int volume)
{
    audio_sfx_volume = audio_clamp_volume(volume);
}

void audio_play_sound(const char* id)
{
    const GeneratedSound* sound;

    if (!audio_initialized)
    {
        LOGI("Audio playback requested before init: id=%s", id != 0 ? id : "(null)");
        return;
    }

    sound = sound_registry_get_by_id(id);
    if (sound == 0 || sound->samples == 0 || sound->sample_count <= 0)
    {
        LOGI("Sound id not found: %s", id != 0 ? id : "(null)");
        return;
    }

    audio_sound = sound;
    audio_sound_cursor = 0;
    LOGI("Playback start: %s", sound->id);
}

void audio_play_music(const char* id)
{
    const GeneratedMusic* music;

    if (!audio_initialized)
    {
        LOGI("Music playback requested before init: id=%s", id != 0 ? id : "(null)");
        return;
    }

    music = music_registry_get_by_id(id);
    if (music == 0 || music->samples == 0 || music->sample_count <= 0)
    {
        LOGI("Music id not found: %s", id != 0 ? id : "(null)");
        return;
    }

    if (audio_music == music)
    {
        return;
    }

    audio_music = music;
    audio_music_cursor = 0;
    audio_music_rate_accumulator = 0;
    LOGI("Music playback start: %s", music->id);
}

void audio_stop_music(void)
{
    audio_music = 0;
    audio_music_cursor = 0;
    audio_music_rate_accumulator = 0;
    LOGI("Music playback stop");
}

#else
#include "music_registry.h"
#include "sound_registry.h"

void audio_init(void)
{
    sound_registry_initialize_all();
    music_registry_initialize_all();
}

void audio_shutdown(void)
{
    music_registry_shutdown_all();
    sound_registry_shutdown_all();
}

void audio_pause(void)
{
}

void audio_resume(void)
{
}

void audio_set_music_volume(int volume)
{
    (void)volume;
}

void audio_set_music_speed(float multiplier)
{
    (void)multiplier;
}

void audio_set_sfx_volume(int volume)
{
    (void)volume;
}

void audio_play_sound(const char* id)
{
    (void)sound_registry_get_by_id(id);
}

void audio_play_music(const char* id)
{
    (void)music_registry_get_by_id(id);
}

void audio_stop_music(void)
{
}
#endif
