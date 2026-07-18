#include "generated_sprite.h"

#include <stdlib.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
#include "../config.h"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LITTLE_ONE_LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) ((void)0)
#define LOGE(...) ((void)0)
#endif

#include "definitions/sprites_packed.h"

enum
{
    MAX_SPRITE_WIDTH = 4096,
    MAX_SPRITE_HEIGHT = 4096,
    MAX_SPRITE_PIXELS = 1024 * 1024,
    SPRITE_TRIG_SCALE = 65536,
    SPRITE_PI_MILLIRADIANS = 3141,
    SPRITE_TWO_PI_MILLIRADIANS = 6283,
    SPRITE_HALF_PI_MILLIRADIANS = 1571
};

static GeneratedSprite *GENERATED_SPRITES = 0;
static size_t GENERATED_SPRITE_COUNT = 0;

static int generated_sprites_initialized = 0;

static const char *SPRITE_ID_NAMES[SPRITE_ID_COUNT] = {
    "player",
    "boar",
    "ork",
    "rat",
    "bird",
    "bat",
    "rock",
    "cactus",
    "stump",
};

static int sprite_alpha(uint32_t color)
{
    return (int)(color & 0xff);
}

static uint8_t sprite_r(uint32_t color)
{
    return (uint8_t)((color >> 24) & 0xff);
}

static uint8_t sprite_g(uint32_t color)
{
    return (uint8_t)((color >> 16) & 0xff);
}

static uint8_t sprite_b(uint32_t color)
{
    return (uint8_t)((color >> 8) & 0xff);
}

static uint32_t sprite_pack_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a;
}

static uint32_t sprite_blend_rgba(uint32_t src_color, uint32_t dst_color)
{
    int src_a = sprite_alpha(src_color);
    int dst_a = sprite_alpha(dst_color);
    int inv_src_a;
    int out_a;
    int out_r;
    int out_g;
    int out_b;

    if (src_a <= 0)
    {
        return dst_color;
    }

    if (src_a >= 255 || dst_a <= 0)
    {
        return src_color;
    }

    inv_src_a = 255 - src_a;
    out_a = src_a + (dst_a * inv_src_a) / 255;
    if (out_a <= 0)
    {
        return 0x00000000;
    }

    out_r = ((int)sprite_r(src_color) * src_a * 255 + (int)sprite_r(dst_color) * dst_a * inv_src_a) / (out_a * 255);
    out_g = ((int)sprite_g(src_color) * src_a * 255 + (int)sprite_g(dst_color) * dst_a * inv_src_a) / (out_a * 255);
    out_b = ((int)sprite_b(src_color) * src_a * 255 + (int)sprite_b(dst_color) * dst_a * inv_src_a) / (out_a * 255);

    return sprite_pack_rgba((uint8_t)out_r, (uint8_t)out_g, (uint8_t)out_b, (uint8_t)out_a);
}

static int sprite_abs_int(int value)
{
    return value < 0 ? -value : value;
}

static int64_t sprite_abs_i64(int64_t value)
{
    return value < 0 ? -value : value;
}

static int sprite_normalize_rotation(int rotation)
{
    while (rotation > SPRITE_PI_MILLIRADIANS)
    {
        rotation -= SPRITE_TWO_PI_MILLIRADIANS;
    }

    while (rotation < -SPRITE_PI_MILLIRADIANS)
    {
        rotation += SPRITE_TWO_PI_MILLIRADIANS;
    }

    return rotation;
}

static int sprite_sin_q16(int rotation)
{
    int normalized = sprite_normalize_rotation(rotation);
    int64_t x;
    int64_t x2;
    int64_t result;

    if (normalized > SPRITE_HALF_PI_MILLIRADIANS)
    {
        normalized = SPRITE_PI_MILLIRADIANS - normalized;
    }
    else if (normalized < -SPRITE_HALF_PI_MILLIRADIANS)
    {
        normalized = -SPRITE_PI_MILLIRADIANS - normalized;
    }

    x = ((int64_t)normalized * SPRITE_TRIG_SCALE) / 1000;
    x2 = (x * x) / SPRITE_TRIG_SCALE;
    result = x;
    result -= (((x * x2) / SPRITE_TRIG_SCALE) / 6);
    result += (((((x * x2) / SPRITE_TRIG_SCALE) * x2) / SPRITE_TRIG_SCALE) / 120);
    result -= (((((((x * x2) / SPRITE_TRIG_SCALE) * x2) / SPRITE_TRIG_SCALE) * x2) / SPRITE_TRIG_SCALE) / 5040);

    if (result > SPRITE_TRIG_SCALE)
    {
        return SPRITE_TRIG_SCALE;
    }
    if (result < -SPRITE_TRIG_SCALE)
    {
        return -SPRITE_TRIG_SCALE;
    }

    return (int)result;
}

static int sprite_cos_q16(int rotation)
{
    return sprite_sin_q16(rotation + SPRITE_HALF_PI_MILLIRADIANS);
}

static void sprite_rotate_point(
    int center_x,
    int center_y,
    int local_x,
    int local_y,
    int cos_rotation,
    int sin_rotation,
    int *out_x,
    int *out_y)
{
    *out_x = center_x + (int)(((int64_t)local_x * cos_rotation - (int64_t)local_y * sin_rotation) / SPRITE_TRIG_SCALE);
    *out_y = center_y + (int)(((int64_t)local_x * sin_rotation + (int64_t)local_y * cos_rotation) / SPRITE_TRIG_SCALE);
}

static int sprite_id_equals(const char *left, const char *right)
{
    int index = 0;

    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (left[index] != 0 && right[index] != 0)
    {
        if (left[index] != right[index])
        {
            return 0;
        }
        index += 1;
    }

    return left[index] == right[index];
}

static int sprite_pixel_count(int width, int height, size_t *pixel_count)
{
    size_t width_size;
    size_t height_size;
    size_t count;

    if (pixel_count == 0)
    {
        return 0;
    }

    *pixel_count = 0;

    if (width <= 0 || height <= 0)
    {
        return 0;
    }

    width_size = (size_t)width;
    height_size = (size_t)height;
    count = width_size * height_size;

    if (width > MAX_SPRITE_WIDTH || height > MAX_SPRITE_HEIGHT || count > (size_t)MAX_SPRITE_PIXELS)
    {
        return 0;
    }

    *pixel_count = count;
    return 1;
}

static int packed_sprite_width(const PackedSpriteDefinition *definition)
{
    return (int)definition->width;
}

static int packed_sprite_height(const PackedSpriteDefinition *definition)
{
    return (int)definition->height;
}

static int packed_command_decode(uint8_t value, uint8_t bank)
{
    return (int)value + ((int)bank * 256);
}

static int packed_command_x(const PackedSpriteCommand *command)
{
    return packed_command_decode(
        command->x,
        (command->bank_mask >> 0) & 0x03);
}

static int packed_command_y(const PackedSpriteCommand *command)
{
    return packed_command_decode(
        command->y,
        (command->bank_mask >> 2) & 0x03);
}

static int packed_command_w(const PackedSpriteCommand *command)
{
    return packed_command_decode(
        command->w,
        (command->bank_mask >> 4) & 0x03);
}

static int packed_command_h(const PackedSpriteCommand *command)
{
    return packed_command_decode(
        command->h,
        (command->bank_mask >> 6) & 0x03);
}

static int packed_command_rotation(const PackedSpriteCommand *command)
{
    return (int)(((uint32_t)command->rotation * SPRITE_TWO_PI_MILLIRADIANS) / 255u);
}

static void generated_sprite_reset(GeneratedSprite *sprite)
{
    if (sprite == 0)
    {
        return;
    }

    sprite->id = 0;
    sprite->width = 0;
    sprite->height = 0;
    sprite->pivot_x = 0;
    sprite->pivot_y = 0;
    sprite->pixels = 0;
}

static void generated_sprite_release(GeneratedSprite *sprite)
{
    if (sprite == 0)
    {
        return;
    }

    free(sprite->pixels);
    generated_sprite_reset(sprite);
}

static int packed_sprite_definition_is_valid(
    const PackedSpriteDefinition *definition,
    size_t sprite_index,
    size_t *pixel_count)
{
    if (definition == 0)
    {
        LOGE("Generated sprite %zu invalid: definition is null", sprite_index);
        return 0;
    }

    if (definition->id == 0)
    {
        LOGE("Generated sprite %zu invalid: id is null", sprite_index);
        return 0;
    }

    if (definition->command_count > 0 && definition->commands == 0)
    {
        LOGE(
            "Generated sprite %zu (%s) invalid: commands are null for command_count=%u",
            sprite_index,
            definition->id,
            (unsigned int)definition->command_count);
        return 0;
    }

    if (!sprite_pixel_count(packed_sprite_width(definition), packed_sprite_height(definition), pixel_count))
    {
        LOGE(
            "Generated sprite %zu (%s) invalid: width=%d height=%d max_pixels=%d",
            sprite_index,
            definition->id,
            packed_sprite_width(definition),
            packed_sprite_height(definition),
            MAX_SPRITE_PIXELS);
        return 0;
    }

    return 1;
}

static void sprite_clear(uint32_t *pixels, int width, int height)
{
    size_t pixel_count;

    if (pixels == 0 || !sprite_pixel_count(width, height, &pixel_count))
    {
        return;
    }

    memset(pixels, 0, pixel_count * sizeof(uint32_t));
}

static void sprite_put_pixel(uint32_t *pixels, int width, int height, int x, int y, uint32_t color)
{
    size_t pixel_index;

    if (pixels == 0)
    {
        return;
    }

    if (sprite_alpha(color) == 0)
    {
        return;
    }

    if (x < 0 || y < 0 || x >= width || y >= height)
    {
        return;
    }

    pixel_index = (size_t)y * (size_t)width + (size_t)x;
    pixels[pixel_index] = sprite_blend_rgba(color, pixels[pixel_index]);
}

static void sprite_draw_rect(
    uint32_t *pixels,
    int sprite_width,
    int sprite_height,
    int center_x,
    int center_y,
    int width,
    int height,
    int rotation,
    uint32_t color)
{
    int cos_rotation;
    int sin_rotation;
    int64_t half_width = ((int64_t)width * SPRITE_TRIG_SCALE) / 2;
    int64_t half_height = ((int64_t)height * SPRITE_TRIG_SCALE) / 2;
    int extent_x;
    int extent_y;
    int left;
    int top;
    int right;
    int bottom;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (rotation == 0)
    {
        left = center_x - width / 2;
        top = center_y - height / 2;
        right = left + width;
        bottom = top + height;

        for (int y = top; y < bottom; ++y)
        {
            for (int x = left; x < right; ++x)
            {
                sprite_put_pixel(pixels, sprite_width, sprite_height, x, y, color);
            }
        }

        return;
    }

    cos_rotation = sprite_cos_q16(rotation);
    sin_rotation = sprite_sin_q16(rotation);
    extent_x = (int)(((int64_t)sprite_abs_int(cos_rotation) * width + (int64_t)sprite_abs_int(sin_rotation) * height) / (2 * SPRITE_TRIG_SCALE)) + 2;
    extent_y = (int)(((int64_t)sprite_abs_int(sin_rotation) * width + (int64_t)sprite_abs_int(cos_rotation) * height) / (2 * SPRITE_TRIG_SCALE)) + 2;
    left = center_x - extent_x;
    top = center_y - extent_y;
    right = center_x + extent_x;
    bottom = center_y + extent_y;

    for (int y = top; y < bottom; ++y)
    {
        for (int x = left; x < right; ++x)
        {
            int dx = x - center_x;
            int dy = y - center_y;
            int64_t local_x = (int64_t)dx * cos_rotation + (int64_t)dy * sin_rotation;
            int64_t local_y = -(int64_t)dx * sin_rotation + (int64_t)dy * cos_rotation;

            if (sprite_abs_i64(local_x) <= half_width && sprite_abs_i64(local_y) <= half_height)
            {
                sprite_put_pixel(pixels, sprite_width, sprite_height, x, y, color);
            }
        }
    }
}

static void sprite_draw_ellipse(
    uint32_t *pixels,
    int sprite_width,
    int sprite_height,
    int center_x,
    int center_y,
    int width,
    int height,
    int rotation,
    uint32_t color)
{
    int radius_x;
    int radius_y;
    int cos_rotation;
    int sin_rotation;
    int extent_x;
    int extent_y;
    int left;
    int top;
    int right;
    int bottom;
    int64_t rx2;
    int64_t ry2;
    int64_t limit;

    if (width <= 0)
    {
        return;
    }

    if (height <= 0)
    {
        height = width;
    }

    radius_x = width / 2;
    radius_y = height / 2;

    if (radius_x <= 0 || radius_y <= 0)
    {
        return;
    }

    cos_rotation = sprite_cos_q16(rotation);
    sin_rotation = sprite_sin_q16(rotation);

    extent_x = (int)(((int64_t)sprite_abs_int(cos_rotation) * radius_x + (int64_t)sprite_abs_int(sin_rotation) * radius_y) / SPRITE_TRIG_SCALE) + 2;

    extent_y = (int)(((int64_t)sprite_abs_int(sin_rotation) * radius_x + (int64_t)sprite_abs_int(cos_rotation) * radius_y) / SPRITE_TRIG_SCALE) + 2;

    left = center_x - extent_x;
    top = center_y - extent_y;
    right = center_x + extent_x;
    bottom = center_y + extent_y;

    rx2 = (int64_t)radius_x * radius_x;
    ry2 = (int64_t)radius_y * radius_y;
    limit = rx2 * ry2;

    for (int y = top; y <= bottom; ++y)
    {
        for (int x = left; x <= right; ++x)
        {
            int dx = x - center_x;
            int dy = y - center_y;

            int64_t local_x = ((int64_t)dx * cos_rotation + (int64_t)dy * sin_rotation) / SPRITE_TRIG_SCALE;

            int64_t local_y = (-(int64_t)dx * sin_rotation + (int64_t)dy * cos_rotation) / SPRITE_TRIG_SCALE;

            int64_t value = local_x * local_x * ry2 + local_y * local_y * rx2;

            if (value <= limit)
            {
                sprite_put_pixel(pixels, sprite_width, sprite_height, x, y, color);
            }
        }
    }
}

static int sprite_edge(int ax, int ay, int bx, int by, int px, int py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static void sprite_draw_triangle(
    uint32_t *pixels,
    int sprite_width,
    int sprite_height,
    int center_x,
    int center_y,
    int width,
    int height,
    int rotation,
    uint32_t color)
{
    int cos_rotation = sprite_cos_q16(rotation);
    int sin_rotation = sprite_sin_q16(rotation);
    int x1;
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;
    int min_x;
    int max_x;
    int min_y;
    int max_y;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    sprite_rotate_point(
        center_x,
        center_y,
        0,
        -height / 2,
        cos_rotation,
        sin_rotation,
        &x1,
        &y1);
    sprite_rotate_point(
        center_x,
        center_y,
        -width / 2,
        height / 2,
        cos_rotation,
        sin_rotation,
        &x2,
        &y2);
    sprite_rotate_point(
        center_x,
        center_y,
        width / 2,
        height / 2,
        cos_rotation,
        sin_rotation,
        &x3,
        &y3);

    min_x = x1;
    max_x = x1;
    min_y = y1;
    max_y = y1;

    if (x2 < min_x)
    {
        min_x = x2;
    }
    if (x3 < min_x)
    {
        min_x = x3;
    }
    if (x2 > max_x)
    {
        max_x = x2;
    }
    if (x3 > max_x)
    {
        max_x = x3;
    }
    if (y2 < min_y)
    {
        min_y = y2;
    }
    if (y3 < min_y)
    {
        min_y = y3;
    }
    if (y2 > max_y)
    {
        max_y = y2;
    }
    if (y3 > max_y)
    {
        max_y = y3;
    }

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            int edge1 = sprite_edge(x1, y1, x2, y2, x, y);
            int edge2 = sprite_edge(x2, y2, x3, y3, x, y);
            int edge3 = sprite_edge(x3, y3, x1, y1, x, y);

            if ((edge1 >= 0 && edge2 >= 0 && edge3 >= 0) || (edge1 <= 0 && edge2 <= 0 && edge3 <= 0))
            {
                sprite_put_pixel(pixels, sprite_width, sprite_height, x, y, color);
            }
        }
    }
}

static void sprite_generate(const PackedSpriteDefinition *definition, GeneratedSprite *sprite)
{
    sprite_clear(sprite->pixels, sprite->width, sprite->height);

    for (uint16_t command_index = 0; command_index < definition->command_count; ++command_index)
    {
        const PackedSpriteCommand *command = definition->commands + command_index;
        int x = packed_command_x(command);
        int y = packed_command_y(command);
        int w = packed_command_w(command);
        int h = packed_command_h(command);
        int rotation = packed_command_rotation(command);
        uint32_t color;

        if (command->color_index >= SPRITE_PALETTE_COUNT)
        {
            LOGE(
                "Generated sprite %s command %u invalid: color_index=%u palette_count=%u",
                definition->id != 0 ? definition->id : "(null)",
                (unsigned int)command_index,
                (unsigned int)command->color_index,
                (unsigned int)SPRITE_PALETTE_COUNT);
            continue;
        }

        color = SPRITE_PALETTE[command->color_index];

        if (command->kind == PACKED_SPRITE_CMD_RECT)
        {
            sprite_draw_rect(
                sprite->pixels,
                sprite->width,
                sprite->height,
                x,
                y,
                w,
                h,
                rotation,
                color);
        }
        else if (command->kind == PACKED_SPRITE_CMD_CIRCLE)
        {
            sprite_draw_ellipse(
                sprite->pixels,
                sprite->width,
                sprite->height,
                x,
                y,
                w,
                h,
                rotation,
                color);
        }
        else if (command->kind == PACKED_SPRITE_CMD_TRIANGLE)
        {
            sprite_draw_triangle(
                sprite->pixels,
                sprite->width,
                sprite->height,
                x,
                y,
                w,
                h,
                rotation,
                color);
        }
        else
        {
            LOGE(
                "Generated sprite %s command %u invalid: kind=%u",
                definition->id != 0 ? definition->id : "(null)",
                (unsigned int)command_index,
                (unsigned int)command->kind);
        }
    }
}

void generated_sprite_initialize_all(void)
{
    if (generated_sprites_initialized)
    {
        return;
    }

    GENERATED_SPRITE_COUNT = (size_t)PACKED_SPRITE_DEFINITION_COUNT;
    if (GENERATED_SPRITE_COUNT == 0)
    {
        generated_sprites_initialized = 1;
        return;
    }

    GENERATED_SPRITES = (GeneratedSprite *)calloc(GENERATED_SPRITE_COUNT, sizeof(GeneratedSprite));
    if (GENERATED_SPRITES == 0)
    {
        LOGE("Generated sprites allocation failed: count=%zu", GENERATED_SPRITE_COUNT);
        GENERATED_SPRITE_COUNT = 0;
        generated_sprites_initialized = 1;
        return;
    }

    for (size_t sprite_index = 0; sprite_index < GENERATED_SPRITE_COUNT; ++sprite_index)
    {
        const PackedSpriteDefinition *definition = PACKED_SPRITE_DEFINITIONS[sprite_index];
        GeneratedSprite *sprite = GENERATED_SPRITES + sprite_index;
        size_t pixel_count;
        size_t byte_count;
        uint32_t *pixels;

        generated_sprite_release(sprite);

        if (definition != 0)
        {
            LOGI(
                "Generated sprite %zu: id=%s width=%d height=%d command_count=%u",
                sprite_index,
                definition->id != 0 ? definition->id : "(null)",
                packed_sprite_width(definition),
                packed_sprite_height(definition),
                (unsigned int)definition->command_count);
        }

        if (!packed_sprite_definition_is_valid(definition, sprite_index, &pixel_count))
        {
            continue;
        }

        byte_count = pixel_count * sizeof(uint32_t);
        pixels = (uint32_t *)malloc(byte_count);
        if (pixels == 0)
        {
            LOGE(
                "Generated sprite %zu (%s) allocation failed: pixels=%zu bytes=%zu",
                sprite_index,
                definition->id,
                pixel_count,
                byte_count);
            continue;
        }

        sprite->id = definition->id;
        sprite->width = (int16_t)packed_sprite_width(definition);
        sprite->height = (int16_t)packed_sprite_height(definition);
        sprite->pivot_x = (int16_t)(sprite->width / 2);
        sprite->pivot_y = sprite->height;
        sprite->pixels = pixels;

        sprite_generate(definition, sprite);
    }

    generated_sprites_initialized = 1;
}

void generated_sprite_shutdown_all(void)
{
    for (size_t sprite_index = 0; sprite_index < GENERATED_SPRITE_COUNT; ++sprite_index)
    {
        generated_sprite_release(GENERATED_SPRITES + sprite_index);
    }

    free(GENERATED_SPRITES);
    GENERATED_SPRITES = 0;
    GENERATED_SPRITE_COUNT = 0;
    generated_sprites_initialized = 0;
}

const GeneratedSprite *generated_sprite_get(SpriteId sprite_id)
{
    if (sprite_id < 0 || sprite_id >= SPRITE_ID_COUNT)
    {
        return 0;
    }

    return generated_sprite_get_by_id(SPRITE_ID_NAMES[sprite_id]);
}

const GeneratedSprite *generated_sprite_get_by_id(const char *sprite_id)
{
    for (size_t sprite_index = 0; sprite_index < GENERATED_SPRITE_COUNT; ++sprite_index)
    {
        if (GENERATED_SPRITES[sprite_index].pixels != 0 && sprite_id_equals(GENERATED_SPRITES[sprite_index].id, sprite_id))
        {
            return GENERATED_SPRITES + sprite_index;
        }
    }

    return 0;
}
