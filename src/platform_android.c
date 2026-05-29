#include "platform_android.h"

#include <android/log.h>
#include <android/native_window.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "game.h"
#include "renderer.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)

typedef struct AndroidPlatform {
    ANativeWindow* window;
    pthread_t thread;
    volatile int loop_running;
    GameState game;
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
    game_update(&platform->game, dt);
    renderer_draw_frame(&buffer, &platform->game);
    ANativeWindow_unlockAndPost(platform->window);

    return 1;
}

static void* platform_game_loop(void* data) {
    AndroidPlatform* platform = (AndroidPlatform*)data;
    double last_time = platform_now_seconds();

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

    LOGI("Native activity destroyed");
    platform_stop_game_loop(platform);
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
    game_init(&platform->game);

    activity->callbacks->onNativeWindowCreated = platform_on_native_window_created;
    activity->callbacks->onNativeWindowDestroyed = platform_on_native_window_destroyed;
    activity->callbacks->onDestroy = platform_on_destroy;

    LOGI("Little One native activity created");
}
