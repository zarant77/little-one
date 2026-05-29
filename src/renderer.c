#include "renderer.h"

#include <stdint.h>

#include "entity_config.h"

#define RENDERER_SMASH_EXTRA_HEIGHT 24
#define RENDERER_GROUND_MARGIN 48
#define RENDERER_GROUND_LINE_HEIGHT 2
#define RENDERER_GROUND_MARKER_SPACING 96
#define RENDERER_GROUND_MARKER_WIDTH 8
#define RENDERER_GROUND_MARKER_HEIGHT 18
#define RENDERER_BACKGROUND_MARKER_SPACING 180
#define RENDERER_BACKGROUND_MARKER_SIZE 5
#define RENDERER_PARALLAX_DIVISOR 3
#define RENDERER_DIGIT_WIDTH 12
#define RENDERER_DIGIT_HEIGHT 20
#define RENDERER_DIGIT_THICKNESS 3
#define RENDERER_DIGIT_SPACING 4

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

static int renderer_count_digits(int value) {
    int digits = 1;

    while (value >= 10) {
        value /= 10;
        digits += 1;
    }

    return digits;
}

static int renderer_is_digit_segment_pixel(int x, int y, int origin_x, int origin_y, int segment) {
    int half_height = RENDERER_DIGIT_HEIGHT / 2;

    if (segment == 0) {
        return renderer_is_inside_rect(
                x,
                y,
                origin_x + RENDERER_DIGIT_THICKNESS,
                origin_y,
                RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS * 2,
                RENDERER_DIGIT_THICKNESS
        );
    }

    if (segment == 1) {
        return renderer_is_inside_rect(
                x,
                y,
                origin_x + RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS,
                origin_y + RENDERER_DIGIT_THICKNESS,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS
        );
    }

    if (segment == 2) {
        return renderer_is_inside_rect(
                x,
                y,
                origin_x + RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS,
                origin_y + half_height,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS
        );
    }

    if (segment == 3) {
        return renderer_is_inside_rect(
                x,
                y,
                origin_x + RENDERER_DIGIT_THICKNESS,
                origin_y + RENDERER_DIGIT_HEIGHT - RENDERER_DIGIT_THICKNESS,
                RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS * 2,
                RENDERER_DIGIT_THICKNESS
        );
    }

    if (segment == 4) {
        return renderer_is_inside_rect(
                x,
                y,
                origin_x,
                origin_y + half_height,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS
        );
    }

    if (segment == 5) {
        return renderer_is_inside_rect(
                x,
                y,
                origin_x,
                origin_y + RENDERER_DIGIT_THICKNESS,
                RENDERER_DIGIT_THICKNESS,
                half_height - RENDERER_DIGIT_THICKNESS
        );
    }

    if (segment == 6) {
        return renderer_is_inside_rect(
                x,
                y,
                origin_x + RENDERER_DIGIT_THICKNESS,
                origin_y + half_height - RENDERER_DIGIT_THICKNESS / 2,
                RENDERER_DIGIT_WIDTH - RENDERER_DIGIT_THICKNESS * 2,
                RENDERER_DIGIT_THICKNESS
        );
    }

    return 0;
}

static int renderer_is_digit_pixel(int x, int y, int origin_x, int origin_y, int digit) {
    unsigned char segments;

    if (digit < 0 || digit > 9) {
        return 0;
    }

    segments = RENDERER_DIGIT_SEGMENTS[digit];
    for (int segment = 0; segment < 7; ++segment) {
        if ((segments & (1 << segment)) != 0
                && renderer_is_digit_segment_pixel(x, y, origin_x, origin_y, segment)) {
            return 1;
        }
    }

    return 0;
}

static int renderer_is_number_pixel(int x, int y, int origin_x, int origin_y, int value) {
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

        if (renderer_is_digit_pixel(x, y, digit_x, origin_y, digits[draw_index])) {
            return 1;
        }
    }

    return 0;
}

static int renderer_is_score_pixel(int x, int y, const GameState* game, int screen_width) {
    int best_digit_count = renderer_count_digits(game->bestScore);
    int best_width = best_digit_count * RENDERER_DIGIT_WIDTH
            + (best_digit_count - 1) * RENDERER_DIGIT_SPACING;

    return renderer_is_number_pixel(x, y, 12, 12, game->score)
           || renderer_is_number_pixel(x, y, screen_width - best_width - 12, 12, game->bestScore);
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
            entity_config_get_player()->visual.width,
            rect_height
    );
}

static int renderer_is_entity_pixel(int x, int y, const Entity* entity) {
    if (entity == 0 || !entity->active) {
        return 0;
    }

    return renderer_is_inside_rect(
            x,
            y,
            (int)entity->x,
            (int)entity->y,
            entity_get_width(entity),
            entity_get_height(entity)
    );
}

static int renderer_is_any_entity_pixel(int x, int y, const GameState* game) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        if (renderer_is_entity_pixel(x, y, game->entities + entity_index)) {
            return 1;
        }
    }

    return 0;
}

void renderer_draw_frame(ANativeWindow_Buffer* buffer, const GameState* game) {
    if (buffer == NULL || buffer->bits == NULL || game == NULL) {
        return;
    }

    int width = buffer->width;
    int height = buffer->height;
    int rect_x = (int)game->playerX;
    int rect_y = (int)game->playerY;
    int rect_height = entity_config_get_player()->visual.height;
    if (game->playerSmashing) {
        rect_height += RENDERER_SMASH_EXTRA_HEIGHT;
    }

    if (buffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* pixels = (uint16_t*)buffer->bits;

        for (int y = 0; y < height; ++y) {
            uint16_t* row = pixels + (y * buffer->stride);

            for (int x = 0; x < width; ++x) {
                int is_ground = renderer_is_ground_line(y, game);
                int is_ground_marker = renderer_is_ground_marker(x, y, game);
                int is_background = renderer_is_background_marker(x, y, game);
                int is_entity = renderer_is_any_entity_pixel(x, y, game);
                int is_player = renderer_is_player_pixel(x, y, rect_x, rect_y, rect_height);
                int is_score = renderer_is_score_pixel(x, y, game, width);
                int is_foreground = is_ground || is_ground_marker || is_entity || is_player || is_score;
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
            int is_entity = renderer_is_any_entity_pixel(x, y, game);
            int is_player = renderer_is_player_pixel(x, y, rect_x, rect_y, rect_height);
            int is_score = renderer_is_score_pixel(x, y, game, width);
            int is_foreground = is_ground || is_ground_marker || is_entity || is_player || is_score;
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
