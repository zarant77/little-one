#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ANIM_PROP_OFFSET_X = 0,
    ANIM_PROP_OFFSET_Y = 1,
    ANIM_PROP_SCALE_X  = 2,
    ANIM_PROP_SCALE_Y  = 3,
    ANIM_PROP_ROTATION = 4,
    ANIM_PROP_ALPHA    = 5
} AnimProperty;

typedef enum {
    ANIM_EASE_LINEAR      = 0,
    ANIM_EASE_IN          = 1,
    ANIM_EASE_OUT         = 2,
    ANIM_EASE_IN_OUT      = 3,
    ANIM_EASE_STEP        = 4
} AnimEasing;

typedef struct {
    int16_t time_ms;
    int16_t value;
    uint8_t easing;
} AnimKey;

typedef struct {
    uint8_t property;
    const AnimKey *keys;
    uint8_t key_count;
} AnimTrack;

typedef struct {
    const char *id;

    int16_t duration_ms;
    bool loop;

    const AnimTrack *tracks;
    uint8_t track_count;
} AnimationClip;

typedef struct {
    int16_t offset_x;
    int16_t offset_y;

    int16_t scale_x;
    int16_t scale_y;

    int16_t rotation;
    uint8_t alpha;
} AnimationImpact;

extern const AnimationClip ANIMATION_CLIPS[];
extern const uint8_t ANIMATION_CLIP_COUNT;
