#include "foreground_decoration_config.h"

static const ForegroundDecorationConfig FOREGROUND_DECORATION_CONFIGS[] = {
    {
        .sprite_id = "tree_oak_01",
        .width = 450,
        .height = 900,
        .spawn_y_min = 0.0f,
        .spawn_y_max = 0.0f,
        .min_gap = 420.0f,
        .max_gap = 800.0f,
        .scale_min = 0.75f,
        .scale_max = 1.0f,
        .draw_offset_y = 0.0f,
        .spawn_weight = 0.35f,
    },
    {
        .sprite_id = "tree_pine_01",
        .width = 520,
        .height = 830,
        .spawn_y_min = 0.0f,
        .spawn_y_max = 0.0f,
        .min_gap = 420.0f,
        .max_gap = 800.0f,
        .scale_min = 0.75f,
        .scale_max = 1.0f,
        .draw_offset_y = 0.0f,
        .spawn_weight = 0.35f,
    },
    {
        .sprite_id = "bush_01",
        .width = 550,
        .height = 260,
        .spawn_y_min = 0.0f,
        .spawn_y_max = 0.0f,
        .min_gap = FOREGROUND_DEFAULT_MIN_GAP,
        .max_gap = FOREGROUND_DEFAULT_MAX_GAP,
        .scale_min = 0.4f,
        .scale_max = 0.5f,
        .draw_offset_y = 0.0f,
        .spawn_weight = 1.0f,
    },
    {
        .sprite_id = "bush_02",
        .width = 510,
        .height = 180,
        .spawn_y_min = 0.0f,
        .spawn_y_max = 8.0f,
        .min_gap = FOREGROUND_DEFAULT_MIN_GAP,
        .max_gap = FOREGROUND_DEFAULT_MAX_GAP,
        .scale_min = 0.4f,
        .scale_max = 0.5f,
        .draw_offset_y = 0.0f,
        .spawn_weight = 1.0f,
    },
    {
        .sprite_id = "grass_01",
        .width = 230,
        .height = 200,
        .spawn_y_min = 0.0f,
        .spawn_y_max = 0.0f,
        .min_gap = 100.0f,
        .max_gap = 260.0f,
        .scale_min = 0.4f,
        .scale_max = 0.6f,
        .draw_offset_y = 0.0f,
        .spawn_weight = 1.5f,
    },
    {
        .sprite_id = "rock_01",
        .width = 270,
        .height = 172,
        .spawn_y_min = 0.0f,
        .spawn_y_max = 0.0f,
        .min_gap = FOREGROUND_DEFAULT_MIN_GAP,
        .max_gap = FOREGROUND_DEFAULT_MAX_GAP,
        .scale_min = 0.3f,
        .scale_max = 0.4f,
        .draw_offset_y = 0.0f,
        .spawn_weight = 0.8f,
    },
    {
        .sprite_id = "stump_01",
        .width = 360,
        .height = 300,
        .spawn_y_min = 0.0f,
        .spawn_y_max = 0.0f,
        .min_gap = 240.0f,
        .max_gap = 520.0f,
        .scale_min = 0.2f,
        .scale_max = 0.3f,
        .draw_offset_y = 0.0f,
        .spawn_weight = 0.65f,
    },
};

const ForegroundDecorationConfig *foreground_decoration_config_get_all(int *count)
{
    if (count != 0) {
        *count = (int)(sizeof(FOREGROUND_DECORATION_CONFIGS)
                / sizeof(FOREGROUND_DECORATION_CONFIGS[0]));
    }

    return FOREGROUND_DECORATION_CONFIGS;
}
