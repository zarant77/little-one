#include "platform_android.h"

#include <android/log.h>
#include <android/native_window.h>
#include <stdlib.h>

#include "config.h"
#include "renderer.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)

typedef struct AndroidPlatform {
    ANativeWindow* window;
} AndroidPlatform;

static AndroidPlatform* platform_from_activity(ANativeActivity* activity) {
    return (AndroidPlatform*)activity->instance;
}

static void platform_draw(AndroidPlatform* platform) {
    if (platform == NULL || platform->window == NULL) {
        return;
    }

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(platform->window, &buffer, NULL) != 0) {
        LOGE("Failed to lock native window");
        return;
    }

    renderer_draw_first_pixel(&buffer);
    ANativeWindow_unlockAndPost(platform->window);
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
    platform_draw(platform);
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
    platform->window = NULL;
}

static void platform_on_destroy(ANativeActivity* activity) {
    AndroidPlatform* platform = platform_from_activity(activity);

    LOGI("Native activity destroyed");
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

    activity->callbacks->onNativeWindowCreated = platform_on_native_window_created;
    activity->callbacks->onNativeWindowDestroyed = platform_on_native_window_destroyed;
    activity->callbacks->onDestroy = platform_on_destroy;

    LOGI("Little One native activity created");
}
