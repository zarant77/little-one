#include <android/log.h>
#include <android/native_activity.h>

#define LOG_TAG "LittleOne"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

void ANativeActivity_onCreate(
        ANativeActivity* activity,
        void* savedState,
        size_t savedStateSize
) {
    (void)activity;
    (void)savedState;
    (void)savedStateSize;

    LOGI("Hello from Native C");
}