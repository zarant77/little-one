#ifndef LITTLE_ONE_FOREGROUND_DECORATION_CONFIG_H
#define LITTLE_ONE_FOREGROUND_DECORATION_CONFIG_H

#define FOREGROUND_MAX_INSTANCES 12
#define FOREGROUND_SCROLL_MULTIPLIER 1.0f
#define FOREGROUND_INITIAL_SPAWN_GAP_MIN 80.0f
#define FOREGROUND_INITIAL_SPAWN_GAP_MAX 240.0f
#define FOREGROUND_DEFAULT_MIN_GAP 180.0f
#define FOREGROUND_DEFAULT_MAX_GAP 420.0f
#define FOREGROUND_SPAWN_RIGHT_PADDING 16.0f
#define FOREGROUND_FADE_ENABLED 1
#define FOREGROUND_FADE_MIN_ALPHA 0.6f
#define FOREGROUND_FADE_DISTANCE 100.0f
#define FOREGROUND_FADE_USE_SMOOTH_DISTANCE 1

typedef struct
{
    const char *sprite_id;
    int width;
    int height;
    /* Distance from the gameplay ground line to the decoration bottom edge. */
    float spawn_y_min;
    float spawn_y_max;
    float min_gap;
    float max_gap;
    float scale_min;
    float scale_max;
    /* draw_offset_y defaults to 0.0f; spawn_weight defaults to 1.0f. */
    float draw_offset_y;
    float spawn_weight;
} ForegroundDecorationConfig;

const ForegroundDecorationConfig *foreground_decoration_config_get_all(int *count);

#endif
