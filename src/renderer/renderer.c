#include "renderer.h"

#include <stddef.h>
#include <stdint.h>

#include "../config/background_config.h"
#include "../game/game_effects.h"
#include "../game/game_settings.h"
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
#define RENDERER_GROUND_DEBUG_COLOR 0x00ffffff
#define RENDERER_TRIG_SCALE 65536
#define RENDERER_PI_MILLIRADIANS 3141
#define RENDERER_TWO_PI_MILLIRADIANS 6283
#define RENDERER_HALF_PI_MILLIRADIANS 1571
#define BIRD_CAMERA_HIT_IMPACT_MS 520
#define BIRD_CAMERA_HIT_CRACK_DURATION_MS 500

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

static void renderer_write_sprite_pixel(
        Framebuffer* framebuffer,
        int target_x,
        int target_y,
        uint32_t source_color,
        uint8_t alpha
) {
    uint32_t color;

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
                uint32_t source_color = renderer_multiply_alpha(source_row[source_x], alpha);
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
            uint32_t source_color = renderer_multiply_alpha(source_row[source_x], alpha);
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
        int y
) {
    renderer_draw_generated_sprite_scaled(
            framebuffer,
            sprite,
            x,
            y,
            sprite->width,
            sprite->height
    );
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
        renderer_draw_parallax_tile(framebuffer, sprite, screen_x, layer_y);
        return;
    }

    start_x = screen_x;
    if (layer->repeat_x) {
        start_x = screen_x % tile_w;
        if (start_x > 0) {
            start_x -= tile_w;
        }
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
                renderer_draw_parallax_tile(framebuffer, sprite, x, y);
            }
        } else {
            renderer_draw_parallax_tile(framebuffer, sprite, screen_x, y);
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

        if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
            color = entity->enemyConfig->visual.color;
            sprite_id = entity->enemyConfig->visual.sprite_id;
        } else if (entity->type == ENTITY_OBSTACLE && entity->obstacleConfig != 0) {
            color = entity->obstacleConfig->visual.color;
            sprite_id = entity->obstacleConfig->visual.sprite_id;
        }

        entity_width = entity_get_width(entity);
        entity_height = entity_get_height(entity);
        impact = entity_animation_get_impact(&entity->animation);

        sprite = generated_sprite_get(sprite_id);
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

static void renderer_draw_player(
        ANativeWindow_Buffer* buffer,
        const GameState* game,
        int32_t shake_x,
        int32_t shake_y
) {
    const EntityVisualConfig* visual = game_player_visual_config();
    const GeneratedSprite* sprite = generated_sprite_get(SPRITE_PLAYER);
    AnimationImpact impact = entity_animation_get_impact(&game->playerAnimation);

    if (sprite == 0) {
        renderer_draw_color_rect(
                buffer,
                (int)game->playerX + shake_x + impact.offset_x,
                (int)game->playerY + shake_y + impact.offset_y,
                visual->width,
                visual->height,
                visual->color
        );
        return;
    }

    renderer_draw_generated_sprite_fit_ex(
            buffer,
            sprite,
            (int)game->playerX + shake_x + impact.offset_x,
            (int)game->playerY + shake_y + impact.offset_y,
            visual->width,
            visual->height,
            DEFAULT_SPRITE_FIT_MODE,
            impact.scale_x,
            impact.scale_y,
            impact.rotation,
            impact.alpha
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
                || entity->enemyConfig == 0
                || entity->enemyConfig->visual.sprite_id != SPRITE_BIRD
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

        if (entity->type != ENTITY_ENEMY && entity->type != ENTITY_OBSTACLE) {
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
    const EntityVisualConfig* visual = game_player_visual_config();

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
            game_player_hurt_zone_config()
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
    menu_render(buffer, game);
}
