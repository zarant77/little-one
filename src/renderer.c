#include "renderer.h"

#include <stdint.h>

static void renderer_put_rgba_8888(uint8_t* pixel, uint8_t r, uint8_t g, uint8_t b) {
    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = 255;
}

static void renderer_put_rgb_565(uint16_t* pixel, uint8_t r, uint8_t g, uint8_t b) {
    uint16_t r5 = (uint16_t)(r >> 3);
    uint16_t g6 = (uint16_t)(g >> 2);
    uint16_t b5 = (uint16_t)(b >> 3);

    *pixel = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static int renderer_is_inside_rect(
        int x,
        int y,
        int rect_x,
        int rect_y,
        int rect_width,
        int rect_height
) {
    return x >= rect_x
           && x < rect_x + rect_width
           && y >= rect_y
           && y < rect_y + rect_height;
}

void renderer_draw_first_pixel(ANativeWindow_Buffer* buffer) {
    if (buffer == NULL || buffer->bits == NULL) {
        return;
    }

    int width = buffer->width;
    int height = buffer->height;
    int rect_width = 64;
    int rect_height = 64;

    if (rect_width > width) {
        rect_width = width;
    }
    if (rect_height > height) {
        rect_height = height;
    }

    int rect_x = (width - rect_width) / 2;
    int rect_y = (height - rect_height) / 2;

    if (buffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* pixels = (uint16_t*)buffer->bits;

        for (int y = 0; y < height; ++y) {
            uint16_t* row = pixels + (y * buffer->stride);

            for (int x = 0; x < width; ++x) {
                uint8_t color = renderer_is_inside_rect(
                        x,
                        y,
                        rect_x,
                        rect_y,
                        rect_width,
                        rect_height
                ) ? 255 : 0;

                renderer_put_rgb_565(row + x, color, color, color);
            }
        }

        return;
    }

    uint8_t* pixels = (uint8_t*)buffer->bits;

    for (int y = 0; y < height; ++y) {
        uint8_t* row = pixels + ((y * buffer->stride) * 4);

        for (int x = 0; x < width; ++x) {
            uint8_t color = renderer_is_inside_rect(
                    x,
                    y,
                    rect_x,
                    rect_y,
                    rect_width,
                    rect_height
            ) ? 255 : 0;

            renderer_put_rgba_8888(row + (x * 4), color, color, color);
        }
    }
}
