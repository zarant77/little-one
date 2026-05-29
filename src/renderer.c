#include "renderer.h"

#include <stdint.h>

#define RENDERER_PLAYER_SIZE 64
#define RENDERER_SMASH_PLAYER_HEIGHT 88
#define RENDERER_GROUND_MARGIN 48
#define RENDERER_GROUND_LINE_HEIGHT 2
#define RENDERER_GROUND_MARKER_SPACING 96
#define RENDERER_GROUND_MARKER_WIDTH 8
#define RENDERER_GROUND_MARKER_HEIGHT 18
#define RENDERER_BACKGROUND_MARKER_SPACING 180
#define RENDERER_BACKGROUND_MARKER_SIZE 5
#define RENDERER_PARALLAX_DIVISOR 3

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

static int renderer_is_ground_line(int y, const GameState* game) {
    int ground_y = game->screenHeight - RENDERER_GROUND_MARGIN;

    return y >= ground_y && y < ground_y + RENDERER_GROUND_LINE_HEIGHT;
}

static int renderer_positive_mod(int value, int divisor) {
    int result = value % divisor;

    if (result < 0) {
        result += divisor;
    }

    return result;
}

static int renderer_is_ground_marker(int x, int y, const GameState* game) {
    int ground_y = game->screenHeight - RENDERER_GROUND_MARGIN;
    int scroll = (int)game->worldScrollX;
    int marker_x = renderer_positive_mod(x + scroll, RENDERER_GROUND_MARKER_SPACING);

    return marker_x < RENDERER_GROUND_MARKER_WIDTH
           && y >= ground_y - RENDERER_GROUND_MARKER_HEIGHT
           && y < ground_y;
}

static int renderer_is_background_marker(int x, int y, const GameState* game) {
    int scroll = (int)(game->worldScrollX / RENDERER_PARALLAX_DIVISOR);
    int marker_x = renderer_positive_mod(x + scroll, RENDERER_BACKGROUND_MARKER_SPACING);
    int marker_row = renderer_positive_mod((x + scroll) / RENDERER_BACKGROUND_MARKER_SPACING, 3);
    int marker_y = 70 + marker_row * 45;

    return marker_x < RENDERER_BACKGROUND_MARKER_SIZE
           && y >= marker_y
           && y < marker_y + RENDERER_BACKGROUND_MARKER_SIZE;
}

static int renderer_is_player_pixel(int x, int y, int rect_x, int rect_y, int rect_height) {
    return renderer_is_inside_rect(
            x,
            y,
            rect_x,
            rect_y,
            RENDERER_PLAYER_SIZE,
            rect_height
    );
}

void renderer_draw_frame(ANativeWindow_Buffer* buffer, const GameState* game) {
    if (buffer == NULL || buffer->bits == NULL || game == NULL) {
        return;
    }

    int width = buffer->width;
    int height = buffer->height;
    int rect_x = (int)game->playerX;
    int rect_y = (int)game->playerY;
    int rect_height = game->playerSmashing ? RENDERER_SMASH_PLAYER_HEIGHT : RENDERER_PLAYER_SIZE;

    if (buffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* pixels = (uint16_t*)buffer->bits;

        for (int y = 0; y < height; ++y) {
            uint16_t* row = pixels + (y * buffer->stride);

            for (int x = 0; x < width; ++x) {
                int is_ground = renderer_is_ground_line(y, game);
                int is_ground_marker = renderer_is_ground_marker(x, y, game);
                int is_background = renderer_is_background_marker(x, y, game);
                int is_player = renderer_is_player_pixel(x, y, rect_x, rect_y, rect_height);
                int is_foreground = is_ground || is_ground_marker || is_player;
                uint8_t color = is_foreground ? 255 : 0;
                uint8_t red = is_background && !is_foreground ? 48 : color;
                uint8_t green = is_background && !is_foreground ? 48 : color;
                uint8_t blue = is_background && !is_foreground ? 48 : color;

                if (game->playerSmashing && is_player) {
                    red = 255;
                    green = 64;
                    blue = 64;
                }

                renderer_put_rgb_565(row + x, red, green, blue);
            }
        }

        return;
    }

    uint8_t* pixels = (uint8_t*)buffer->bits;

    for (int y = 0; y < height; ++y) {
        uint8_t* row = pixels + ((y * buffer->stride) * 4);

        for (int x = 0; x < width; ++x) {
            int is_ground = renderer_is_ground_line(y, game);
            int is_ground_marker = renderer_is_ground_marker(x, y, game);
            int is_background = renderer_is_background_marker(x, y, game);
            int is_player = renderer_is_player_pixel(x, y, rect_x, rect_y, rect_height);
            int is_foreground = is_ground || is_ground_marker || is_player;
            uint8_t color = is_foreground ? 255 : 0;
            uint8_t red = is_background && !is_foreground ? 48 : color;
            uint8_t green = is_background && !is_foreground ? 48 : color;
            uint8_t blue = is_background && !is_foreground ? 48 : color;

            if (game->playerSmashing && is_player) {
                red = 255;
                green = 64;
                blue = 64;
            }

            renderer_put_rgba_8888(row + (x * 4), red, green, blue);
        }
    }
}
