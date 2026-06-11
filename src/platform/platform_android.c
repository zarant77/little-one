#include "platform_android.h"

#include <android/input.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../audio/audio.h"
#include "../audio/music_registry.h"
#include "../audio/sound_registry.h"
#include "../config.h"
#include "../game/game.h"
#include "../game/game_settings.h"
#include "../input/input.h"
#include "../renderer/renderer.h"
#include "../sprites/generated_sprite.h"
#include "../ui/hud.h"
#include "../ui/menu.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)

typedef struct AndroidPlatform {
    ANativeActivity* activity;
    ANativeWindow* window;
    pthread_mutex_t window_mutex;
    AInputQueue* input_queue;
    AInputQueue* attached_input_queue;
    pthread_mutex_t input_queue_mutex;
    int input_cancel_requested;
    int pause_requested;
    pthread_t thread;
    volatile int loop_running;
    GameState game;
    InputState input;
    double fps_elapsed;
    double fps_frame_time_total;
    int fps_frame_count;
    int null_window_logged;
    int reset_frame_time;
    int buffer_format_logged;
    char progress_path[512];
    char settings_path[512];
    int finish_requested;
} AndroidPlatform;

static AndroidPlatform* platform_from_activity(ANativeActivity* activity) {
    return (AndroidPlatform*)activity->instance;
}

static double platform_now_seconds(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);

    return (double)time.tv_sec + ((double)time.tv_nsec / 1000000000.0);
}

static void platform_sleep_seconds(double seconds) {
    struct timespec sleep_time;

    if (seconds <= 0.0) {
        return;
    }

    sleep_time.tv_sec = (time_t)seconds;
    sleep_time.tv_nsec = (long)((seconds - (double)sleep_time.tv_sec) * 1000000000.0);

    nanosleep(&sleep_time, NULL);
}

static void platform_set_progress_path(AndroidPlatform* platform, ANativeActivity* activity) {
    const char* data_path;

    if (platform == NULL || activity == NULL) {
        return;
    }

    data_path = activity->internalDataPath;
    if (data_path == NULL || data_path[0] == 0) {
        platform->progress_path[0] = 0;
        return;
    }

    snprintf(
            platform->progress_path,
            sizeof(platform->progress_path),
            "%s/little_one_progress.txt",
            data_path
    );
}

static void platform_set_settings_path(AndroidPlatform* platform, ANativeActivity* activity) {
    const char* data_path;

    if (platform == NULL || activity == NULL) {
        return;
    }

    data_path = activity->internalDataPath;
    if (data_path == NULL || data_path[0] == 0) {
        platform->settings_path[0] = 0;
        return;
    }

    snprintf(
            platform->settings_path,
            sizeof(platform->settings_path),
            "%s/little_one_settings.txt",
            data_path
    );
}

static void platform_load_progress(AndroidPlatform* platform) {
    ProgressionState progress;

    if (platform == NULL || platform->progress_path[0] == 0) {
        return;
    }

    if (progression_load_from_path(platform->progress_path, &progress)) {
        platform->game.progress = progress;
        platform->game.progressInitialized = 1;
        platform->game.progressDirty = 0;
    }
}

static void platform_save_progress(AndroidPlatform* platform) {
    if (platform == NULL
            || platform->progress_path[0] == 0
            || !platform->game.progressDirty) {
        return;
    }

    if (progression_save_to_path(platform->progress_path, &platform->game.progress)) {
        platform->game.progressDirty = 0;
    }
}

static void platform_load_settings(AndroidPlatform* platform) {
    GameSettings settings;

    if (platform == NULL || platform->settings_path[0] == 0) {
        return;
    }

    if (game_settings_load_from_path(platform->settings_path, &settings)) {
        platform->game.settings = settings;
        platform->game.settingsInitialized = 1;
        platform->game.settingsDirty = 0;
    }
}

static void platform_save_settings(AndroidPlatform* platform) {
    if (platform == NULL
            || platform->settings_path[0] == 0
            || !platform->game.settingsDirty) {
        return;
    }

    if (game_settings_save_to_path(platform->settings_path, &platform->game.settings)) {
        platform->game.settingsDirty = 0;
    }
}

static GameLocale platform_detect_locale(ANativeActivity* activity) {
    JNIEnv* env = NULL;
    JavaVM* vm;
    jclass locale_class;
    jmethodID get_default_method;
    jmethodID get_language_method;
    jobject locale;
    jstring language;
    const char* language_chars;
    GameLocale locale_value = GAME_LOCALE_ENGLISH;
    int attached = 0;

    if (activity == NULL || activity->vm == NULL) {
        return GAME_LOCALE_ENGLISH;
    }

    vm = activity->vm;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*vm)->AttachCurrentThread(vm, &env, NULL) != JNI_OK) {
            return GAME_LOCALE_ENGLISH;
        }
        attached = 1;
    }

    locale_class = (*env)->FindClass(env, "java/util/Locale");
    if (locale_class == NULL) {
        goto done;
    }

    get_default_method = (*env)->GetStaticMethodID(
            env,
            locale_class,
            "getDefault",
            "()Ljava/util/Locale;"
    );
    get_language_method = (*env)->GetMethodID(
            env,
            locale_class,
            "getLanguage",
            "()Ljava/lang/String;"
    );
    if (get_default_method == NULL || get_language_method == NULL) {
        goto done;
    }

    locale = (*env)->CallStaticObjectMethod(env, locale_class, get_default_method);
    if (locale == NULL) {
        goto done;
    }

    language = (jstring)(*env)->CallObjectMethod(env, locale, get_language_method);
    if (language == NULL) {
        goto done;
    }

    language_chars = (*env)->GetStringUTFChars(env, language, NULL);
    if (language_chars != NULL) {
        if (strncmp(language_chars, "uk", 2) == 0) {
            locale_value = GAME_LOCALE_UKRAINIAN;
        }
        (*env)->ReleaseStringUTFChars(env, language, language_chars);
    }

done:
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        locale_value = GAME_LOCALE_ENGLISH;
    }
    if (attached) {
        (*vm)->DetachCurrentThread(vm);
    }

    return locale_value;
}

static int platform_motion_action_index(int action) {
    return (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
           >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
}

static void platform_handle_motion_event(
        AndroidPlatform* platform,
        AInputEvent* event,
        float screen_width
) {
    int action = AMotionEvent_getAction(event);
    int action_type = action & AMOTION_EVENT_ACTION_MASK;
    int action_index = platform_motion_action_index(action);
    size_t pointer_count = AMotionEvent_getPointerCount(event);
    int log_index = action_index;

    #if LITTLE_ONE_DEBUG_INPUT
    if (pointer_count > 0) {
        if (log_index < 0 || (size_t)log_index >= pointer_count) {
            log_index = 0;
        }

        int pointer_id = AMotionEvent_getPointerId(event, log_index);
        float x = AMotionEvent_getX(event, log_index);
        float y = AMotionEvent_getY(event, log_index);

        LOGI(
                "Motion event action=%d pointerCount=%zu pointerId=%d x=%.1f y=%.1f",
                action_type,
                pointer_count,
                pointer_id,
                x,
                y
        );
    } else {
        LOGI("Motion event action=%d pointerCount=0", action_type);
    }
    #else
    (void)log_index;
    #endif

    if (action_type == AMOTION_EVENT_ACTION_DOWN
            || action_type == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        if (action_index < 0 || (size_t)action_index >= pointer_count) {
            #if LITTLE_ONE_DEBUG_INPUT
            LOGI("Motion event ignored because action index is invalid");
            #endif
            return;
        }

        int pointer_id = AMotionEvent_getPointerId(event, action_index);
        float x = AMotionEvent_getX(event, action_index);
        float y = AMotionEvent_getY(event, action_index);

        if (!menu_handle_touch(&platform->game, INPUT_TOUCH_DOWN, pointer_id, (int)x, (int)y)) {
            input_handle_touch(
                    &platform->input,
                    INPUT_TOUCH_DOWN,
                    pointer_id,
                    x,
                    y,
                    screen_width
            );
        }
        return;
    }

    if (action_type == AMOTION_EVENT_ACTION_MOVE) {
        size_t pointer_count = AMotionEvent_getPointerCount(event);

        for (size_t pointer_index = 0; pointer_index < pointer_count; ++pointer_index) {
            int pointer_id = AMotionEvent_getPointerId(event, pointer_index);
            float x = AMotionEvent_getX(event, pointer_index);
            float y = AMotionEvent_getY(event, pointer_index);

            if (!menu_handle_touch(&platform->game, INPUT_TOUCH_MOVE, pointer_id, (int)x, (int)y)) {
                input_handle_touch(
                        &platform->input,
                        INPUT_TOUCH_MOVE,
                        pointer_id,
                        x,
                        y,
                        screen_width
                );
            }
        }

        return;
    }

    if (action_type == AMOTION_EVENT_ACTION_UP
            || action_type == AMOTION_EVENT_ACTION_POINTER_UP) {
        if (action_index < 0 || (size_t)action_index >= pointer_count) {
            #if LITTLE_ONE_DEBUG_INPUT
            LOGI("Motion event ignored because action index is invalid");
            #endif
            return;
        }

        int pointer_id = AMotionEvent_getPointerId(event, action_index);

        if (!menu_handle_touch(&platform->game, INPUT_TOUCH_UP, pointer_id, 0, 0)) {
            input_handle_touch(
                    &platform->input,
                    INPUT_TOUCH_UP,
                    pointer_id,
                    0.0f,
                    0.0f,
                    screen_width
            );
        }
        return;
    }

    if (action_type == AMOTION_EVENT_ACTION_CANCEL) {
        menu_handle_touch(&platform->game, INPUT_TOUCH_CANCEL, -1, 0, 0);
        input_handle_touch(
                &platform->input,
                INPUT_TOUCH_CANCEL,
                -1,
                0.0f,
                0.0f,
                screen_width
        );
    }
}

static int platform_handle_key_event(AndroidPlatform* platform, AInputEvent* event) {
    if (AKeyEvent_getKeyCode(event) != AKEYCODE_BACK) {
        return 0;
    }

    if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN) {
        menu_pause(&platform->game);
        input_init(&platform->input);
    }

    return 1;
}

static void platform_process_input(AndroidPlatform* platform, float screen_width) {
    AInputQueue* queue;
    AInputEvent* event = NULL;

    pthread_mutex_lock(&platform->input_queue_mutex);
    queue = platform->input_queue;

    if (platform->pause_requested) {
        menu_pause(&platform->game);
        input_init(&platform->input);
        platform->pause_requested = 0;
    }

    if (platform->input_cancel_requested) {
        menu_handle_touch(&platform->game, INPUT_TOUCH_CANCEL, -1, 0, 0);
        input_handle_touch(&platform->input, INPUT_TOUCH_CANCEL, -1, 0.0f, 0.0f, screen_width);
        platform->input_cancel_requested = 0;
    }

    if (platform->attached_input_queue != queue) {
        if (platform->attached_input_queue != NULL) {
            AInputQueue_detachLooper(platform->attached_input_queue);
            platform->attached_input_queue = NULL;
        }

        if (queue != NULL) {
            ALooper* looper = ALooper_forThread();
            if (looper != NULL) {
                AInputQueue_attachLooper(queue, looper, 1, NULL, NULL);
                platform->attached_input_queue = queue;
            }
        }
    }

    if (queue == NULL) {
        pthread_mutex_unlock(&platform->input_queue_mutex);
        return;
    }

    while (AInputQueue_getEvent(queue, &event) >= 0) {
        int handled = 0;

        if (AInputQueue_preDispatchEvent(queue, event)) {
            AInputQueue_finishEvent(queue, event, 0);
            #if LITTLE_ONE_DEBUG_INPUT
            LOGI("Input event pre-dispatched and finished handled=0");
            #endif
            continue;
        }

        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            platform_handle_motion_event(platform, event, screen_width);
            handled = 1;
        } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
            handled = platform_handle_key_event(platform, event);
        }

        AInputQueue_finishEvent(queue, event, handled);
        #if LITTLE_ONE_DEBUG_INPUT
        LOGI("Input event finished handled=%d", handled);
        #endif
    }

    pthread_mutex_unlock(&platform->input_queue_mutex);
}

static ANativeWindow* platform_acquire_window(AndroidPlatform* platform) {
    ANativeWindow* window;
    int should_log_null_window = 0;

    pthread_mutex_lock(&platform->window_mutex);
    window = platform->window;
    if (window == NULL) {
        if (!platform->null_window_logged) {
            platform->null_window_logged = 1;
            should_log_null_window = 1;
        }
    } else {
        ANativeWindow_acquire(window);
        platform->null_window_logged = 0;
    }
    pthread_mutex_unlock(&platform->window_mutex);

    if (should_log_null_window) {
        LOGI("Render skipped because window is null");
    }

    return window;
}

static int platform_draw(AndroidPlatform* platform, float dt) {
    ANativeWindow* window;

    if (platform == NULL) {
        return 0;
    }

    window = platform_acquire_window(platform);
    if (window == NULL) {
        return 0;
    }

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, NULL) != 0) {
        ANativeWindow_release(window);
        LOGE("Failed to lock native window");
        return 0;
    }

    if (!platform->buffer_format_logged) {
        LOGI(
                "Window buffer locked format=%d width=%d height=%d stride=%d",
                buffer.format,
                buffer.width,
                buffer.height,
                buffer.stride
        );
        platform->buffer_format_logged = 1;
    }

    game_set_screen_size(&platform->game, (float)buffer.width, (float)buffer.height);
    if (platform->game.uiState != GAME_UI_PLAYING) {
        input_init(&platform->input);
    }
    game_update(&platform->game, &platform->input, dt);
    platform_save_progress(platform);
    platform_save_settings(platform);
    input_end_frame(&platform->input);
    renderer_draw_frame(&buffer, &platform->game);
    ANativeWindow_unlockAndPost(window);
    ANativeWindow_release(window);

    return 1;
}

static int platform_take_reset_frame_time(AndroidPlatform* platform) {
    int reset_frame_time;

    pthread_mutex_lock(&platform->window_mutex);
    reset_frame_time = platform->reset_frame_time;
    platform->reset_frame_time = 0;
    pthread_mutex_unlock(&platform->window_mutex);

    return reset_frame_time;
}

static void platform_update_fps(AndroidPlatform* platform, float frame_time) {
    if (platform == NULL) {
        return;
    }

    platform->fps_elapsed += frame_time;
    platform->fps_frame_time_total += frame_time;
    platform->fps_frame_count += 1;

    if (platform->fps_elapsed < 1.0) {
        return;
    }

    platform->game.fps = (int)((double)platform->fps_frame_count / platform->fps_elapsed + 0.5);
    platform->game.averageFrameMs = (int)((platform->fps_frame_time_total * 1000.0)
            / (double)platform->fps_frame_count);

    #if LITTLE_ONE_LOG_FPS
    LOGI(
            "fps=%d avgFrameMs=%d entities=%d",
            platform->game.fps,
            platform->game.averageFrameMs,
            platform->game.activeEntityCount
    );
    #endif

    platform->fps_elapsed = 0.0;
    platform->fps_frame_time_total = 0.0;
    platform->fps_frame_count = 0;
}

static void platform_handle_exit_requested(AndroidPlatform* platform) {
    if (platform == NULL
            || !platform->game.exitRequested
            || platform->finish_requested) {
        return;
    }

    platform->finish_requested = 1;
    platform_save_progress(platform);
    platform_save_settings(platform);
    if (platform->activity != NULL) {
        ANativeActivity_finish(platform->activity);
    }
}

static void* platform_game_loop(void* data) {
    AndroidPlatform* platform = (AndroidPlatform*)data;
    #if LITTLE_ONE_UNCAPPED_FPS
    const double target_frame_seconds = 0.0;
    #else
    const double target_frame_seconds = 1.0 / (double)LITTLE_ONE_TARGET_FPS;
    #endif
    const double reset_frame_seconds = 1.0 / (double)LITTLE_ONE_TARGET_FPS;
    double last_time = platform_now_seconds();

    ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    LOGI("Game loop start");

    while (platform->loop_running) {
        double frame_start = platform_now_seconds();
        float dt = (float)(frame_start - last_time);
        float input_screen_width = (float)platform->game.screenWidth;
        double frame_elapsed;

        last_time = frame_start;
        if (platform_take_reset_frame_time(platform) || dt > 0.1f) {
            dt = (float)reset_frame_seconds;
        }

        /* Keep input processing outside the window lock to avoid lifecycle deadlocks. */
        platform_process_input(platform, input_screen_width);
        platform_handle_exit_requested(platform);

        if (platform_draw(platform, dt)) {
            platform_update_fps(platform, dt);
        }
        platform_handle_exit_requested(platform);

        frame_elapsed = platform_now_seconds() - frame_start;
        if (target_frame_seconds > 0.0) {
            platform_sleep_seconds(target_frame_seconds - frame_elapsed);
        }
    }

    LOGI("Game loop stop");
    return NULL;
}

static void platform_start_game_loop(AndroidPlatform* platform) {
    if (platform == NULL) {
        return;
    }

    if (platform->loop_running) {
        return;
    }

    platform->loop_running = 1;

    if (pthread_create(&platform->thread, NULL, platform_game_loop, platform) != 0) {
        platform->loop_running = 0;
        LOGE("Failed to start game loop");
    }
}

static void platform_stop_game_loop(AndroidPlatform* platform) {
    if (platform == NULL) {
        return;
    }

    if (!platform->loop_running) {
        return;
    }

    platform->loop_running = 0;
    pthread_join(platform->thread, NULL);
}

static void platform_on_input_queue_created(
        ANativeActivity* activity,
        AInputQueue* queue
) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Input queue created");

    pthread_mutex_lock(&platform->input_queue_mutex);
    platform->input_queue = queue;
    pthread_mutex_unlock(&platform->input_queue_mutex);
}

static void platform_on_input_queue_destroyed(
        ANativeActivity* activity,
        AInputQueue* queue
) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Input queue destroyed");

    pthread_mutex_lock(&platform->input_queue_mutex);
    if (platform->attached_input_queue == queue) {
        AInputQueue_detachLooper(platform->attached_input_queue);
        platform->attached_input_queue = NULL;
    }
    platform->input_queue = NULL;
    platform->input_cancel_requested = 1;
    pthread_mutex_unlock(&platform->input_queue_mutex);
}

static void platform_on_native_window_created(
        ANativeActivity* activity,
        ANativeWindow* window
) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Native window created");
    ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);

    pthread_mutex_lock(&platform->window_mutex);
    platform->window = window;
    platform->null_window_logged = 0;
    platform->reset_frame_time = 1;
    pthread_mutex_unlock(&platform->window_mutex);
}

static void platform_on_native_window_destroyed(
        ANativeActivity* activity,
        ANativeWindow* window
) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Native window destroyed");
    pthread_mutex_lock(&platform->window_mutex);
    if (platform->window == window) {
        platform->window = NULL;
    }
    pthread_mutex_unlock(&platform->window_mutex);
}

static void platform_on_pause(ANativeActivity* activity) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Native activity paused");
    platform_save_progress(platform);
    platform_save_settings(platform);
    audio_pause();

    pthread_mutex_lock(&platform->input_queue_mutex);
    platform->pause_requested = 1;
    pthread_mutex_unlock(&platform->input_queue_mutex);
}

static void platform_on_resume(ANativeActivity* activity) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Native activity resumed");
    audio_resume();

    pthread_mutex_lock(&platform->window_mutex);
    platform->reset_frame_time = 1;
    pthread_mutex_unlock(&platform->window_mutex);
}

static void platform_on_destroy(ANativeActivity* activity) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Native activity destroyed");
    platform_stop_game_loop(platform);
    platform_save_progress(platform);
    platform_save_settings(platform);
    pthread_mutex_lock(&platform->window_mutex);
    platform->window = NULL;
    pthread_mutex_unlock(&platform->window_mutex);
    pthread_mutex_lock(&platform->input_queue_mutex);
    if (platform->attached_input_queue != NULL) {
        AInputQueue_detachLooper(platform->attached_input_queue);
        platform->attached_input_queue = NULL;
    }
    pthread_mutex_unlock(&platform->input_queue_mutex);
    pthread_mutex_destroy(&platform->input_queue_mutex);
    pthread_mutex_destroy(&platform->window_mutex);
    audio_shutdown();
    music_registry_shutdown_all();
    sound_registry_shutdown_all();
    generated_sprite_shutdown_all();
    activity->instance = NULL;

    free(platform);
}

void platform_android_on_create(
        ANativeActivity* activity,
        void* saved_state,
        size_t saved_state_size
) {
    (void)saved_state;
    (void)saved_state_size;

    AndroidPlatform* platform = (AndroidPlatform*)calloc(1, sizeof(AndroidPlatform));
    if (platform == NULL) {
        LOGE("Failed to allocate Android platform state");
        return;
    }

    activity->instance = platform;
    platform->activity = activity;
    pthread_mutex_init(&platform->window_mutex, NULL);
    pthread_mutex_init(&platform->input_queue_mutex, NULL);
    platform->fps_elapsed = 0.0;
    platform->fps_frame_time_total = 0.0;
    platform->fps_frame_count = 0;
    platform->null_window_logged = 0;
    platform->reset_frame_time = 1;
    platform_set_progress_path(platform, activity);
    platform_set_settings_path(platform, activity);
    game_settings_init_with_locale(&platform->game.settings, platform_detect_locale(activity));
    platform->game.settingsInitialized = 1;
    generated_sprite_initialize_all();
    hud_initialize();
    menu_initialize();
    sound_registry_initialize_all();
    music_registry_initialize_all();
    audio_init();
    platform_load_settings(platform);
    platform_load_progress(platform);
    game_init(&platform->game);
    input_init(&platform->input);

    activity->callbacks->onInputQueueCreated = platform_on_input_queue_created;
    activity->callbacks->onInputQueueDestroyed = platform_on_input_queue_destroyed;
    activity->callbacks->onNativeWindowCreated = platform_on_native_window_created;
    activity->callbacks->onNativeWindowDestroyed = platform_on_native_window_destroyed;
    activity->callbacks->onPause = platform_on_pause;
    activity->callbacks->onResume = platform_on_resume;
    activity->callbacks->onDestroy = platform_on_destroy;

    LOGI("Little One native activity created");
    platform_start_game_loop(platform);
}
