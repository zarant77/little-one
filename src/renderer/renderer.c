#include "renderer.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../config/background_config.h"
#include "../fonts/font_renderer.h"
#include "../game/game_effects.h"
#include "../game/game_settings.h"
#include "../localization/localization.h"
#include "../sprites/animations/animation_evaluate.h"
#include "../ui/hud.h"
#include "../ui/menu.h"

#define RENDERER_GROUND_LINE_HEIGHT 2
#define RENDERER_DIGIT_WIDTH 12
#define RENDERER_DIGIT_HEIGHT 20
#define RENDERER_DIGIT_THICKNESS 3
#define RENDERER_DIGIT_SPACING 4
#define DEFAULT_SPRITE_FIT_MODE SPRITE_FIT_CONTAIN
#define RENDERER_SIZE_WIREFRAME_COLOR 0xffff00ff
#define RENDERER_HURT_ZONE_WIREFRAME_COLOR 0xff0000ff
#define RENDERER_ATTACK_ZONE_WIREFRAME_COLOR 0x00ffffff
#define RENDERER_GROUND_DEBUG_COLOR 0x00ffffff
#define RENDERER_TRIG_SCALE 65536
#define RENDERER_PI_MILLIRADIANS 3141
#define RENDERER_TWO_PI_MILLIRADIANS 6283
#define RENDERER_HALF_PI_MILLIRADIANS 1571
#define BIRD_CAMERA_HIT_IMPACT_MS 520
#define BIRD_CAMERA_HIT_CRACK_DURATION_MS 500
#define FLOATING_TEXT_FONT_ID "vector_16_basic"
#define FLOATING_TEXT_SCALE 4
#define PLAYER_SMASH_SLASH_REVEAL_MS 150
#define PLAYER_SMASH_SLASH_LIFETIME_MS 260
#define PLAYER_SMASH_SLASH_SEGMENTS 18
#define PLAYER_SMASH_SLASH_START_ANGLE -1350
#define PLAYER_SMASH_SLASH_SWEEP_ANGLE 2550
#define PLAYER_SMASH_SLASH_RADIUS_X 190
#define PLAYER_SMASH_SLASH_RADIUS_Y 105
#define PLAYER_SMASH_SLASH_GLOW_COLOR 0xf59e0b66u
#define PLAYER_SMASH_SLASH_BODY_COLOR 0xffd166ddu
#define PLAYER_SMASH_SLASH_CORE_COLOR 0xffffffffu
#define PLAYER_HEART_RENDER_WIDTH 48
#define PLAYER_HEARTS_PER_ORBIT_LAYER 6
#define PLAYER_HEART_ORBIT_LAYER_STEP 16
#define PLAYER_BERSERK_AURA_RAYS 12

/* Canonical Little One colors are 0xRRGGBBAA:
 * 0xff0000ff red, 0x00ff00ff green, 0x0000ffff blue,
 * 0x00000033 transparent black with alpha 0x33.
 */

static const unsigned char RENDERER_DIGIT_SEGMENTS[10] = {
        0x3f,
        0x06,
        0x5b,
        0x4f,
        0x66,
        0x6d,
        0x7d,
        0x07,
        0x7f,
        0x6f,
};

static uint8_t rgba_r(uint32_t color) {
    return (uint8_t)((color >> 24) & 0xff);
}

static uint8_t rgba_g(uint32_t color) {
    return (uint8_t)((color >> 16) & 0xff);
}

static uint8_t rgba_b(uint32_t color) {
    return (uint8_t)((color >> 8) & 0xff);
}

static uint8_t rgba_a(uint32_t color) {
    return (uint8_t)(color & 0xff);
}

static uint32_t rgba_pack(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)r << 24)
            | ((uint32_t)g << 16)
            | ((uint32_t)b << 8)
            | (uint32_t)a;
}

static uint16_t rgba_to_rgb565(uint32_t color) {
    uint8_t r = rgba_r(color);
    uint8_t g = rgba_g(color);
    uint8_t b = rgba_b(color);
    uint16_t r5 = (uint16_t)(r >> 3);
    uint16_t g6 = (uint16_t)(g >> 2);
    uint16_t b5 = (uint16_t)(b >> 3);

    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static uint32_t rgb565_to_rgba(uint16_t color) {
    uint8_t r = (uint8_t)((((color >> 11) & 0x1f) * 255) / 31);
    uint8_t g = (uint8_t)((((color >> 5) & 0x3f) * 255) / 63);
    uint8_t b = (uint8_t)(((color & 0x1f) * 255) / 31);

    return rgba_pack(r, g, b, 0xff);
}

/* 32-bit Android path writes RGBA8888 bytes:
 * pixel[0]=R, pixel[1]=G, pixel[2]=B, pixel[3]=A.
 */
static uint32_t framebuffer_rgba8888_read(const uint8_t* pixel) {
    return rgba_pack(pixel[0], pixel[1], pixel[2], pixel[3]);
}

static void framebuffer_rgba8888_write_opaque(uint8_t* pixel, uint32_t color) {
    pixel[0] = rgba_r(color);
    pixel[1] = rgba_g(color);
    pixel[2] = rgba_b(color);
    pixel[3] = 255;
}

static uint32_t renderer_blend_rgba(uint32_t src_color, uint32_t dst_color) {
    uint8_t src_a = rgba_a(src_color);
    uint8_t src_r;
    uint8_t src_g;
    uint8_t src_b;
    uint8_t dst_r;
    uint8_t dst_g;
    uint8_t dst_b;
    uint8_t out_r;
    uint8_t out_g;
    uint8_t out_b;

    if (src_a == 0) {
        return dst_color;
    }

    if (src_a == 255) {
        return (src_color & 0xffffff00) | 0xff;
    }

    src_r = rgba_r(src_color);
    src_g = rgba_g(src_color);
    src_b = rgba_b(src_color);
    dst_r = rgba_r(dst_color);
    dst_g = rgba_g(dst_color);
    dst_b = rgba_b(dst_color);

    out_r = (uint8_t)(((int)src_r * (int)src_a + (int)dst_r * (255 - (int)src_a)) / 255);
    out_g = (uint8_t)(((int)src_g * (int)src_a + (int)dst_g * (255 - (int)src_a)) / 255);
    out_b = (uint8_t)(((int)src_b * (int)src_a + (int)dst_b * (255 - (int)src_a)) / 255);

    return rgba_pack(out_r, out_g, out_b, 0xff);
}

static uint32_t renderer_multiply_alpha(uint32_t color, uint8_t alpha) {
    uint8_t source_alpha;
    uint8_t combined_alpha;

    if (alpha == 255) {
        return color;
    }

    source_alpha = rgba_a(color);
    combined_alpha = (uint8_t)(((int)source_alpha * (int)alpha) / 255);

    return (color & 0xffffff00) | (uint32_t)combined_alpha;
}

static int renderer_normalize_rotation(int rotation) {
    while (rotation > RENDERER_PI_MILLIRADIANS) {
        rotation -= RENDERER_TWO_PI_MILLIRADIANS;
    }

    while (rotation < -RENDERER_PI_MILLIRADIANS) {
        rotation += RENDERER_TWO_PI_MILLIRADIANS;
    }

    return rotation;
}

static int renderer_sin_q16(int rotation) {
    int normalized = renderer_normalize_rotation(rotation);
    int64_t x;
    int64_t x2;
    int64_t result;

    if (normalized > RENDERER_HALF_PI_MILLIRADIANS) {
        normalized = RENDERER_PI_MILLIRADIANS - normalized;
    } else if (normalized < -RENDERER_HALF_PI_MILLIRADIANS) {
        normalized = -RENDERER_PI_MILLIRADIANS - normalized;
    }

    x = ((int64_t)normalized * RENDERER_TRIG_SCALE) / 1000;
    x2 = (x * x) / RENDERER_TRIG_SCALE;
    result = x;
    result -= (((x * x2) / RENDERER_TRIG_SCALE) / 6);
    result += (((((x * x2) / RENDERER_TRIG_SCALE) * x2) / RENDERER_TRIG_SCALE) / 120);
    result -= (((((((x * x2) / RENDERER_TRIG_SCALE) * x2) / RENDERER_TRIG_SCALE) * x2) / RENDERER_TRIG_SCALE) / 5040);

    if (result > RENDERER_TRIG_SCALE) {
        return RENDERER_TRIG_SCALE;
    }
    if (result < -RENDERER_TRIG_SCALE) {
        return -RENDERER_TRIG_SCALE;
    }

    return (int)result;
}

static int renderer_cos_q16(int rotation) {
    return renderer_sin_q16(rotation + RENDERER_HALF_PI_MILLIRADIANS);
}

static int renderer_sprite_tint_enabled = 0;
static uint32_t renderer_sprite_tint_source = 0;
static uint32_t renderer_sprite_tint_target = 0;

static uint32_t renderer_apply_sprite_tint(uint32_t color) {
    if (renderer_sprite_tint_enabled && color == renderer_sprite_tint_source) {
        return renderer_sprite_tint_target;
    }

    return color;
}

static void renderer_write_sprite_pixel(
        Framebuffer* framebuffer,
        int target_x,
        int target_y,
        uint32_t source_color,
        uint8_t alpha
) {
    uint32_t color;

    source_color = renderer_apply_sprite_tint(source_color);
    source_color = renderer_multiply_alpha(source_color, alpha);
    if (rgba_a(source_color) == 0) {
        return;
    }

    if (framebuffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* framebuffer_pixels = (uint16_t*)framebuffer->bits;
        uint16_t* target_pixel = framebuffer_pixels + target_y * framebuffer->stride + target_x;

        if (rgba_a(source_color) == 255) {
            color = source_color;
        } else {
            color = renderer_blend_rgba(source_color, rgb565_to_rgba(*target_pixel));
        }

        *target_pixel = rgba_to_rgb565(color);
        return;
    }

    {
        uint8_t* framebuffer_pixels = (uint8_t*)framebuffer->bits;
        uint8_t* target_pixel =
                framebuffer_pixels + (size_t)target_y * (size_t)framebuffer->stride * 4
                + (size_t)target_x * 4;

        if (rgba_a(source_color) == 255) {
            color = source_color;
        } else {
            color = renderer_blend_rgba(source_color, framebuffer_rgba8888_read(target_pixel));
        }

        framebuffer_rgba8888_write_opaque(target_pixel, color);
    }
}

static int64_t renderer_div_ceil_i64(int64_t numerator, int64_t denominator) {
    return (numerator + denominator - 1) / denominator;
}

static int64_t renderer_max_i64(int64_t left, int64_t right) {
    return left > right ? left : right;
}

static int64_t renderer_min_i64(int64_t left, int64_t right) {
    return left < right ? left : right;
}

void renderer_fill_vertical_gradient(
        Framebuffer* framebuffer,
        uint32_t top_color,
        uint32_t bottom_color
) {
    int height_denominator;
    int top_r;
    int top_g;
    int top_b;
    int top_a;
    int delta_r;
    int delta_g;
    int delta_b;
    int delta_a;

    if (framebuffer == 0 || framebuffer->bits == 0 || framebuffer->width <= 0 || framebuffer->height <= 0) {
        return;
    }

    height_denominator = framebuffer->height > 1 ? framebuffer->height - 1 : 1;
    top_r = rgba_r(top_color);
    top_g = rgba_g(top_color);
    top_b = rgba_b(top_color);
    top_a = rgba_a(top_color);
    delta_r = (int)rgba_r(bottom_color) - top_r;
    delta_g = (int)rgba_g(bottom_color) - top_g;
    delta_b = (int)rgba_b(bottom_color) - top_b;
    delta_a = (int)rgba_a(bottom_color) - top_a;

    if (framebuffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* pixels = (uint16_t*)framebuffer->bits;

        for (int y = 0; y < framebuffer->height; ++y) {
            uint8_t r = (uint8_t)(top_r + (delta_r * y) / height_denominator);
            uint8_t g = (uint8_t)(top_g + (delta_g * y) / height_denominator);
            uint8_t b = (uint8_t)(top_b + (delta_b * y) / height_denominator);
            uint8_t a = (uint8_t)(top_a + (delta_a * y) / height_denominator);
            uint16_t color = rgba_to_rgb565(rgba_pack(r, g, b, a));
            uint16_t* row = pixels + (y * framebuffer->stride);

            for (int x = 0; x < framebuffer->width; ++x) {
                row[x] = color;
            }
        }

        return;
    }

    uint8_t* pixels = (uint8_t*)framebuffer->bits;
    for (int y = 0; y < framebuffer->height; ++y) {
        uint8_t r = (uint8_t)(top_r + (delta_r * y) / height_denominator);
        uint8_t g = (uint8_t)(top_g + (delta_g * y) / height_denominator);
        uint8_t b = (uint8_t)(top_b + (delta_b * y) / height_denominator);
        uint8_t a = (uint8_t)(top_a + (delta_a * y) / height_denominator);
        uint32_t color = rgba_pack(r, g, b, a);
        uint8_t* row = pixels + (size_t)y * (size_t)framebuffer->stride * 4;

        for (int x = 0; x < framebuffer->width; ++x) {
            framebuffer_rgba8888_write_opaque(row + (size_t)x * 4, color);
        }
    }
}

static void renderer_draw_rect(
        ANativeWindow_Buffer* buffer,
        int x,
        int y,
        int width,
        int height,
        uint8_t r,
        uint8_t g,
        uint8_t b
) {
    int min_x = x;
    int min_y = y;
    int max_x = x + width;
    int max_y = y + height;

    if (min_x < 0) {
        min_x = 0;
    }
    if (min_y < 0) {
        min_y = 0;
    }
    if (max_x > buffer->width) {
        max_x = buffer->width;
    }
    if (max_y > buffer->height) {
        max_y = buffer->height;
    }
    if (min_x >= max_x || min_y >= max_y) {
        return;
    }

    if (buffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t color = rgba_to_rgb565(rgba_pack(r, g, b, 0xff));
        uint16_t* pixels = (uint16_t*)buffer->bits;

        for (int draw_y = min_y; draw_y < max_y; ++draw_y) {
            uint16_t* row = pixels + (draw_y * buffer->stride);
            for (int draw_x = min_x; draw_x < max_x; ++draw_x) {
                row[draw_x] = color;
            }
        }

        return;
    }

    uint32_t color = rgba_pack(r, g, b, 0xff);
    uint8_t* pixels = (uint8_t*)buffer->bits;
    for (int draw_y = min_y; draw_y < max_y; ++draw_y) {
        uint8_t* row = pixels + (size_t)draw_y * (size_t)buffer->stride * 4;
        for (int draw_x = min_x; draw_x < max_x; ++draw_x) {
            uint8_t* pixel = row + (size_t)draw_x * 4;
            framebuffer_rgba8888_write_opaque(pixel, color);
        }
    }
}

void renderer_draw_color_rect(
        ANativeWindow_Buffer* buffer,
        int x,
        int y,
        int width,
        int height,
        uint32_t color
) {
    int min_x = x;
    int min_y = y;
    int max_x = x + width;
    int max_y = y + height;
    uint8_t alpha = rgba_a(color);

    if (alpha == 0) {
        return;
    }

    if (alpha == 255) {
        renderer_draw_rect(
                buffer,
                x,
                y,
                width,
                height,
                rgba_r(color),
                rgba_g(color),
                rgba_b(color)
        );
        return;
    }

    if (min_x < 0) {
        min_x = 0;
    }
    if (min_y < 0) {
        min_y = 0;
    }
    if (max_x > buffer->width) {
        max_x = buffer->width;
    }
    if (max_y > buffer->height) {
        max_y = buffer->height;
    }
    if (min_x >= max_x || min_y >= max_y) {
        return;
    }

    if (buffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* pixels = (uint16_t*)buffer->bits;

        for (int draw_y = min_y; draw_y < max_y; ++draw_y) {
            uint16_t* row = pixels + draw_y * buffer->stride;
            for (int draw_x = min_x; draw_x < max_x; ++draw_x) {
                uint32_t blended = renderer_blend_rgba(color, rgb565_to_rgba(row[draw_x]));
                row[draw_x] = rgba_to_rgb565(blended);
            }
        }

        return;
    }

    uint8_t* pixels = (uint8_t*)buffer->bits;
    for (int draw_y = min_y; draw_y < max_y; ++draw_y) {
        uint8_t* row = pixels + (size_t)draw_y * (size_t)buffer->stride * 4;
        for (int draw_x = min_x; draw_x < max_x; ++draw_x) {
            uint8_t* pixel = row + (size_t)draw_x * 4;
            uint32_t blended = renderer_blend_rgba(color, framebuffer_rgba8888_read(pixel));
            framebuffer_rgba8888_write_opaque(pixel, blended);
        }
    }
}

#if LITTLE_ONE_SHOW_WIREFRAMES
static void renderer_draw_circle_outline_points(
        ANativeWindow_Buffer* buffer,
        int center_x,
        int center_y,
        int x,
        int y,
        uint32_t color
) {
    renderer_draw_color_rect(buffer, center_x + x, center_y + y, 1, 1, color);
    renderer_draw_color_rect(buffer, center_x + y, center_y + x, 1, 1, color);
    renderer_draw_color_rect(buffer, center_x - x, center_y + y, 1, 1, color);
    renderer_draw_color_rect(buffer, center_x - y, center_y + x, 1, 1, color);
    renderer_draw_color_rect(buffer, center_x + x, center_y - y, 1, 1, color);
    renderer_draw_color_rect(buffer, center_x + y, center_y - x, 1, 1, color);
    renderer_draw_color_rect(buffer, center_x - x, center_y - y, 1, 1, color);
    renderer_draw_color_rect(buffer, center_x - y, center_y - x, 1, 1, color);
}

static void renderer_draw_hurt_zone_outline(
        ANativeWindow_Buffer* buffer,
        int entity_x,
        int entity_y,
        int entity_width,
        int entity_height,
        const HurtZone* zone
) {
    int center_x;
    int center_y;
    int x;
    int y;
    int decision;

    if (zone == 0 || zone->radius < 0) {
        return;
    }

    center_x = (int)hurt_zone_world_x((int32_t)entity_x, entity_width, zone);
    center_y = (int)hurt_zone_world_y((int32_t)entity_y, entity_height, zone);
    x = zone->radius;
    y = 0;
    decision = 1 - x;

    while (x >= y) {
        renderer_draw_circle_outline_points(
                buffer,
                center_x,
                center_y,
                x,
                y,
                RENDERER_HURT_ZONE_WIREFRAME_COLOR
        );

        y += 1;
        if (decision <= 0) {
            decision += 2 * y + 1;
        } else {
            x -= 1;
            decision += 2 * (y - x) + 1;
        }
    }
}

static void renderer_draw_size_wireframe_rect(
        ANativeWindow_Buffer* buffer,
        int x,
        int y,
        int width,
        int height
) {
    if (width <= 0 || height <= 0) {
        return;
    }

    renderer_draw_color_rect(buffer, x, y, width, 1, RENDERER_SIZE_WIREFRAME_COLOR);
    renderer_draw_color_rect(buffer, x, y + height - 1, width, 1, RENDERER_SIZE_WIREFRAME_COLOR);
    renderer_draw_color_rect(buffer, x, y, 1, height, RENDERER_SIZE_WIREFRAME_COLOR);
    renderer_draw_color_rect(buffer, x + width - 1, y, 1, height, RENDERER_SIZE_WIREFRAME_COLOR);
}

static void renderer_draw_attack_zone_wireframe(
        Framebuffer* buffer,
        int x,
        int y,
        int width,
        int height
) {
    if (width <= 0 || height <= 0) {
        return;
    }

    renderer_draw_color_rect(buffer, x, y, width, 2, RENDERER_ATTACK_ZONE_WIREFRAME_COLOR);
    renderer_draw_color_rect(buffer, x, y + height - 2, width, 2, RENDERER_ATTACK_ZONE_WIREFRAME_COLOR);
    renderer_draw_color_rect(buffer, x, y, 2, height, RENDERER_ATTACK_ZONE_WIREFRAME_COLOR);
    renderer_draw_color_rect(buffer, x + width - 2, y, 2, height, RENDERER_ATTACK_ZONE_WIREFRAME_COLOR);
}
#endif

static void renderer_draw_generated_sprite_fit_alpha(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height,
        SpriteFitMode fit_mode,
        uint8_t alpha
) {
    int64_t dst_left;
    int64_t dst_top;
    int64_t dst_right;
    int64_t dst_bottom;
    int64_t draw_left;
    int64_t draw_top;
    int64_t draw_width;
    int64_t draw_height;
    int64_t draw_right;
    int64_t draw_bottom;
    int64_t clip_left_i64;
    int64_t clip_top_i64;
    int64_t clip_right_i64;
    int64_t clip_bottom_i64;
    int64_t source_scaled_to_dst;
    int64_t dst_scaled_to_source;
    int clip_left;
    int clip_top;
    int clip_right;
    int clip_bottom;

    if (framebuffer == 0
            || framebuffer->bits == 0
            || sprite == 0
            || sprite->pixels == 0
            || sprite->width <= 0
            || sprite->height <= 0
            || dst_width <= 0
            || dst_height <= 0) {
        return;
    }

    dst_left = (int64_t)dst_x;
    dst_top = (int64_t)dst_y;
    dst_right = dst_left + (int64_t)dst_width;
    dst_bottom = dst_top + (int64_t)dst_height;
    draw_left = dst_left;
    draw_top = dst_top;
    draw_width = (int64_t)dst_width;
    draw_height = (int64_t)dst_height;

    if (dst_right <= 0
            || dst_bottom <= 0
            || dst_left >= framebuffer->width
            || dst_top >= framebuffer->height) {
        return;
    }

    if (fit_mode == SPRITE_FIT_CONTAIN || fit_mode == SPRITE_FIT_COVER) {
        source_scaled_to_dst = (int64_t)sprite->width * (int64_t)dst_height;
        dst_scaled_to_source = (int64_t)dst_width * (int64_t)sprite->height;

        if (fit_mode == SPRITE_FIT_CONTAIN) {
            if (source_scaled_to_dst > dst_scaled_to_source) {
                draw_width = (int64_t)dst_width;
                draw_height = ((int64_t)dst_width * (int64_t)sprite->height)
                        / (int64_t)sprite->width;
            } else {
                draw_width = ((int64_t)dst_height * (int64_t)sprite->width)
                        / (int64_t)sprite->height;
                draw_height = (int64_t)dst_height;
            }
        } else {
            if (source_scaled_to_dst > dst_scaled_to_source) {
                draw_width = renderer_div_ceil_i64(
                        (int64_t)dst_height * (int64_t)sprite->width,
                        (int64_t)sprite->height
                );
                draw_height = (int64_t)dst_height;
            } else {
                draw_width = (int64_t)dst_width;
                draw_height = renderer_div_ceil_i64(
                        (int64_t)dst_width * (int64_t)sprite->height,
                        (int64_t)sprite->width
                );
            }
        }

        if (draw_width <= 0) {
            draw_width = 1;
        }
        if (draw_height <= 0) {
            draw_height = 1;
        }

        draw_left = dst_left + ((int64_t)dst_width - draw_width) / 2;
        draw_top = dst_top + ((int64_t)dst_height - draw_height) / 2;
    }

    draw_right = draw_left + draw_width;
    draw_bottom = draw_top + draw_height;

    clip_left_i64 = renderer_max_i64(renderer_max_i64(0, dst_left), draw_left);
    clip_top_i64 = renderer_max_i64(renderer_max_i64(0, dst_top), draw_top);
    clip_right_i64 = renderer_min_i64(
            renderer_min_i64((int64_t)framebuffer->width, dst_right),
            draw_right
    );
    clip_bottom_i64 = renderer_min_i64(
            renderer_min_i64((int64_t)framebuffer->height, dst_bottom),
            draw_bottom
    );

    if (clip_left_i64 >= clip_right_i64 || clip_top_i64 >= clip_bottom_i64) {
        return;
    }

    clip_left = (int)clip_left_i64;
    clip_top = (int)clip_top_i64;
    clip_right = (int)clip_right_i64;
    clip_bottom = (int)clip_bottom_i64;

    if (framebuffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* framebuffer_pixels = (uint16_t*)framebuffer->bits;

        for (int target_y = clip_top; target_y < clip_bottom; ++target_y) {
            int64_t dst_local_y = (int64_t)target_y - draw_top;
            int source_y = (int)(dst_local_y * (int64_t)sprite->height / draw_height);
            uint16_t* target_row = framebuffer_pixels + target_y * framebuffer->stride;
            const uint32_t* source_row =
                    sprite->pixels + (size_t)source_y * (size_t)sprite->width;

            for (int target_x = clip_left; target_x < clip_right; ++target_x) {
                int64_t dst_local_x = (int64_t)target_x - draw_left;
                int source_x = (int)(dst_local_x * (int64_t)sprite->width / draw_width);
                uint32_t source_color = renderer_apply_sprite_tint(source_row[source_x]);
                source_color = renderer_multiply_alpha(source_color, alpha);
                uint8_t source_alpha = rgba_a(source_color);
                uint32_t color;

                if (source_alpha == 0) {
                    continue;
                }

                if (source_alpha == 255) {
                    color = source_color;
                } else {
                    color = renderer_blend_rgba(
                            source_color,
                            rgb565_to_rgba(target_row[target_x])
                    );
                }

                target_row[target_x] = rgba_to_rgb565(color);
            }
        }

        return;
    }

    uint8_t* framebuffer_pixels = (uint8_t*)framebuffer->bits;
    for (int target_y = clip_top; target_y < clip_bottom; ++target_y) {
        int64_t dst_local_y = (int64_t)target_y - draw_top;
        int source_y = (int)(dst_local_y * (int64_t)sprite->height / draw_height);
        uint8_t* target_row =
                framebuffer_pixels + (size_t)target_y * (size_t)framebuffer->stride * 4;
        const uint32_t* source_row =
                sprite->pixels + (size_t)source_y * (size_t)sprite->width;

        for (int target_x = clip_left; target_x < clip_right; ++target_x) {
            int64_t dst_local_x = (int64_t)target_x - draw_left;
            int source_x = (int)(dst_local_x * (int64_t)sprite->width / draw_width);
            uint32_t source_color = renderer_apply_sprite_tint(source_row[source_x]);
            source_color = renderer_multiply_alpha(source_color, alpha);
            uint8_t source_alpha = rgba_a(source_color);
            uint32_t color;

            if (source_alpha == 0) {
                continue;
            }

            if (source_alpha == 255) {
                color = source_color;
            } else {
                uint8_t* target_pixel = target_row + (size_t)target_x * 4;
                color = renderer_blend_rgba(
                        source_color,
                        framebuffer_rgba8888_read(target_pixel)
                );
            }

            framebuffer_rgba8888_write_opaque(target_row + (size_t)target_x * 4, color);
        }
    }
}

void renderer_draw_generated_sprite_fit(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height,
        SpriteFitMode fit_mode
) {
    renderer_draw_generated_sprite_fit_alpha(
            framebuffer,
            sprite,
            dst_x,
            dst_y,
            dst_width,
            dst_height,
            fit_mode,
            255
    );
}

void renderer_draw_generated_sprite_region_scaled(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int src_x,
        int src_y,
        int src_width,
        int src_height,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height
) {
    int clip_left;
    int clip_top;
    int clip_right;
    int clip_bottom;

    if (framebuffer == 0
            || framebuffer->bits == 0
            || sprite == 0
            || sprite->pixels == 0
            || sprite->width <= 0
            || sprite->height <= 0
            || src_width <= 0
            || src_height <= 0
            || dst_width <= 0
            || dst_height <= 0) {
        return;
    }

    if (src_x < 0) {
        src_width += src_x;
        src_x = 0;
    }
    if (src_y < 0) {
        src_height += src_y;
        src_y = 0;
    }
    if (src_x + src_width > sprite->width) {
        src_width = sprite->width - src_x;
    }
    if (src_y + src_height > sprite->height) {
        src_height = sprite->height - src_y;
    }
    if (src_width <= 0 || src_height <= 0) {
        return;
    }

    clip_left = dst_x < 0 ? 0 : dst_x;
    clip_top = dst_y < 0 ? 0 : dst_y;
    clip_right = dst_x + dst_width;
    clip_bottom = dst_y + dst_height;
    if (clip_right > framebuffer->width) {
        clip_right = framebuffer->width;
    }
    if (clip_bottom > framebuffer->height) {
        clip_bottom = framebuffer->height;
    }
    if (clip_left >= clip_right || clip_top >= clip_bottom) {
        return;
    }

    for (int target_y = clip_top; target_y < clip_bottom; ++target_y) {
        int dst_local_y = target_y - dst_y;
        int source_y = src_y + (int)((int64_t)dst_local_y * (int64_t)src_height / (int64_t)dst_height);
        const uint32_t* source_row = sprite->pixels + (size_t)source_y * (size_t)sprite->width;

        for (int target_x = clip_left; target_x < clip_right; ++target_x) {
            int dst_local_x = target_x - dst_x;
            int source_x = src_x + (int)((int64_t)dst_local_x * (int64_t)src_width / (int64_t)dst_width);

            renderer_write_sprite_pixel(
                    framebuffer,
                    target_x,
                    target_y,
                    source_row[source_x],
                    255
            );
        }
    }
}

typedef struct {
    int x;
    int y;
    int width;
    int height;
} RendererSpriteRect;

static RendererSpriteRect renderer_sprite_fit_rect(
        const GeneratedSprite* sprite,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height,
        SpriteFitMode fit_mode
) {
    RendererSpriteRect rect;

    rect.x = dst_x;
    rect.y = dst_y;
    rect.width = dst_width;
    rect.height = dst_height;

    if (sprite == 0
            || sprite->width <= 0
            || sprite->height <= 0
            || dst_width <= 0
            || dst_height <= 0) {
        rect.width = 0;
        rect.height = 0;
        return rect;
    }

    if (fit_mode == SPRITE_FIT_CONTAIN || fit_mode == SPRITE_FIT_COVER) {
        int64_t source_scaled_to_dst = (int64_t)sprite->width * (int64_t)dst_height;
        int64_t dst_scaled_to_source = (int64_t)dst_width * (int64_t)sprite->height;
        int64_t draw_width;
        int64_t draw_height;

        if (fit_mode == SPRITE_FIT_CONTAIN) {
            if (source_scaled_to_dst > dst_scaled_to_source) {
                draw_width = (int64_t)dst_width;
                draw_height = ((int64_t)dst_width * (int64_t)sprite->height)
                        / (int64_t)sprite->width;
            } else {
                draw_width = ((int64_t)dst_height * (int64_t)sprite->width)
                        / (int64_t)sprite->height;
                draw_height = (int64_t)dst_height;
            }
        } else {
            if (source_scaled_to_dst > dst_scaled_to_source) {
                draw_width = renderer_div_ceil_i64(
                        (int64_t)dst_height * (int64_t)sprite->width,
                        (int64_t)sprite->height
                );
                draw_height = (int64_t)dst_height;
            } else {
                draw_width = (int64_t)dst_width;
                draw_height = renderer_div_ceil_i64(
                        (int64_t)dst_width * (int64_t)sprite->height,
                        (int64_t)sprite->width
                );
            }
        }

        if (draw_width <= 0) {
            draw_width = 1;
        }
        if (draw_height <= 0) {
            draw_height = 1;
        }

        rect.width = (int)draw_width;
        rect.height = (int)draw_height;
        rect.x = dst_x + (dst_width - rect.width) / 2;
        rect.y = dst_y + (dst_height - rect.height) / 2;
    }

    return rect;
}

static void renderer_transform_corner(
        int64_t local_x,
        int64_t local_y,
        int draw_width,
        int draw_height,
        const GeneratedSprite* sprite,
        int16_t scale_x,
        int16_t scale_y,
        int cos_rotation,
        int sin_rotation,
        int64_t *out_x,
        int64_t *out_y
) {
    int64_t scaled_x = (local_x * (int64_t)draw_width * (int64_t)scale_x)
            / ((int64_t)sprite->width * 1000);
    int64_t scaled_y = (local_y * (int64_t)draw_height * (int64_t)scale_y)
            / ((int64_t)sprite->height * 1000);

    *out_x = (scaled_x * cos_rotation - scaled_y * sin_rotation) / RENDERER_TRIG_SCALE;
    *out_y = (scaled_x * sin_rotation + scaled_y * cos_rotation) / RENDERER_TRIG_SCALE;
}

static void renderer_draw_generated_sprite_transformed(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int draw_x,
        int draw_y,
        int draw_width,
        int draw_height,
        int16_t scale_x,
        int16_t scale_y,
        int16_t rotation,
        uint8_t alpha
) {
    int cos_rotation;
    int sin_rotation;
    int64_t pivot_x;
    int64_t pivot_y;
    int64_t corners_x[4];
    int64_t corners_y[4];
    int64_t min_x;
    int64_t min_y;
    int64_t max_x;
    int64_t max_y;
    int clip_left;
    int clip_top;
    int clip_right;
    int clip_bottom;

    if (framebuffer == 0
            || framebuffer->bits == 0
            || sprite == 0
            || sprite->pixels == 0
            || sprite->width <= 0
            || sprite->height <= 0
            || draw_width <= 0
            || draw_height <= 0
            || scale_x == 0
            || scale_y == 0
            || alpha == 0) {
        return;
    }

    cos_rotation = renderer_cos_q16(rotation);
    sin_rotation = renderer_sin_q16(rotation);
    pivot_x = (int64_t)draw_x
            + ((int64_t)sprite->pivot_x * (int64_t)draw_width) / (int64_t)sprite->width;
    pivot_y = (int64_t)draw_y
            + ((int64_t)sprite->pivot_y * (int64_t)draw_height) / (int64_t)sprite->height;

    renderer_transform_corner(
            -sprite->pivot_x,
            -sprite->pivot_y,
            draw_width,
            draw_height,
            sprite,
            scale_x,
            scale_y,
            cos_rotation,
            sin_rotation,
            corners_x,
            corners_y
    );
    renderer_transform_corner(
            (int64_t)sprite->width - sprite->pivot_x,
            -sprite->pivot_y,
            draw_width,
            draw_height,
            sprite,
            scale_x,
            scale_y,
            cos_rotation,
            sin_rotation,
            corners_x + 1,
            corners_y + 1
    );
    renderer_transform_corner(
            -sprite->pivot_x,
            (int64_t)sprite->height - sprite->pivot_y,
            draw_width,
            draw_height,
            sprite,
            scale_x,
            scale_y,
            cos_rotation,
            sin_rotation,
            corners_x + 2,
            corners_y + 2
    );
    renderer_transform_corner(
            (int64_t)sprite->width - sprite->pivot_x,
            (int64_t)sprite->height - sprite->pivot_y,
            draw_width,
            draw_height,
            sprite,
            scale_x,
            scale_y,
            cos_rotation,
            sin_rotation,
            corners_x + 3,
            corners_y + 3
    );

    min_x = corners_x[0];
    max_x = corners_x[0];
    min_y = corners_y[0];
    max_y = corners_y[0];
    for (int corner_index = 1; corner_index < 4; ++corner_index) {
        if (corners_x[corner_index] < min_x) {
            min_x = corners_x[corner_index];
        }
        if (corners_x[corner_index] > max_x) {
            max_x = corners_x[corner_index];
        }
        if (corners_y[corner_index] < min_y) {
            min_y = corners_y[corner_index];
        }
        if (corners_y[corner_index] > max_y) {
            max_y = corners_y[corner_index];
        }
    }

    clip_left = (int)renderer_max_i64(0, pivot_x + min_x - 2);
    clip_top = (int)renderer_max_i64(0, pivot_y + min_y - 2);
    clip_right = (int)renderer_min_i64((int64_t)framebuffer->width, pivot_x + max_x + 2);
    clip_bottom = (int)renderer_min_i64((int64_t)framebuffer->height, pivot_y + max_y + 2);

    if (clip_left >= clip_right || clip_top >= clip_bottom) {
        return;
    }

    for (int target_y = clip_top; target_y < clip_bottom; ++target_y) {
        for (int target_x = clip_left; target_x < clip_right; ++target_x) {
            int64_t dx = (int64_t)target_x - pivot_x;
            int64_t dy = (int64_t)target_y - pivot_y;
            int64_t unrotated_x = (dx * cos_rotation + dy * sin_rotation)
                    / RENDERER_TRIG_SCALE;
            int64_t unrotated_y = (-dx * sin_rotation + dy * cos_rotation)
                    / RENDERER_TRIG_SCALE;
            int64_t source_local_x =
                    (unrotated_x * 1000 * (int64_t)sprite->width)
                    / ((int64_t)scale_x * (int64_t)draw_width);
            int64_t source_local_y =
                    (unrotated_y * 1000 * (int64_t)sprite->height)
                    / ((int64_t)scale_y * (int64_t)draw_height);
            int source_x = (int)(source_local_x + sprite->pivot_x);
            int source_y = (int)(source_local_y + sprite->pivot_y);

            if (source_x < 0 || source_y < 0 || source_x >= sprite->width || source_y >= sprite->height) {
                continue;
            }

            renderer_write_sprite_pixel(
                    framebuffer,
                    target_x,
                    target_y,
                    sprite->pixels[(size_t)source_y * (size_t)sprite->width + (size_t)source_x],
                    alpha
            );
        }
    }
}

static void renderer_draw_generated_sprite_scale_only(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        const RendererSpriteRect* rect,
        int16_t scale_x,
        int16_t scale_y,
        uint8_t alpha
) {
    int pivot_x;
    int pivot_y;
    int scaled_width;
    int scaled_height;
    int scaled_pivot_x;
    int scaled_pivot_y;

    if (rect == 0 || rect->width <= 0 || rect->height <= 0) {
        return;
    }

    pivot_x = (int)(((int64_t)sprite->pivot_x * (int64_t)rect->width) / (int64_t)sprite->width);
    pivot_y = (int)(((int64_t)sprite->pivot_y * (int64_t)rect->height) / (int64_t)sprite->height);
    scaled_width = (int)(((int64_t)rect->width * (int64_t)scale_x) / 1000);
    scaled_height = (int)(((int64_t)rect->height * (int64_t)scale_y) / 1000);
    scaled_pivot_x = (int)(((int64_t)pivot_x * (int64_t)scale_x) / 1000);
    scaled_pivot_y = (int)(((int64_t)pivot_y * (int64_t)scale_y) / 1000);

    if (scaled_width < 0) {
        scaled_width = -scaled_width;
    }
    if (scaled_height < 0) {
        scaled_height = -scaled_height;
    }

    renderer_draw_generated_sprite_fit_alpha(
            framebuffer,
            sprite,
            rect->x + pivot_x - scaled_pivot_x,
            rect->y + pivot_y - scaled_pivot_y,
            scaled_width,
            scaled_height,
            SPRITE_FIT_STRETCH,
            alpha
    );
}

void renderer_draw_generated_sprite_fit_ex(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height,
        SpriteFitMode fit_mode,
        int16_t scale_x,
        int16_t scale_y,
        int16_t rotation,
        uint8_t alpha
) {
    RendererSpriteRect rect;

    if (framebuffer == 0
            || framebuffer->bits == 0
            || sprite == 0
            || sprite->width <= 0
            || sprite->height <= 0
            || dst_width <= 0
            || dst_height <= 0
            || scale_x == 0
            || scale_y == 0
            || alpha == 0) {
        return;
    }

    if (scale_x == 1000 && scale_y == 1000 && rotation == 0) {
        renderer_draw_generated_sprite_fit_alpha(
                framebuffer,
                sprite,
                dst_x,
                dst_y,
                dst_width,
                dst_height,
                fit_mode,
                alpha
        );
        return;
    }

    rect = renderer_sprite_fit_rect(
            sprite,
            dst_x,
            dst_y,
            dst_width,
            dst_height,
            fit_mode
    );

    if (rotation == 0) {
        renderer_draw_generated_sprite_scale_only(
                framebuffer,
                sprite,
                &rect,
                scale_x,
                scale_y,
                alpha
        );
        return;
    }

    renderer_draw_generated_sprite_transformed(
            framebuffer,
            sprite,
            rect.x,
            rect.y,
            rect.width,
            rect.height,
            scale_x,
            scale_y,
            rotation,
            alpha
    );
}

void blit_sprite(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int32_t x,
        int32_t y
) {
    blit_sprite_ex(framebuffer, sprite, x, y, 1000, 1000, 0, 255);
}

void blit_sprite_ex(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int32_t x,
        int32_t y,
        int16_t scale_x,
        int16_t scale_y,
        int16_t rotation,
        uint8_t alpha
) {
    if (sprite == 0) {
        return;
    }

    renderer_draw_generated_sprite_fit_ex(
            framebuffer,
            sprite,
            (int)x,
            (int)y,
            sprite->width,
            sprite->height,
            SPRITE_FIT_STRETCH,
            scale_x,
            scale_y,
            rotation,
            alpha
    );
}

void renderer_draw_generated_sprite_scaled(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height
) {
    renderer_draw_generated_sprite_fit(
            framebuffer,
            sprite,
            dst_x,
            dst_y,
            dst_width,
            dst_height,
            SPRITE_FIT_STRETCH
    );
}

static int renderer_count_digits(int value) {
    int digits = 1;

    while (value >= 10) {
        value /= 10;
        digits += 1;
    }

    return digits;
}

static void renderer_draw_digit_segment(
        ANativeWindow_Buffer* buffer,
        int origin_x,
        int origin_y,
        int segment
) {
    int half_height = RENDERER_DIGIT_HEIGHT / 2;

    if (segment == 0) {
        renderer_draw_rect(
                buffer,
                origin_x + RENDERER_DIGIT_THICKNESS,
                origin_y,
                RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS * 2,
                RENDERER_DIGIT_THICKNESS,
                255,
                255,
                255
        );
    } else if (segment == 1) {
        renderer_draw_rect(
                buffer,
                origin_x + RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS,
                origin_y + RENDERER_DIGIT_THICKNESS,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS,
                255,
                255,
                255
        );
    } else if (segment == 2) {
        renderer_draw_rect(
                buffer,
                origin_x + RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS,
                origin_y + half_height,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS,
                255,
                255,
                255
        );
    } else if (segment == 3) {
        renderer_draw_rect(
                buffer,
                origin_x + RENDERER_DIGIT_THICKNESS,
                origin_y + RENDERER_DIGIT_HEIGHT - RENDERER_DIGIT_THICKNESS,
                RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS * 2,
                RENDERER_DIGIT_THICKNESS,
                255,
                255,
                255
        );
    } else if (segment == 4) {
        renderer_draw_rect(
                buffer,
                origin_x,
                origin_y + half_height,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS,
                255,
                255,
                255
        );
    } else if (segment == 5) {
        renderer_draw_rect(
                buffer,
                origin_x,
                origin_y + RENDERER_DIGIT_THICKNESS,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS,
                255,
                255,
                255
        );
    } else if (segment == 6) {
        renderer_draw_rect(
                buffer,
                origin_x + RENDERER_DIGIT_THICKNESS,
                origin_y + half_height - RENDERER_DIGIT_THICKNESS / 2,
                RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS * 2,
                RENDERER_DIGIT_THICKNESS,
                255,
                255,
                255
        );
    }
}

static void renderer_draw_digit(
        ANativeWindow_Buffer* buffer,
        int origin_x,
        int origin_y,
        int digit
) {
    if (digit < 0 || digit > 9) {
        return;
    }

    unsigned char segments = RENDERER_DIGIT_SEGMENTS[digit];
    for (int segment = 0; segment < 7; ++segment) {
        if ((segments & (1 << segment)) != 0) {
            renderer_draw_digit_segment(buffer, origin_x, origin_y, segment);
        }
    }
}

static void renderer_draw_number(
        ANativeWindow_Buffer* buffer,
        int origin_x,
        int origin_y,
        int value
) {
    int digits[10];
    int digit_count = 0;

    if (value < 0) {
        value = 0;
    }

    do {
        digits[digit_count] = value % 10;
        value /= 10;
        digit_count += 1;
    } while (value > 0 && digit_count < 10);

    for (int digit_index = 0; digit_index < digit_count; ++digit_index) {
        int draw_index = digit_count - digit_index - 1;
        int digit_x = origin_x + digit_index * (RENDERER_DIGIT_WIDTH + RENDERER_DIGIT_SPACING);

        renderer_draw_digit(buffer, digit_x, origin_y, digits[draw_index]);
    }
}

static void renderer_draw_parallax_tile(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int x,
        int y,
        int mirrored
) {
    if (!mirrored) {
        renderer_draw_generated_sprite_scaled(
                framebuffer,
                sprite,
                x,
                y,
                sprite->width,
                sprite->height
        );
        return;
    }

    for (int target_y = y; target_y < y + sprite->height; ++target_y) {
        int source_y;
        const uint32_t* source_row;

        if (target_y < 0 || target_y >= framebuffer->height) {
            continue;
        }

        source_y = target_y - y;
        source_row = sprite->pixels + (size_t)source_y * (size_t)sprite->width;

        for (int target_x = x; target_x < x + sprite->width; ++target_x) {
            int source_x;

            if (target_x < 0 || target_x >= framebuffer->width) {
                continue;
            }

            source_x = sprite->width - 1 - (target_x - x);
            renderer_write_sprite_pixel(
                    framebuffer,
                    target_x,
                    target_y,
                    source_row[source_x],
                    255
            );
        }
    }
}

static void renderer_draw_parallax_layer(
        Framebuffer* framebuffer,
        const ParallaxLayerConfig* layer,
        int32_t world_scroll_x,
        int32_t gameplay_ground_y,
        int32_t shake_x,
        int32_t shake_y
) {
    const GeneratedSprite* sprite;
    int32_t layer_scroll;
    int32_t screen_x;
    int32_t layer_y;
    int tile_w;
    int tile_h;
    int start_x;
    int start_y;
    int start_tile_index;

    if (framebuffer == 0
            || layer == 0
            || !layer->enabled
            || layer->sprite_id == 0
            || layer->scroll_den <= 0) {
        return;
    }

    sprite = generated_sprite_get_by_id(layer->sprite_id);
    if (sprite == 0 || sprite->pixels == 0 || sprite->width <= 0 || sprite->height <= 0) {
        return;
    }

    layer_scroll = (int32_t)(((int64_t)world_scroll_x * (int64_t)layer->scroll_num)
            / (int64_t)layer->scroll_den);
    screen_x = (int32_t)layer->x_offset - layer_scroll + shake_x;
    tile_w = sprite->width;
    tile_h = sprite->height;
    layer_y = gameplay_ground_y - (int32_t)layer->y - tile_h + shake_y;

    if (!layer->repeat_x && !layer->repeat_y) {
        renderer_draw_parallax_tile(framebuffer, sprite, screen_x, layer_y, 0);
        return;
    }

    start_x = screen_x;
    start_tile_index = 0;
    if (layer->repeat_x) {
        start_x = screen_x % tile_w;
        if (start_x > 0) {
            start_x -= tile_w;
        }
        start_tile_index = (start_x - screen_x) / tile_w;
    }

    start_y = layer_y;
    if (layer->repeat_y) {
        while (start_y > 0) {
            start_y -= tile_h;
        }
    }

    for (int y = start_y; y < layer_y + tile_h; y += tile_h) {
        if (layer->repeat_x) {
            for (int x = start_x; x < framebuffer->width; x += tile_w) {
                int tile_index = start_tile_index + (x - start_x) / tile_w;
                int mirrored = layer->mirror_x && ((tile_index & 1) != 0);
                renderer_draw_parallax_tile(framebuffer, sprite, x, y, mirrored);
            }
        } else {
            renderer_draw_parallax_tile(framebuffer, sprite, screen_x, y, 0);
        }

        if (!layer->repeat_y) {
            break;
        }
    }
}

void render_background(
        Framebuffer* framebuffer,
        const BackgroundConfig* background,
        int32_t world_scroll_x,
        int32_t gameplay_ground_y,
        int32_t shake_x,
        int32_t shake_y
) {
    int layer_count;

    if (framebuffer == 0 || background == 0) {
        return;
    }

    renderer_fill_vertical_gradient(
            framebuffer,
            background->sky.top_color,
            background->sky.bottom_color
    );

    layer_count = background->layer_count;
    if (layer_count < 0) {
        layer_count = 0;
    }
    if (layer_count > LITTLE_ONE_MAX_PARALLAX_LAYERS) {
        layer_count = LITTLE_ONE_MAX_PARALLAX_LAYERS;
    }

    for (int layer_index = 0; layer_index < layer_count; ++layer_index) {
        renderer_draw_parallax_layer(
                framebuffer,
                background->layers + layer_index,
                world_scroll_x,
                gameplay_ground_y,
                shake_x,
                shake_y
        );
    }
}

static int32_t renderer_gameplay_ground_y(const GameState* game) {
    return (int32_t)game->screenHeight - (int32_t)LITTLE_ONE_GROUND_BOTTOM_MARGIN_PX;
}

static void renderer_draw_ground_fallback_line(
        Framebuffer* framebuffer,
        int32_t gameplay_ground_y
) {
    renderer_draw_rect(
            framebuffer,
            0,
            gameplay_ground_y,
            framebuffer->width,
            RENDERER_GROUND_LINE_HEIGHT,
            255,
            255,
            255
    );
}

void render_ground(
        Framebuffer* framebuffer,
        const GroundVisualConfig* ground,
        int32_t world_scroll_x,
        int32_t gameplay_ground_y,
        int32_t shake_x,
        int32_t shake_y
) {
    const GeneratedSprite* sprite;
    int32_t screen_x;
    int32_t start_x;
    int32_t draw_y;
    int tile_w;
    int draw_h;

    if (framebuffer == 0 || framebuffer->bits == 0) {
        return;
    }

    if (ground == 0 || !ground->enabled || ground->sprite_id == 0) {
        renderer_draw_ground_fallback_line(framebuffer, gameplay_ground_y + shake_y);
        return;
    }

    sprite = generated_sprite_get_by_id(ground->sprite_id);
    if (sprite == 0 || sprite->pixels == 0 || sprite->width <= 0 || sprite->height <= 0) {
        renderer_draw_ground_fallback_line(framebuffer, gameplay_ground_y + shake_y);
        return;
    }

    tile_w = sprite->width;
    draw_h = ground->height > 0 ? ground->height : sprite->height;
    draw_y = (ground->y != 0 ? ground->y : gameplay_ground_y) + shake_y;
    screen_x = (int32_t)ground->x_offset - world_scroll_x + shake_x;

    if (!ground->repeat_x) {
        renderer_draw_generated_sprite_scaled(
                framebuffer,
                sprite,
                screen_x,
                draw_y,
                tile_w,
                draw_h
        );
        return;
    }

    start_x = screen_x % tile_w;
    if (start_x > 0) {
        start_x -= tile_w;
    }

    for (int32_t x = start_x; x < framebuffer->width; x += tile_w) {
        renderer_draw_generated_sprite_scaled(
                framebuffer,
                sprite,
                x,
                draw_y,
                tile_w,
                draw_h
        );
    }
}

#if LITTLE_ONE_SHOW_WIREFRAMES
static void renderer_draw_ground_debug_line(
        Framebuffer* framebuffer,
        int32_t gameplay_ground_y
) {
    renderer_draw_color_rect(
            framebuffer,
            0,
            gameplay_ground_y,
            framebuffer->width,
            1,
            RENDERER_GROUND_DEBUG_COLOR
    );
}
#endif

static void renderer_draw_entities(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y,
        int draw_dead
) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        const Entity* entity = game->entities + entity_index;
        uint32_t color = 0xffffffff;
        SpriteId sprite_id = SPRITE_NONE;
        int entity_width;
        int entity_height;
        const GeneratedSprite* sprite;
        AnimationImpact impact;

        if (!entity->active || entity->dead != draw_dead) {
            continue;
        }

        impact = animation_impact_default();
        sprite = 0;

        if (entity->type == ENTITY_THREAT && entity->config != 0) {
            color = entity->config->visual.color;
            sprite_id = entity->config->visual.sprite_id;
            impact = entity_animation_get_impact(&entity->animation);
            sprite = generated_sprite_get(sprite_id);
        } else if (entity->type == ENTITY_POWERUP && entity->powerup_config != 0) {
            const PowerupTuning* tuning = powerup_tuning_get();

            color = entity->powerup_config->type == POWERUP_HP
                    ? 0x55ff70ff : 0xff7a18ff;
            sprite = generated_sprite_get_by_id(entity->powerup_config->sprite_id);
            if (entity->grounded && tuning->idle_cycle_ms > 0) {
                int phase = (entity->ground_age_ms % tuning->idle_cycle_ms)
                        * RENDERER_TWO_PI_MILLIRADIANS / tuning->idle_cycle_ms;
                int wave = renderer_sin_q16(phase);

                impact.offset_y = (int16_t)(-tuning->idle_bob_px / 2
                        - wave * tuning->idle_bob_px / (2 * RENDERER_TRIG_SCALE));
                impact.scale_x = impact.scale_y = (int16_t)(1000
                        + wave * tuning->idle_pulse_permille / RENDERER_TRIG_SCALE);
                impact.rotation = (int16_t)(
                        renderer_sin_q16(phase + RENDERER_HALF_PI_MILLIRADIANS)
                        * 70 / RENDERER_TRIG_SCALE
                );
            } else {
                impact.rotation = (int16_t)(
                        (entity->age_ms * 5) % RENDERER_TWO_PI_MILLIRADIANS
                        - RENDERER_PI_MILLIRADIANS
                );
            }
        }

        entity_width = entity_get_width(entity);
        entity_height = entity_get_height(entity);
        if (sprite != 0) {
            renderer_draw_generated_sprite_fit_ex(
                    buffer,
                    sprite,
                    (int)entity->x + shake_x + impact.offset_x,
                    (int)entity->y + shake_y + impact.offset_y,
                    entity_width,
                    entity_height,
                    DEFAULT_SPRITE_FIT_MODE,
                    impact.scale_x,
                    impact.scale_y,
                    impact.rotation,
                    impact.alpha
            );
            continue;
        }

        renderer_draw_color_rect(
                buffer,
                (int)entity->x + shake_x + impact.offset_x,
                (int)entity->y + shake_y + impact.offset_y,
                entity_width,
                entity_height,
                color
        );
    }
}

static void renderer_draw_effect_line(
        Framebuffer* framebuffer,
        int x0,
        int y0,
        int x1,
        int y1,
        int width,
        uint32_t color
) {
    int dx;
    int sx;
    int dy;
    int sy;
    int error;
    int half_width;

    if (framebuffer == 0 || width <= 0 || (color & 0xffu) == 0u) {
        return;
    }

    dx = x1 > x0 ? x1 - x0 : x0 - x1;
    sx = x0 < x1 ? 1 : -1;
    dy = y1 > y0 ? y0 - y1 : y1 - y0;
    sy = y0 < y1 ? 1 : -1;
    error = dx + dy;
    half_width = width / 2;

    for (;;) {
        renderer_draw_color_rect(
                framebuffer,
                x0 - half_width,
                y0 - half_width,
                width,
                width,
                color
        );
        if (x0 == x1 && y0 == y1) {
            break;
        }

        {
            int doubled_error = error * 2;

            if (doubled_error >= dy) {
                error += dy;
                x0 += sx;
            }
            if (doubled_error <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    }
}

static void renderer_draw_player_smash_slash(
        Framebuffer* framebuffer,
        const GameState* game,
        const EntityVisualConfig* visual,
        const PlayerSwordPoseConfig* sword_pose,
        int sprite_x,
        int sprite_y,
        int32_t shake_x,
        int32_t shake_y,
        AnimationImpact impact
) {
    int time_ms;
    int reveal_ms;
    int visible_segments;
    int effect_alpha;
    int center_x;
    int center_y;

    if (game->playerAnimation.slot != ENTITY_ANIM_SMASH) {
        return;
    }

    time_ms = game->playerAnimation.time_ms;
    if (time_ms < 0 || time_ms >= PLAYER_SMASH_SLASH_LIFETIME_MS) {
        return;
    }

    reveal_ms = time_ms;
    if (reveal_ms > PLAYER_SMASH_SLASH_REVEAL_MS) {
        reveal_ms = PLAYER_SMASH_SLASH_REVEAL_MS;
    }
    visible_segments = (PLAYER_SMASH_SLASH_SEGMENTS * reveal_ms
            + PLAYER_SMASH_SLASH_REVEAL_MS - 1) / PLAYER_SMASH_SLASH_REVEAL_MS;
    if (visible_segments <= 0) {
        return;
    }

    effect_alpha = 255;
    if (time_ms > PLAYER_SMASH_SLASH_REVEAL_MS) {
        effect_alpha = 255
                * (PLAYER_SMASH_SLASH_LIFETIME_MS - time_ms)
                / (PLAYER_SMASH_SLASH_LIFETIME_MS - PLAYER_SMASH_SLASH_REVEAL_MS);
    }

    /* Keep the whole crescent in front of the player, matching the attack zone. */
    center_x = sprite_x + sword_pose->offset_x + shake_x + impact.offset_x;
    center_y = sprite_y + sword_pose->offset_y - visual->height / 5
            + shake_y + impact.offset_y;

    for (int segment = 0; segment < visible_segments; ++segment) {
        int angle_0 = PLAYER_SMASH_SLASH_START_ANGLE
                + PLAYER_SMASH_SLASH_SWEEP_ANGLE * segment / PLAYER_SMASH_SLASH_SEGMENTS;
        int angle_1 = PLAYER_SMASH_SLASH_START_ANGLE
                + PLAYER_SMASH_SLASH_SWEEP_ANGLE * (segment + 1) / PLAYER_SMASH_SLASH_SEGMENTS;
        int x0 = center_x
                + renderer_cos_q16(angle_0) * PLAYER_SMASH_SLASH_RADIUS_X / RENDERER_TRIG_SCALE;
        int y0 = center_y
                + renderer_sin_q16(angle_0) * PLAYER_SMASH_SLASH_RADIUS_Y / RENDERER_TRIG_SCALE;
        int x1 = center_x
                + renderer_cos_q16(angle_1) * PLAYER_SMASH_SLASH_RADIUS_X / RENDERER_TRIG_SCALE;
        int y1 = center_y
                + renderer_sin_q16(angle_1) * PLAYER_SMASH_SLASH_RADIUS_Y / RENDERER_TRIG_SCALE;
        int taper = (segment + 1) * 1000 / PLAYER_SMASH_SLASH_SEGMENTS;
        int segment_alpha = effect_alpha * (90 + taper * 165 / 1000) / 255;
        int body_width = 7 + taper * 9 / 1000;

        renderer_draw_effect_line(
                framebuffer,
                x0,
                y0,
                x1,
                y1,
                body_width + 14,
                renderer_multiply_alpha(
                        PLAYER_SMASH_SLASH_GLOW_COLOR,
                        (uint8_t)segment_alpha
                )
        );
        renderer_draw_effect_line(
                framebuffer,
                x0,
                y0,
                x1,
                y1,
                body_width,
                renderer_multiply_alpha(
                        PLAYER_SMASH_SLASH_BODY_COLOR,
                        (uint8_t)segment_alpha
                )
        );
        renderer_draw_effect_line(
                framebuffer,
                x0,
                y0,
                x1,
                y1,
                3,
                renderer_multiply_alpha(
                        PLAYER_SMASH_SLASH_CORE_COLOR,
                        (uint8_t)segment_alpha
                )
        );
    }
}

static void renderer_draw_player_berserk_aura(
        Framebuffer* framebuffer,
        const GameState* game,
        const EntityVisualConfig* visual,
        int32_t shake_x,
        int32_t shake_y
) {
    const TimedPowerupState* berserk = game_timed_powerup_get(
            game,
            POWERUP_BERSERK
    );
    int center_x;
    int center_y;
    int phase;
    int alpha = 185;

    if (framebuffer == 0 || game == 0 || visual == 0
            || berserk == 0 || game->gameOver) {
        return;
    }

    if (berserk->remaining_ms < 2000
            && ((game->visualTimeMs / 100) & 1) != 0) {
        alpha = 90;
    }
    center_x = (int)game->playerX + visual->width / 2 + shake_x;
    center_y = (int)game->playerY + visual->height / 2 + shake_y;
    phase = (game->visualTimeMs * 6) % RENDERER_TWO_PI_MILLIRADIANS;

    for (int ray = 0; ray < PLAYER_BERSERK_AURA_RAYS; ++ray) {
        int angle = phase
                + ray * RENDERER_TWO_PI_MILLIRADIANS / PLAYER_BERSERK_AURA_RAYS;
        int pulse = renderer_sin_q16(angle * 2 + phase);
        int inner_x_radius = visual->width / 2 + 16
                + pulse * 5 / RENDERER_TRIG_SCALE;
        int inner_y_radius = visual->height / 2 + 8
                + pulse * 5 / RENDERER_TRIG_SCALE;
        int ray_length = 24 + pulse * 8 / RENDERER_TRIG_SCALE;
        int x0 = center_x
                + renderer_cos_q16(angle) * inner_x_radius / RENDERER_TRIG_SCALE;
        int y0 = center_y
                + renderer_sin_q16(angle) * inner_y_radius / RENDERER_TRIG_SCALE;
        int x1 = center_x
                + renderer_cos_q16(angle) * (inner_x_radius + ray_length)
                        / RENDERER_TRIG_SCALE;
        int y1 = center_y
                + renderer_sin_q16(angle) * (inner_y_radius + ray_length)
                        / RENDERER_TRIG_SCALE;
        uint32_t color = (ray & 1) == 0 ? 0xff5b1fb0u : 0xffc21f90u;

        renderer_draw_effect_line(
                framebuffer,
                x0,
                y0,
                x1,
                y1,
                5,
                renderer_multiply_alpha(color, (uint8_t)alpha)
        );
    }
}

typedef enum {
    PLAYER_HP_TIER_LOW = 0,
    PLAYER_HP_TIER_MEDIUM = 1,
    PLAYER_HP_TIER_FULL = 2
} PlayerHpTier;

static PlayerHpTier renderer_player_hp_tier(
        const GameState* game,
        const PlayerConfig* player_config
) {
    int max_hp;
    int current_hp;

    if (game == 0 || player_config == 0) {
        return PLAYER_HP_TIER_LOW;
    }

    max_hp = player_config->max_hp;
    current_hp = game->playerHp;
    if (max_hp <= 0 || current_hp <= 0) {
        return PLAYER_HP_TIER_LOW;
    }

    if ((int64_t)current_hp * 3 > (int64_t)max_hp * 2) {
        return PLAYER_HP_TIER_FULL;
    }
    if ((int64_t)current_hp * 3 > (int64_t)max_hp) {
        return PLAYER_HP_TIER_MEDIUM;
    }

    return PLAYER_HP_TIER_LOW;
}

static uint32_t renderer_player_heart_seed(int heart_index)
{
    uint32_t value = (uint32_t)(heart_index + 1) * 0x9e3779b9u;

    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

static int renderer_player_heart_angle(
        int run_time_ms,
        int phase,
        int speed,
        int reverse
) {
    int64_t angle = (int64_t)phase
            + (int64_t)run_time_ms * (int64_t)speed * (reverse ? -1 : 1) / 1000;

    angle %= RENDERER_TWO_PI_MILLIRADIANS;
    if (angle < 0) angle += RENDERER_TWO_PI_MILLIRADIANS;
    return (int)angle;
}

typedef struct {
    int offset_x;
    int offset_y;
    int scale;
    int16_t rotation;
} PlayerHeartPose;

static PlayerHeartPose renderer_player_heart_pose(
        int heart_index,
        int time_ms,
        const EntityVisualConfig* visual
) {
    uint32_t seed = renderer_player_heart_seed(heart_index);
    int orbit_layer = heart_index / PLAYER_HEARTS_PER_ORBIT_LAYER;
    int radius_x = visual->width / 2 + 34
            + orbit_layer * PLAYER_HEART_ORBIT_LAYER_STEP;
    int radius_y = visual->height / 2 + 24
            + orbit_layer * PLAYER_HEART_ORBIT_LAYER_STEP;
    int angle_x = renderer_player_heart_angle(
            time_ms,
            (int)(seed % RENDERER_TWO_PI_MILLIRADIANS),
            360 + (int)((seed >> 8) % 420u),
            (seed & 1u) != 0u
    );
    int angle_y = renderer_player_heart_angle(
            time_ms,
            (int)((seed >> 12) % RENDERER_TWO_PI_MILLIRADIANS),
            430 + (int)((seed >> 20) % 380u),
            (seed & 2u) != 0u
    );
    PlayerHeartPose pose;

    pose.offset_x = renderer_cos_q16(angle_x) * radius_x / RENDERER_TRIG_SCALE
            + renderer_sin_q16(angle_y) * 13 / RENDERER_TRIG_SCALE;
    pose.offset_y = renderer_sin_q16(angle_x) * radius_y / RENDERER_TRIG_SCALE
            + renderer_cos_q16(angle_y) * 9 / RENDERER_TRIG_SCALE;
    pose.scale = 940
            + renderer_sin_q16(angle_x + angle_y) * 90 / RENDERER_TRIG_SCALE;
    pose.rotation = (int16_t)(
            renderer_sin_q16(angle_x) * 140 / RENDERER_TRIG_SCALE
    );
    return pose;
}

static void renderer_draw_player_hearts(
        Framebuffer* framebuffer,
        const GameState* game,
        const EntityVisualConfig* visual,
        int sprite_x,
        int sprite_y,
        int32_t shake_x,
        int32_t shake_y,
        AnimationImpact impact,
        int draw_front
) {
    const GeneratedSprite* heart;
    int heart_width;
    int heart_height;
    int center_x;
    int center_y;

    if (game == 0 || visual == 0 || game->playerHp <= 0 || game->gameOver) {
        return;
    }

    heart = generated_sprite_get_by_id("heart");
    if (heart == 0 || heart->width <= 0 || heart->height <= 0) {
        return;
    }

    heart_width = PLAYER_HEART_RENDER_WIDTH;
    heart_height = heart_width * heart->height / heart->width;
    center_x = sprite_x + visual->width / 2 + shake_x + impact.offset_x;
    center_y = sprite_y + visual->height / 2 + shake_y + impact.offset_y;

    for (int heart_index = 0; heart_index < game->playerHp; ++heart_index) {
        PlayerHeartPose pose = renderer_player_heart_pose(
                heart_index,
                game->visualTimeMs,
                visual
        );
        int is_front = pose.offset_y >= 0;
        int draw_x;
        int draw_y;

        if (is_front != draw_front) {
            continue;
        }

        draw_x = center_x + pose.offset_x - heart_width / 2;
        draw_y = center_y + pose.offset_y - heart_height / 2;
        if (draw_x < 0) {
            draw_x = 0;
        } else if (draw_x + heart_width > framebuffer->width) {
            draw_x = framebuffer->width - heart_width;
        }
        if (draw_y < 0) {
            draw_y = 0;
        } else if (draw_y + heart_height > framebuffer->height) {
            draw_y = framebuffer->height - heart_height;
        }

        renderer_draw_generated_sprite_fit_ex(
                framebuffer,
                heart,
                draw_x,
                draw_y,
                heart_width,
                heart_height,
                SPRITE_FIT_STRETCH,
                (int16_t)pose.scale,
                (int16_t)pose.scale,
                pose.rotation,
                255
        );
    }
}

static void renderer_draw_player_lost_heart(
        Framebuffer* framebuffer,
        const GameState* game,
        const EntityVisualConfig* visual,
        int32_t shake_x,
        int32_t shake_y
) {
    const GeneratedSprite* heart;
    PlayerHeartPose start_pose;
    uint32_t seed;
    int heart_width;
    int heart_height;
    int age_ms;
    int progress;
    int inverse;
    int eased_progress;
    int direction;
    int back_distance;
    int rise_distance;
    int drift_angle;
    int draw_x;
    int draw_y;
    int scale;
    int alpha;
    int16_t rotation;

    if (framebuffer == 0
            || game == 0
            || visual == 0
            || !game->playerLostHeartActive
            || game->playerLostHeartIndex < 0) {
        return;
    }

    heart = generated_sprite_get_by_id("heart");
    if (heart == 0 || heart->width <= 0 || heart->height <= 0) {
        return;
    }

    age_ms = game->playerLostHeartAgeMs;
    if (age_ms < 0 || age_ms >= PLAYER_LOST_HEART_LIFETIME_MS) {
        return;
    }

    start_pose = renderer_player_heart_pose(
            game->playerLostHeartIndex,
            game->playerLostHeartStartTimeMs,
            visual
    );
    seed = renderer_player_heart_seed(game->playerLostHeartIndex);
    progress = age_ms * 1000 / PLAYER_LOST_HEART_LIFETIME_MS;
    inverse = 1000 - progress;
    eased_progress = 1000 - inverse * inverse / 1000;
    direction = -1;
    drift_angle = renderer_player_heart_angle(
            age_ms,
            (int)((seed >> 10) % RENDERER_TWO_PI_MILLIRADIANS),
            2600,
            direction < 0
    );
    heart_width = PLAYER_HEART_RENDER_WIDTH;
    heart_height = heart_width * heart->height / heart->width;
    back_distance = visual->width + 80;
    rise_distance = framebuffer->height * 9 / 10;
    draw_x = (int)game->playerLostHeartOriginX
            + visual->width / 2
            + start_pose.offset_x
            + direction * back_distance * progress / 1000
            + renderer_sin_q16(drift_angle) * 18 / RENDERER_TRIG_SCALE
            + shake_x
            - heart_width / 2;
    draw_y = (int)game->playerLostHeartOriginY
            + visual->height / 2
            + start_pose.offset_y
            - rise_distance * eased_progress / 1000
            + shake_y
            - heart_height / 2;
    scale = start_pose.scale * (1100 - progress * 180 / 1000) / 1000;
    rotation = (int16_t)(
            start_pose.rotation + direction * age_ms * 3
    );
    alpha = 255;
    if (age_ms > 1100) {
        alpha = 255
                * (PLAYER_LOST_HEART_LIFETIME_MS - age_ms)
                / (PLAYER_LOST_HEART_LIFETIME_MS - 1100);
    }

    renderer_draw_generated_sprite_fit_ex(
            framebuffer,
            heart,
            draw_x,
            draw_y,
            heart_width,
            heart_height,
            SPRITE_FIT_STRETCH,
            (int16_t)scale,
            (int16_t)scale,
            rotation,
            (uint8_t)alpha
    );
}

static void renderer_draw_player(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y
) {
    const PlayerConfig* player_config = game_player_config(game);
    const EntityVisualConfig* visual = &player_config->visual;
    const PlayerSwordPoseConfig* sword_pose = &player_config->sword.idle;
    const GeneratedSprite* sprite = generated_sprite_get(visual->sprite_id);
    const GeneratedSprite* sword = 0;
    PlayerHpTier hp_tier = renderer_player_hp_tier(game, player_config);
    AnimationImpact impact = entity_animation_get_impact(&game->playerAnimation);
    int sprite_width = sprite != 0
            ? (int)((float)sprite->width * PLAYER_SPRITE_RENDER_SCALE)
            : visual->width;
    int sprite_height = sprite != 0
            ? (int)((float)sprite->height * PLAYER_SPRITE_RENDER_SCALE)
            : visual->height;
    int sprite_x = (int)game->playerX + (visual->width - sprite_width) / 2;
    int sprite_y = (int)game->playerY + visual->height - sprite_height;
    int attack_pose_active = !game->gameOver
            && (game->playerAnimation.slot == ENTITY_ANIM_SMASH
                    || game->playerAttackPoseMs > 0);

    {
        const TimedPowerupState* berserk =
                game_timed_powerup_get(game, POWERUP_BERSERK);

        if (berserk != 0) {
            const PowerupTuning* tuning = powerup_tuning_get();
            int berserk_scale = tuning->berserk_player_scale_permille;
            int elapsed = berserk->age_ms;

            if (elapsed >= 0 && elapsed < tuning->berserk_activation_punch_ms
                    && tuning->berserk_activation_punch_ms > 0) {
                int punch_phase = elapsed * RENDERER_PI_MILLIRADIANS
                        / tuning->berserk_activation_punch_ms;

                berserk_scale += renderer_sin_q16(punch_phase)
                        * tuning->berserk_activation_punch_permille
                        / RENDERER_TRIG_SCALE;
            }
            impact.scale_x = (int16_t)(impact.scale_x * berserk_scale / 1000);
            impact.scale_y = (int16_t)(impact.scale_y * berserk_scale / 1000);
        }
    }

    renderer_draw_player_berserk_aura(
            buffer,
            game,
            visual,
            shake_x,
            shake_y
    );

    if (attack_pose_active) {
        sword_pose = &player_config->sword.attack;
    } else if (game->playerAnimation.slot == ENTITY_ANIM_JUMP
            || game->playerAnimation.slot == ENTITY_ANIM_FALL) {
        sword_pose = &player_config->sword.jump;
    }

    renderer_draw_player_hearts(
            buffer,
            game,
            visual,
            sprite_x,
            sprite_y,
            shake_x,
            shake_y,
            impact,
            0
    );

    if (sprite == 0) {
        renderer_draw_color_rect(
                buffer,
                sprite_x + shake_x + impact.offset_x,
                sprite_y + shake_y + impact.offset_y,
                sprite_width,
                sprite_height,
                visual->color
        );
    } else {
        uint32_t eye_color;

        if (hp_tier == PLAYER_HP_TIER_FULL) {
            eye_color = player_config->eye_colors.full_hp_color;
        } else if (hp_tier == PLAYER_HP_TIER_MEDIUM) {
            eye_color = player_config->eye_colors.medium_hp_color;
        } else {
            eye_color = player_config->eye_colors.low_hp_color;
        }

        renderer_sprite_tint_source = player_config->eye_colors.source_color;
        renderer_sprite_tint_target = eye_color;
        renderer_sprite_tint_enabled = 1;
        renderer_draw_generated_sprite_fit_ex(
                buffer,
                sprite,
                sprite_x + shake_x + impact.offset_x,
                sprite_y + shake_y + impact.offset_y,
                sprite_width,
                sprite_height,
                SPRITE_FIT_STRETCH,
                impact.scale_x,
                impact.scale_y,
                impact.rotation,
                impact.alpha
        );
        renderer_sprite_tint_enabled = 0;
    }

    if (game->gameOver || game->playerHp <= 0 || sprite == 0) {
        renderer_draw_player_lost_heart(
                buffer,
                game,
                visual,
                shake_x,
                shake_y
        );
        return;
    }

    if (hp_tier == PLAYER_HP_TIER_FULL) {
        sword = generated_sprite_get(SPRITE_SWORD_3);
    } else if (hp_tier == PLAYER_HP_TIER_MEDIUM) {
        sword = generated_sprite_get(SPRITE_SWORD_2);
    } else {
        sword = generated_sprite_get(SPRITE_SWORD_1);
    }

    if (sword != 0) {
        int sword_width = (int)((float)sword->width * PLAYER_SWORD_RENDER_SCALE);
        int sword_height = (int)((float)sword->height * PLAYER_SWORD_RENDER_SCALE);
        int sword_x = sprite_x + sword_pose->offset_x
                - (int)((float)sword_width * PLAYER_SWORD_PIVOT_X);
        int sword_y = sprite_y + sword_pose->offset_y
                - (int)((float)sword_height * PLAYER_SWORD_PIVOT_Y);
        int16_t sword_rotation = impact.rotation + (int16_t)(
                sword_pose->rotation_degrees * 17.45329252f
        );

        renderer_draw_player_smash_slash(
                buffer,
                game,
                visual,
                sword_pose,
                sprite_x,
                sprite_y,
                shake_x,
                shake_y,
                impact
        );

        renderer_draw_generated_sprite_fit_ex(
                buffer,
                sword,
                sword_x + shake_x + impact.offset_x,
                sword_y + shake_y + impact.offset_y,
                sword_width,
                sword_height,
                SPRITE_FIT_STRETCH,
                impact.scale_x,
                impact.scale_y,
                sword_rotation,
                impact.alpha
        );
    }

    renderer_draw_player_hearts(
            buffer,
            game,
            visual,
            sprite_x,
            sprite_y,
            shake_x,
            shake_y,
            impact,
            1
    );
    renderer_draw_player_lost_heart(
            buffer,
            game,
            visual,
            shake_x,
            shake_y
    );
}

static void renderer_draw_bird_camera_hit_crack(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y
) {
    const GeneratedSprite* crack_sprite;
    int draw_width;
    int draw_height;
    int draw_x;
    int draw_y;
    int impact_x;
    int impact_y;

    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        const Entity* entity = game->entities + entity_index;

        if (!entity->active
                || !entity->dead
                || entity->config == 0
                || entity->config->visual.sprite_id != SPRITE_BIRD
                || entity->animation.time_ms < BIRD_CAMERA_HIT_IMPACT_MS
                || entity->animation.time_ms >= BIRD_CAMERA_HIT_IMPACT_MS
                        + BIRD_CAMERA_HIT_CRACK_DURATION_MS) {
            continue;
        }

        crack_sprite = generated_sprite_get_by_id("cracked_screen");
        if (crack_sprite == 0) {
            return;
        }

        {
            AnimationImpact impact = animation_evaluate(
                    entity->animation.clip,
                    BIRD_CAMERA_HIT_IMPACT_MS
            );
            impact_x = (int)entity->x
                    + shake_x
                    + impact.offset_x
                    + entity_get_width(entity) / 2;
            impact_y = (int)entity->y
                    + shake_y
                    + impact.offset_y
                    + entity_get_height(entity) / 2;
        }

        draw_width = buffer->width * 4 / 5;
        draw_height = draw_width * crack_sprite->height / crack_sprite->width;
        if (draw_height > buffer->height * 4 / 5) {
            draw_height = buffer->height * 4 / 5;
            draw_width = draw_height * crack_sprite->width / crack_sprite->height;
        }

        draw_x = impact_x - draw_width / 2;
        draw_y = impact_y - draw_height / 2;
        if (draw_x < 0) {
            draw_x = 0;
        } else if (draw_x + draw_width > buffer->width) {
            draw_x = buffer->width - draw_width;
        }
        if (draw_y < 0) {
            draw_y = 0;
        } else if (draw_y + draw_height > buffer->height) {
            draw_y = buffer->height - draw_height;
        }
        renderer_draw_generated_sprite_fit(
                buffer,
                crack_sprite,
                draw_x,
                draw_y,
                draw_width,
                draw_height,
                SPRITE_FIT_CONTAIN
        );
        return;
    }
}

static void renderer_draw_foreground_decorations(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y
) {
    for (int decoration_index = 0;
            decoration_index < FOREGROUND_MAX_INSTANCES;
            ++decoration_index) {
        const ForegroundDecoration* decoration =
                game->foregroundDecorations + decoration_index;
        float alpha;
        float min_alpha = FOREGROUND_FADE_MIN_ALPHA;
        uint8_t alpha_u8;

        if (!decoration->active
                || decoration->sprite == 0
                || decoration->sprite->pixels == 0
                || decoration->width <= 0
                || decoration->height <= 0) {
            continue;
        }

        alpha = decoration->alpha;
        if (min_alpha < 0.0f) {
            min_alpha = 0.0f;
        } else if (min_alpha > 1.0f) {
            min_alpha = 1.0f;
        }

        if (alpha != alpha) {
            alpha = 1.0f;
        } else if (alpha < min_alpha) {
            alpha = min_alpha;
        } else if (alpha > 1.0f) {
            alpha = 1.0f;
        }

        if (alpha >= 1.0f) {
            renderer_draw_generated_sprite_scaled(
                    buffer,
                    decoration->sprite,
                    (int)decoration->x + shake_x,
                    (int)decoration->y + shake_y,
                    decoration->width,
                    decoration->height
            );
            continue;
        }

        alpha_u8 = (uint8_t)(alpha * 255.0f);
        renderer_draw_generated_sprite_fit_ex(
                buffer,
                decoration->sprite,
                (int)decoration->x + shake_x,
                (int)decoration->y + shake_y,
                decoration->width,
                decoration->height,
                SPRITE_FIT_STRETCH,
                1000,
                1000,
                0,
                alpha_u8
        );
    }
}

static uint32_t renderer_color_with_alpha(uint32_t color, uint8_t alpha)
{
    uint32_t base_alpha = color & 0xffu;
    uint32_t out_alpha = (base_alpha * (uint32_t)alpha) / 255u;

    return (color & 0xffffff00u) | out_alpha;
}

static void renderer_draw_floating_texts(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y
) {
    static const PackedFont* floating_text_font = 0;
    const PackedFont* font;

    if (buffer == 0 || game == 0) {
        return;
    }

    if (floating_text_font == 0) {
        floating_text_font = font_registry_find(FLOATING_TEXT_FONT_ID);
    }

    font = floating_text_font;
    if (font == 0) {
        return;
    }

    for (int text_index = 0; text_index < MAX_FLOATING_TEXTS; ++text_index) {
        const FloatingText* text = game->floatingTexts + text_index;
        int alpha;
        int text_width;
        int text_height;
        int draw_x;
        int draw_y;
        uint32_t color;

        if (!text->active || text->text[0] == 0 || text->lifetime_ms <= 0) {
            continue;
        }

        alpha = 255 - (text->age_ms * 255) / text->lifetime_ms;
        if (alpha < 0) {
            alpha = 0;
        } else if (alpha > 255) {
            alpha = 255;
        }

        text_width = font_measure_text(font, FLOATING_TEXT_SCALE, text->text);
        text_height = (int)font->grid_size * FLOATING_TEXT_SCALE;
        draw_x = (int)text->x + shake_x - text_width / 2;
        draw_y = (int)text->y + shake_y - text_height / 2;
        color = renderer_color_with_alpha(text->color, (uint8_t)alpha);

        font_draw_text(
                buffer,
                font,
                draw_x + 2,
                draw_y + 2,
                FLOATING_TEXT_SCALE,
                renderer_color_with_alpha(0x000000ccu, (uint8_t)alpha),
                text->text
        );
        font_draw_text(
                buffer,
                font,
                draw_x,
                draw_y,
                FLOATING_TEXT_SCALE,
                color,
                text->text
        );
    }
}

static void renderer_draw_timed_powerup_indicators(
        ANativeWindow_Buffer* buffer,
        const GameState* game
) {
    static const PackedFont* indicator_font = 0;
    const PowerupTuning* tuning;
    int row = 0;

    if (buffer == 0 || game == 0 || game->gameOver) return;

    if (indicator_font == 0) {
        indicator_font = font_registry_find(FLOATING_TEXT_FONT_ID);
    }
    if (indicator_font == 0) return;

    tuning = powerup_tuning_get();
    for (int index = 0; index < MAX_TIMED_POWERUPS; ++index) {
        const TimedPowerupState* state = game->timedPowerups + index;
        const char* name;
        char text[32];
        int scale = 4;
        int center_x = buffer->width / 2;
        int center_y = 64 + row * 76;
        int alpha = 255;
        uint32_t color = 0xffb020ff;

        if (!state->active || state->config == 0 || state->remaining_ms <= 0) {
            continue;
        }

        name = localization_text(
                game_settings_normalize_locale(game->settings.locale),
                state->config->display_name_text_id
        );
        if (name == 0 || name[0] == 0) {
            name = state->config->display_name != 0
                    ? state->config->display_name : state->config->id;
        }
        if (name == 0) name = "POWERUP";

        if (tuning->timed_indicator_flight_ms > 0
                && state->age_ms < tuning->timed_indicator_flight_ms) {
            int progress = state->age_ms * 1000 / tuning->timed_indicator_flight_ms;
            int inverse = 1000 - progress;
            int eased = 1000 - inverse * inverse / 1000;

            center_x = (int)state->origin_x
                    + (buffer->width / 2 - (int)state->origin_x) * eased / 1000;
            center_y = (int)state->origin_y
                    + (64 + row * 76 - (int)state->origin_y) * eased / 1000;
            scale = 4 + eased / 700;
            snprintf(text, sizeof(text), "%s", name);
        } else {
            int seconds = (state->remaining_ms + 999) / 1000;
            int tick_phase = state->remaining_ms % 1000;

            snprintf(text, sizeof(text), "%s %d", name, seconds);
            if (tick_phase > 800) scale = 5;
            if (seconds <= 3) {
                color = 0xff4938ff;
                if (((state->remaining_ms / 125) & 1) != 0) alpha = 180;
            }
        }

        {
            int text_width = font_measure_text(indicator_font, scale, text);
            int text_height = (int)indicator_font->grid_size * scale;
            int draw_x = center_x - text_width / 2;
            int draw_y = center_y - text_height / 2;

            font_draw_text(
                    buffer,
                    indicator_font,
                    draw_x + 3,
                    draw_y + 3,
                    scale,
                    renderer_color_with_alpha(0x000000ddu, (uint8_t)alpha),
                    text
            );
            font_draw_text(
                    buffer,
                    indicator_font,
                    draw_x,
                    draw_y,
                    scale,
                    renderer_color_with_alpha(color, (uint8_t)alpha),
                    text
            );
        }
        row += 1;
    }
}

#if LITTLE_ONE_SHOW_WIREFRAMES
static void renderer_draw_entity_wireframes(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y
) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        const Entity* entity = game->entities + entity_index;
        int entity_width;
        int entity_height;

        if (!entity->active) {
            continue;
        }

        if (entity->type != ENTITY_THREAT && entity->type != ENTITY_POWERUP) {
            continue;
        }

        entity_width = entity_get_width(entity);
        entity_height = entity_get_height(entity);

        renderer_draw_size_wireframe_rect(
                buffer,
                (int)entity->x + shake_x,
                (int)entity->y + shake_y,
                entity_width,
                entity_height
        );
        renderer_draw_hurt_zone_outline(
                buffer,
                (int)entity->x + shake_x,
                (int)entity->y + shake_y,
                entity_width,
                entity_height,
                entity_get_hurt_zone(entity)
        );
    }
}

static void renderer_draw_player_wireframe(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y
) {
    const PlayerConfig* player_config = game_player_config(game);
    const EntityVisualConfig* visual = &player_config->visual;
    const CollisionBoundary* attack_zone = &player_config->attack_zone;

    renderer_draw_size_wireframe_rect(
            buffer,
            (int)game->playerX + shake_x,
            (int)game->playerY + shake_y,
            visual->width,
            visual->height
    );
    renderer_draw_hurt_zone_outline(
            buffer,
            (int)game->playerX + shake_x,
            (int)game->playerY + shake_y,
            visual->width,
            visual->height,
            &game_player_config(game)->hurt_zone
    );
    renderer_draw_attack_zone_wireframe(
            buffer,
            (int)game->playerX + attack_zone->x + shake_x,
            (int)game->playerY + attack_zone->y + shake_y,
            attack_zone->width,
            attack_zone->height
    );
}
#endif

static void renderer_draw_diagnostics(ANativeWindow_Buffer* buffer, const GameState* game) {
    int best_digit_count = renderer_count_digits(game->bestScore);
    int best_width = best_digit_count * RENDERER_DIGIT_WIDTH
            + (best_digit_count - 1) * RENDERER_DIGIT_SPACING;
    int fps_y = buffer->height - RENDERER_DIGIT_HEIGHT - 12;

    if (fps_y < 40) {
        fps_y = 40;
    }

    renderer_draw_number(buffer, 12, 12, game->score);
    renderer_draw_number(buffer, buffer->width - best_width - 12, 12, game->bestScore);
    #if LITTLE_ONE_SHOW_FPS
    renderer_draw_number(buffer, 12, fps_y, game->fps);
    #else
    (void)fps_y;
    #endif
}

void renderer_draw_frame(ANativeWindow_Buffer* buffer, const GameState* game) {
    const BackgroundConfig* background;
    int32_t gameplay_ground_y;
    int32_t shake_x;
    int32_t shake_y;

    if (buffer == 0 || buffer->bits == 0 || game == 0) {
        return;
    }

    background = background_config_get();
    gameplay_ground_y = renderer_gameplay_ground_y(game);
    shake_x = screen_shake_get_x(&game->screenShake);
    shake_y = screen_shake_get_y(&game->screenShake);

    render_background(
            buffer,
            background,
            (int32_t)game->worldScrollX,
            gameplay_ground_y,
            shake_x,
            shake_y
    );
    render_ground(
            buffer,
            &background->ground_visual,
            (int32_t)game->worldScrollX,
            gameplay_ground_y,
            shake_x,
            shake_y
    );
    renderer_draw_entities(buffer, game, shake_x, shake_y, 0);
    if (!game->gameOver) {
        renderer_draw_player(buffer, game, shake_x, shake_y);
    }
    game_effects_render(buffer, shake_x, shake_y);
    renderer_draw_foreground_decorations(buffer, game, shake_x, shake_y);
    renderer_draw_floating_texts(buffer, game, shake_x, shake_y);
    #if LITTLE_ONE_SHOW_WIREFRAMES
    renderer_draw_entity_wireframes(buffer, game, shake_x, shake_y);
    renderer_draw_player_wireframe(buffer, game, shake_x, shake_y);
    renderer_draw_ground_debug_line(buffer, gameplay_ground_y + shake_y);
    #endif
    renderer_draw_entities(buffer, game, shake_x, shake_y, 1);
    if (game->gameOver) {
        renderer_draw_player(buffer, game, shake_x, shake_y);
    }
    renderer_draw_bird_camera_hit_crack(buffer, game, shake_x, shake_y);
    hud_render(buffer, game);
    renderer_draw_timed_powerup_indicators(buffer, game);
    menu_render(buffer, game);
}
