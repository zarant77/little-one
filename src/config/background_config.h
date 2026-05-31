#ifndef LITTLE_ONE_BACKGROUND_CONFIG_H
#define LITTLE_ONE_BACKGROUND_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define LITTLE_ONE_MAX_PARALLAX_LAYERS 8

typedef struct {
    uint32_t top_color;
    uint32_t bottom_color;
} SkyGradientConfig;

typedef struct {
    const char* sprite_id;
    /* Distance from gameplay ground line to the sprite bottom edge. */
    int16_t y;
    int16_t x_offset;
    int16_t scroll_num;
    int16_t scroll_den;
    int16_t repeat_x;
    int16_t repeat_y;
    bool enabled;
} ParallaxLayerConfig;

typedef struct {
    const char* sprite_id;
    int16_t y;
    int16_t height;
    int16_t x_offset;
    bool repeat_x;
    bool enabled;
} GroundVisualConfig;

typedef struct {
    SkyGradientConfig sky;
    ParallaxLayerConfig layers[LITTLE_ONE_MAX_PARALLAX_LAYERS];
    int16_t layer_count;
    GroundVisualConfig ground_visual;
} BackgroundConfig;

const BackgroundConfig* background_config_get(void);

#endif
