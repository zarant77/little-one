#include "platform_android.h"

#include <android/input.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "game.h"
#include "input.h"
#include "renderer.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)

typedef struct AndroidPlatform {
    ANativeWindow* window;
    AInputQueue* input_queue;
    AInputQueue* attached_input_queue;
    pthread_mutex_t input_queue_mutex;
    int input_cancel_requested;
    pthread_t thread;
    volatile int loop_running;
    GameState game;
    InputState input;
} AndroidPlatform;

static AndroidPlatform* platform_from_activity(ANativeActivity* activity) {
    return (AndroidPlatform*)activity->instance;
}

static double platform_now_seconds(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);

    return (double)time.tv_sec + ((double)time.tv_nsec / 1000000000.0);
}

static void platform_sleep_frame(void) {
    struct timespec frame_time;
    frame_time.tv_sec = 0;
    frame_time.tv_nsec = 16666667;

    nanosleep(&frame_time, NULL);
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

    if (action_type == AMOTION_EVENT_ACTION_DOWN
            || action_type == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        int pointer_id = AMotionEvent_getPointerId(event, action_index);
        float x = AMotionEvent_getX(event, action_index);
        float y = AMotionEvent_getY(event, action_index);

        input_handle_touch(
                &platform->input,
                INPUT_TOUCH_DOWN,
                pointer_id,
                x,
                y,
                screen_width
        );
        return;
    }

    if (action_type == AMOTION_EVENT_ACTION_MOVE) {
        size_t pointer_count = AMotionEvent_getPointerCount(event);

        for (size_t pointer_index = 0; pointer_index < pointer_count; ++pointer_index) {
            int pointer_id = AMotionEvent_getPointerId(event, pointer_index);
            float x = AMotionEvent_getX(event, pointer_index);
            float y = AMotionEvent_getY(event, pointer_index);

            input_handle_touch(
                    &platform->input,
                    INPUT_TOUCH_MOVE,
                    pointer_id,
                    x,
                    y,
                    screen_width
            );
        }

        return;
    }

    if (action_type == AMOTION_EVENT_ACTION_UP
            || action_type == AMOTION_EVENT_ACTION_POINTER_UP) {
        int pointer_id = AMotionEvent_getPointerId(event, action_index);

        input_handle_touch(
                &platform->input,
                INPUT_TOUCH_UP,
                pointer_id,
                0.0f,
                0.0f,
                screen_width
        );
        return;
    }

    if (action_type == AMOTION_EVENT_ACTION_CANCEL) {
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

static void platform_process_input(AndroidPlatform* platform, float screen_width) {
    AInputQueue* queue;
    AInputEvent* event = NULL;

    pthread_mutex_lock(&platform->input_queue_mutex);
    queue = platform->input_queue;

    if (platform->input_cancel_requested) {
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

    ALooper_pollOnce(0, NULL, NULL, NULL);

    while (AInputQueue_getEvent(queue, &event) >= 0) {
        int handled = 0;

        if (AInputQueue_preDispatchEvent(queue, event)) {
            continue;
        }

        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            platform_handle_motion_event(platform, event, screen_width);
            handled = 1;
        }

        AInputQueue_finishEvent(queue, event, handled);
    }

    pthread_mutex_unlock(&platform->input_queue_mutex);
}

static int platform_draw(AndroidPlatform* platform, float dt) {
    if (platform == NULL || platform->window == NULL) {
        return 0;
    }

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(platform->window, &buffer, NULL) != 0) {
        LOGE("Failed to lock native window");
        return 0;
    }

    game_set_screen_size(&platform->game, (float)buffer.width, (float)buffer.height);
    platform_process_input(platform, (float)buffer.width);
    game_update(&platform->game, &platform->input, dt);
    input_end_frame(&platform->input);
    renderer_draw_frame(&buffer, &platform->game);
    ANativeWindow_unlockAndPost(platform->window);

    return 1;
}

static void* platform_game_loop(void* data) {
    AndroidPlatform* platform = (AndroidPlatform*)data;
    double last_time = platform_now_seconds();

    ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

    while (platform->loop_running) {
        double now = platform_now_seconds();
        float dt = (float)(now - last_time);
        last_time = now;

        platform_draw(platform, dt);
        platform_sleep_frame();
    }

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
    platform->window = window;

    ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);
    platform_start_game_loop(platform);
}

static void platform_on_native_window_destroyed(
        ANativeActivity* activity,
        ANativeWindow* window
) {
    (void)window;

    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Native window destroyed");
    platform_stop_game_loop(platform);
    platform->window = NULL;
}

static void platform_on_destroy(ANativeActivity* activity) {
    AndroidPlatform* platform = platform_from_activity(activity);
    if (platform == NULL) {
        return;
    }

    LOGI("Native activity destroyed");
    platform_stop_game_loop(platform);
    pthread_mutex_lock(&platform->input_queue_mutex);
    if (platform->attached_input_queue != NULL) {
        AInputQueue_detachLooper(platform->attached_input_queue);
        platform->attached_input_queue = NULL;
    }
    pthread_mutex_unlock(&platform->input_queue_mutex);
    pthread_mutex_destroy(&platform->input_queue_mutex);
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
    pthread_mutex_init(&platform->input_queue_mutex, NULL);
    game_init(&platform->game);
    input_init(&platform->input);

    activity->callbacks->onInputQueueCreated = platform_on_input_queue_created;
    activity->callbacks->onInputQueueDestroyed = platform_on_input_queue_destroyed;
    activity->callbacks->onNativeWindowCreated = platform_on_native_window_created;
    activity->callbacks->onNativeWindowDestroyed = platform_on_native_window_destroyed;
    activity->callbacks->onDestroy = platform_on_destroy;

    LOGI("Little One native activity created");
}
