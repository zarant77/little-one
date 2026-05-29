#include "renderer.h"

#include <stdint.h>

#include "game_settings.h"
#include "player_config.h"

#define RENDERER_SMASH_EXTRA_HEIGHT 24
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

static uint16_t renderer_rgb_565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t r5 = (uint16_t)(r >> 3);
    uint16_t g6 = (uint16_t)(g >> 2);
    uint16_t b5 = (uint16_t)(b >> 3);

    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static void renderer_clear(ANativeWindow_Buffer* buffer) {
    if (buffer->format == WINDOW_FORMAT_RGB_565) {
        uint16_t* pixels = (uint16_t*)buffer->bits;

        for (int y = 0; y < buffer->height; ++y) {
            uint16_t* row = pixels + (y * buffer->stride);
            for (int x = 0; x < buffer->width; ++x) {
                row[x] = 0;
            }
        }

        return;
    }

    uint8_t* pixels = (uint8_t*)buffer->bits;
    for (int y = 0; y < buffer->height; ++y) {
        uint8_t* row = pixels + ((y * buffer->stride) * 4);
        for (int x = 0; x < buffer->width; ++x) {
            uint8_t* pixel = row + (x * 4);
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
            pixel[3] = 255;
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
        uint16_t color = renderer_rgb_565(r, g, b);
        uint16_t* pixels = (uint16_t*)buffer->bits;

        for (int draw_y = min_y; draw_y < max_y; ++draw_y) {
            uint16_t* row = pixels + (draw_y * buffer->stride);
            for (int draw_x = min_x; draw_x < max_x; ++draw_x) {
                row[draw_x] = color;
            }
        }

        return;
    }

    uint8_t* pixels = (uint8_t*)buffer->bits;
    for (int draw_y = min_y; draw_y < max_y; ++draw_y) {
        uint8_t* row = pixels + ((draw_y * buffer->stride) * 4);
        for (int draw_x = min_x; draw_x < max_x; ++draw_x) {
            uint8_t* pixel = row + (draw_x * 4);
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
            pixel[3] = 255;
        }
    }
}

static void renderer_draw_color_rect(
        ANativeWindow_Buffer* buffer,
        int x,
        int y,
        int width,
        int height,
        uint32_t color
) {
    renderer_draw_rect(
            buffer,
            x,
            y,
            width,
            height,
            (uint8_t)((color >> 24) & 0xff),
            (uint8_t)((color >> 16) & 0xff),
            (uint8_t)((color >> 8) & 0xff)
    );
}

static int renderer_positive_mod(int value, int divisor) {
    int result = value % divisor;

    if (result < 0) {
        result += divisor;
    }

    return result;
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

static void renderer_draw_background(ANativeWindow_Buffer* buffer, const GameState* game) {
    int scroll = (int)(game->worldScrollX / RENDERER_PARALLAX_DIVISOR);
    int first_x = -renderer_positive_mod(scroll, RENDERER_BACKGROUND_MARKER_SPACING);

    for (int x = first_x; x < buffer->width; x += RENDERER_BACKGROUND_MARKER_SPACING) {
        int marker_index = renderer_positive_mod((x + scroll) / RENDERER_BACKGROUND_MARKER_SPACING, 3);
        int y = 70 + marker_index * 45;

        renderer_draw_rect(
                buffer,
                x,
                y,
                RENDERER_BACKGROUND_MARKER_SIZE,
                RENDERER_BACKGROUND_MARKER_SIZE,
                48,
                48,
                48
        );
    }
}

static void renderer_draw_ground(ANativeWindow_Buffer* buffer, const GameState* game) {
    int ground_y = game->screenHeight - (int)LITTLE_ONE_GROUND_BOTTOM_MARGIN_PX;
    int scroll = (int)game->worldScrollX;
    int first_x = -renderer_positive_mod(scroll, RENDERER_GROUND_MARKER_SPACING);

    renderer_draw_rect(buffer, 0, ground_y, buffer->width, RENDERER_GROUND_LINE_HEIGHT, 255, 255, 255);

    for (int x = first_x; x < buffer->width; x += RENDERER_GROUND_MARKER_SPACING) {
        renderer_draw_rect(
                buffer,
                x,
                ground_y - RENDERER_GROUND_MARKER_HEIGHT,
                RENDERER_GROUND_MARKER_WIDTH,
                RENDERER_GROUND_MARKER_HEIGHT,
                255,
                255,
                255
        );
    }
}

static void renderer_draw_entities(ANativeWindow_Buffer* buffer, const GameState* game) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        const Entity* entity = game->entities + entity_index;
        uint32_t color = 0xffffffff;

        if (!entity->active) {
            continue;
        }

        if (entity->type == ENTITY_ENEMY && entity->enemyConfig != 0) {
            color = entity->enemyConfig->visual.color;
        } else if (entity->type == ENTITY_OBSTACLE && entity->obstacleConfig != 0) {
            color = entity->obstacleConfig->visual.color;
        }

        renderer_draw_color_rect(
                buffer,
                (int)entity->x,
                (int)entity->y,
                entity_get_width(entity),
                entity_get_height(entity),
                color
        );
    }
}

static void renderer_draw_player(ANativeWindow_Buffer* buffer, const GameState* game) {
    const PlayerConfig* player_config = player_config_get();
    int height = player_config->visual.height;

    if (game->playerSmashing) {
        height += RENDERER_SMASH_EXTRA_HEIGHT;
    }

    renderer_draw_color_rect(
            buffer,
            (int)game->playerX,
            (int)game->playerY,
            player_config->visual.width,
            height,
            player_config->visual.color
    );
}

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
    if (buffer == 0 || buffer->bits == 0 || game == 0) {
        return;
    }

    renderer_clear(buffer);
    renderer_draw_background(buffer, game);
    renderer_draw_ground(buffer, game);
    renderer_draw_entities(buffer, game);
    renderer_draw_player(buffer, game);
    renderer_draw_diagnostics(buffer, game);
}
