#include "background_config.h"

static const BackgroundConfig BACKGROUND_CONFIG = {
    .sky = {
        .top_color = 0x78c8ffff,
        .bottom_color = 0xf4f8ffff,
    },
    .layers = {
        {
            .sprite_id = "bg_mountains",
            .y = 50,
            .x_offset = 0,
            .scroll_num = 1,
            .scroll_den = 8,
            .repeat_x = 1,
            .repeat_y = 0,
            .enabled = true,
        },
        {
            .sprite_id = "bg_forest",
            .y = 40,
            .x_offset = 0,
            .scroll_num = 1,
            .scroll_den = 4,
            .repeat_x = 1,
            .repeat_y = 0,
            .enabled = true,
        },
        {
            .sprite_id = "fg_bushes",
            .y = 0,
            .x_offset = 0,
            .scroll_num = 3,
            .scroll_den = 4,
            .repeat_x = 1,
            .repeat_y = 0,
            .enabled = true,
        },
    },
    .layer_count = 3,
    .ground_visual = {
        .sprite_id = "ground_grass",
        .y = 0,
        .height = 300,
        .x_offset = 0,
        .repeat_x = true,
        .enabled = true,
    },
};

const BackgroundConfig *background_config_get(void)
{
    return &BACKGROUND_CONFIG;
}
