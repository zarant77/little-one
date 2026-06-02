#pragma once

#include <stdint.h>

#include "animation_definition.h"

AnimationImpact animation_impact_default(void);

AnimationImpact animation_evaluate(
        const AnimationClip *clip,
        int32_t time_ms
);

const AnimationClip *animation_find_clip(const char *id);
