#include "animation_evaluate.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

AnimationImpact animation_impact_default(void) {
    AnimationImpact impact;

    impact.offset_x = 0;
    impact.offset_y = 0;
    impact.scale_x = 1000;
    impact.scale_y = 1000;
    impact.rotation = 0;
    impact.alpha = 255;

    return impact;
}

static int32_t animation_clamp_i32(int32_t value, int32_t min, int32_t max) {
    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
}

static int32_t animation_wrap_time(int32_t time_ms, int32_t duration_ms) {
    int32_t local_time = time_ms % duration_ms;

    if (local_time < 0) {
        local_time += duration_ms;
    }

    return local_time;
}

static int32_t animation_apply_easing(int32_t t, uint8_t easing) {
    int32_t inverse;

    t = animation_clamp_i32(t, 0, 1000);

    switch (easing) {
        case ANIM_EASE_IN:
            return (t * t) / 1000;

        case ANIM_EASE_OUT:
            inverse = 1000 - t;
            return 1000 - (inverse * inverse) / 1000;

        case ANIM_EASE_IN_OUT:
            if (t < 500) {
                return (2 * t * t) / 1000;
            }

            inverse = 1000 - t;
            return 1000 - (2 * inverse * inverse) / 1000;

        case ANIM_EASE_STEP:
            return 0;

        case ANIM_EASE_LINEAR:
        default:
            return t;
    }
}

static int16_t animation_evaluate_track(const AnimTrack *track, int32_t local_time) {
    const AnimKey *keys;
    const AnimKey *first_key;
    const AnimKey *last_key;

    keys = track->keys;
    first_key = keys;
    last_key = keys + (track->key_count - 1);

    if (local_time <= first_key->time_ms) {
        return first_key->value;
    }

    if (local_time >= last_key->time_ms) {
        return last_key->value;
    }

    for (uint8_t key_index = 0; key_index + 1 < track->key_count; ++key_index) {
        const AnimKey *from_key = keys + key_index;
        const AnimKey *to_key = keys + key_index + 1;
        int32_t segment_duration;
        int32_t t;
        int32_t eased_t;
        int32_t value_delta;
        int32_t value;

        if (local_time < from_key->time_ms || local_time > to_key->time_ms) {
            continue;
        }

        if (local_time == to_key->time_ms) {
            return to_key->value;
        }

        segment_duration = (int32_t)to_key->time_ms - (int32_t)from_key->time_ms;
        if (segment_duration <= 0) {
            return to_key->value;
        }

        t = ((local_time - (int32_t)from_key->time_ms) * 1000) / segment_duration;
        eased_t = animation_apply_easing(t, to_key->easing);
        value_delta = (int32_t)to_key->value - (int32_t)from_key->value;
        value = (int32_t)from_key->value + (value_delta * eased_t) / 1000;

        return (int16_t)value;
    }

    return last_key->value;
}

static void animation_apply_track_value(
        AnimationImpact *impact,
        uint8_t property,
        int16_t value
) {
    switch (property) {
        case ANIM_PROP_OFFSET_X:
            impact->offset_x = value;
            break;

        case ANIM_PROP_OFFSET_Y:
            impact->offset_y = value;
            break;

        case ANIM_PROP_SCALE_X:
            impact->scale_x = value;
            break;

        case ANIM_PROP_SCALE_Y:
            impact->scale_y = value;
            break;

        case ANIM_PROP_ROTATION:
            impact->rotation = value;
            break;

        case ANIM_PROP_ALPHA:
            impact->alpha = (uint8_t)animation_clamp_i32(value, 0, 255);
            break;

        default:
            break;
    }
}

AnimationImpact animation_evaluate(
        const AnimationClip *clip,
        int32_t time_ms
) {
    AnimationImpact impact = animation_impact_default();
    int32_t duration_ms;
    int32_t local_time;

    if (clip == NULL || clip->duration_ms <= 0) {
        return impact;
    }

    duration_ms = (int32_t)clip->duration_ms;
    if (clip->loop) {
        local_time = animation_wrap_time(time_ms, duration_ms);
    } else {
        local_time = animation_clamp_i32(time_ms, 0, duration_ms);
    }

    for (uint8_t track_index = 0; track_index < clip->track_count; ++track_index) {
        const AnimTrack *track = clip->tracks + track_index;
        int16_t value;

        if (track->keys == NULL || track->key_count == 0) {
            continue;
        }

        value = animation_evaluate_track(track, local_time);
        animation_apply_track_value(&impact, track->property, value);
    }

    return impact;
}

const AnimationClip *animation_find_clip(const char *id) {
    if (id == NULL) {
        return NULL;
    }

    for (uint8_t clip_index = 0; clip_index < ANIMATION_CLIP_COUNT; ++clip_index) {
        const AnimationClip *clip = ANIMATION_CLIPS + clip_index;

        if (clip->id != NULL && strcmp(clip->id, id) == 0) {
            return clip;
        }
    }

    return NULL;
}
