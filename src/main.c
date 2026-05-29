#include <android/native_activity.h>

#include "platform_android.h"

void ANativeActivity_onCreate(
        ANativeActivity* activity,
        void* savedState,
        size_t savedStateSize
) {
    platform_android_on_create(activity, savedState, savedStateSize);
}
