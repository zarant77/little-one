#include "game_effects.h"

#include <stdint.h>

#define EFFECTS_MAX_ACTIVE 16

#define SMASH_IMPACT_LIFETIME_MS 90
#define SMASH_IMPACT_RAY_COUNT 8
#define SMASH_IMPACT_RAY_LENGTH_START 10
#define SMASH_IMPACT_RAY_LENGTH_END 55
#define SMASH_IMPACT_RAY_DISTANCE_END 30
#define SMASH_IMPACT_RAY_WIDTH 5
#define SMASH_IMPACT_RAY_LENGTH_VARIATION_PERCENT 35

#define SMASH_FLASH_LIFETIME_MS 45
#define SMASH_FLASH_RADIUS_START 8
#define SMASH_FLASH_RADIUS_END 28
#define SMASH_FLASH_WIDTH 7

#define SMASH_DUST_COUNT 8
#define SMASH_DUST_LIFETIME_MS 110
#define SMASH_DUST_RADIUS_START 8
#define SMASH_DUST_RADIUS_END 3
#define SMASH_DUST_SPREAD_X 70
#define SMASH_DUST_RISE_Y 25
#define SMASH_DUST_GROUND_JITTER_Y 10
#define SMASH_DUST_SIZE_VARIATION_PERCENT 30

#define SMASH_SHOCKWAVE_ENABLED 1
#define SMASH_SHOCKWAVE_LIFETIME_MS 80
#define SMASH_SHOCKWAVE_WIDTH_START 25
#define SMASH_SHOCKWAVE_WIDTH_END 130
#define SMASH_SHOCKWAVE_HEIGHT_START 8
#define SMASH_SHOCKWAVE_HEIGHT_END 25
#define SMASH_SHOCKWAVE_LINE_WIDTH 5

#define SMASH_IMPACT_COLOR 0xffffffff
#define SMASH_FLASH_COLOR 0xffe6a0ff
#define SMASH_DUST_COLOR 0xc8b48cff
#define SMASH_DUST_COLOR_ALT 0x8f7658ff
#define SMASH_SHOCKWAVE_COLOR 0xffffffff

#define EFFECT_DIRECTION_SCALE 256
#define EFFECT_DIRECTION_COUNT 16

typedef enum {
    GAME_EFFECT_NONE = 0,
    GAME_EFFECT_SMASH_IMPACT = 1
} GameEffectKind;

typedef struct {
    GameEffectKind kind;
    int x;
    int y;
    int age_ms;
    int lifetime_ms;
    uint32_t seed;
    int active;
} GameEffect;

static const int EFFECT_DIRECTIONS_X[EFFECT_DIRECTION_COUNT] = {
        256, 237, 181, 98, 0, -98, -181, -237,
        -256, -237, -181, -98, 0, 98, 181, 237,
};

static const int EFFECT_DIRECTIONS_Y[EFFECT_DIRECTION_COUNT] = {
        0, 98, 181, 237, 256, 237, 181, 98,
        0, -98, -181, -237, -256, -237, -181, -98,
};

static GameEffect game_effects[EFFECTS_MAX_ACTIVE];

static int effect_smash_lifetime_ms(void) {
    int lifetime_ms = SMASH_IMPACT_LIFETIME_MS;

    if (SMASH_FLASH_LIFETIME_MS > lifetime_ms) {
        lifetime_ms = SMASH_FLASH_LIFETIME_MS;
    }
    if (SMASH_DUST_LIFETIME_MS > lifetime_ms) {
        lifetime_ms = SMASH_DUST_LIFETIME_MS;
    }
    #if SMASH_SHOCKWAVE_ENABLED
    if (SMASH_SHOCKWAVE_LIFETIME_MS > lifetime_ms) {
        lifetime_ms = SMASH_SHOCKWAVE_LIFETIME_MS;
    }
    #endif

    return lifetime_ms;
}

static int effect_lerp(int start, int end, int age_ms, int lifetime_ms) {
    if (age_ms <= 0 || lifetime_ms <= 0) {
        return start;
    }
    if (age_ms >= lifetime_ms) {
        return end;
    }

    return start + ((end - start) * age_ms) / lifetime_ms;
}

static int effect_ease_out_age(int age_ms, int lifetime_ms) {
    int remaining_ms;

    if (age_ms <= 0 || lifetime_ms <= 0) {
        return 0;
    }
    if (age_ms >= lifetime_ms) {
        return lifetime_ms;
    }

    remaining_ms = lifetime_ms - age_ms;
    return lifetime_ms - remaining_ms * remaining_ms / lifetime_ms;
}

static uint32_t effect_fade_color(uint32_t color, int age_ms, int lifetime_ms) {
    int source_alpha = (int)(color & 0xffu);
    int alpha;

    if (lifetime_ms <= 0 || age_ms >= lifetime_ms) {
        return color & 0xffffff00u;
    }

    alpha = source_alpha * (lifetime_ms - age_ms) / lifetime_ms;
    return (color & 0xffffff00u) | (uint32_t)alpha;
}

static uint32_t effect_random(uint32_t seed, uint32_t index) {
    uint32_t value = seed ^ (index * 0x9e3779b9u);

    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;

    return value;
}

static void effect_draw_line(
        Framebuffer* framebuffer,
        int x0,
        int y0,
        int x1,
        int y1,
        int width,
        uint32_t color
) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    int half_width = width / 2;

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
            int error2 = error * 2;

            if (error2 >= dy) {
                error += dy;
                x0 += sx;
            }
            if (error2 <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    }
}

static void effect_draw_smash_rays(
        Framebuffer* framebuffer,
        const GameEffect* effect,
        int shake_x,
        int shake_y
) {
    int motion_age = effect_ease_out_age(effect->age_ms, SMASH_IMPACT_LIFETIME_MS);
    int ray_length = effect_lerp(
            SMASH_IMPACT_RAY_LENGTH_START,
            SMASH_IMPACT_RAY_LENGTH_END,
            motion_age,
            SMASH_IMPACT_LIFETIME_MS
    );
    int ray_distance = effect_lerp(
            0,
            SMASH_IMPACT_RAY_DISTANCE_END,
            motion_age,
            SMASH_IMPACT_LIFETIME_MS
    );
    uint32_t color = effect_fade_color(
            SMASH_IMPACT_COLOR,
            effect->age_ms,
            SMASH_IMPACT_LIFETIME_MS
    );
    int direction_offset = (int)(effect->seed % EFFECT_DIRECTION_COUNT);

    for (int ray_index = 0; ray_index < SMASH_IMPACT_RAY_COUNT; ++ray_index) {
        uint32_t random = effect_random(effect->seed, (uint32_t)ray_index + 20u);
        int direction_jitter = (int)(random % 3u) - 1;
        int direction_index = (
                direction_offset + ray_index * EFFECT_DIRECTION_COUNT / SMASH_IMPACT_RAY_COUNT
                + direction_jitter
                + EFFECT_DIRECTION_COUNT
        ) % EFFECT_DIRECTION_COUNT;
        int direction_x = EFFECT_DIRECTIONS_X[direction_index];
        int direction_y = EFFECT_DIRECTIONS_Y[direction_index];
        int varied_length = ray_length * (
                100 - SMASH_IMPACT_RAY_LENGTH_VARIATION_PERCENT
                + (int)((random >> 8) % (SMASH_IMPACT_RAY_LENGTH_VARIATION_PERCENT * 2 + 1))
        ) / 100;
        int start_x = effect->x + shake_x
                + direction_x * ray_distance / EFFECT_DIRECTION_SCALE;
        int start_y = effect->y + shake_y
                + direction_y * ray_distance / EFFECT_DIRECTION_SCALE;
        int end_distance = ray_distance + varied_length;
        int end_x = effect->x + shake_x
                + direction_x * end_distance / EFFECT_DIRECTION_SCALE;
        int end_y = effect->y + shake_y
                + direction_y * end_distance / EFFECT_DIRECTION_SCALE;

        effect_draw_line(
                framebuffer,
                start_x,
                start_y,
                end_x,
                end_y,
                SMASH_IMPACT_RAY_WIDTH,
                color
        );
    }
}

static void effect_draw_smash_flash(
        Framebuffer* framebuffer,
        const GameEffect* effect,
        int shake_x,
        int shake_y
) {
    int motion_age = effect_ease_out_age(effect->age_ms, SMASH_FLASH_LIFETIME_MS);
    int radius = effect_lerp(
            SMASH_FLASH_RADIUS_START,
            SMASH_FLASH_RADIUS_END,
            motion_age,
            SMASH_FLASH_LIFETIME_MS
    );
    uint32_t color = effect_fade_color(
            SMASH_FLASH_COLOR,
            effect->age_ms,
            SMASH_FLASH_LIFETIME_MS
    );
    int center_x = effect->x + shake_x;
    int center_y = effect->y + shake_y;

    effect_draw_line(
            framebuffer,
            center_x - radius,
            center_y,
            center_x + radius,
            center_y,
            SMASH_FLASH_WIDTH,
            color
    );
    effect_draw_line(
            framebuffer,
            center_x,
            center_y - radius,
            center_x,
            center_y + radius,
            SMASH_FLASH_WIDTH,
            color
    );
    effect_draw_line(
            framebuffer,
            center_x - radius / 2,
            center_y - radius / 2,
            center_x + radius / 2,
            center_y + radius / 2,
            SMASH_FLASH_WIDTH,
            color
    );
    effect_draw_line(
            framebuffer,
            center_x + radius / 2,
            center_y - radius / 2,
            center_x - radius / 2,
            center_y + radius / 2,
            SMASH_FLASH_WIDTH,
            color
    );
}

static void effect_draw_smash_dust(
        Framebuffer* framebuffer,
        const GameEffect* effect,
        int shake_x,
        int shake_y
) {
    int motion_age = effect_ease_out_age(effect->age_ms, SMASH_DUST_LIFETIME_MS);
    int radius = effect_lerp(
            SMASH_DUST_RADIUS_START,
            SMASH_DUST_RADIUS_END,
            effect->age_ms,
            SMASH_DUST_LIFETIME_MS
    );

    for (int dust_index = 0; dust_index < SMASH_DUST_COUNT; ++dust_index) {
        uint32_t random = effect_random(effect->seed, (uint32_t)dust_index + 1u);
        int signed_x = (int)(random & 0x1ffu) - 256;
        int rise_scale = 128 + (int)((random >> 9) & 0x7fu);
        int ground_jitter = (int)((random >> 16) % (SMASH_DUST_GROUND_JITTER_Y + 1));
        int particle_radius = radius * (
                100 - SMASH_DUST_SIZE_VARIATION_PERCENT
                + (int)((random >> 20) % (SMASH_DUST_SIZE_VARIATION_PERCENT * 2 + 1))
        ) / 100;
        uint32_t base_color = (random & 0x80000000u) != 0u
                ? SMASH_DUST_COLOR_ALT
                : SMASH_DUST_COLOR;
        uint32_t color = effect_fade_color(
                base_color,
                effect->age_ms,
                SMASH_DUST_LIFETIME_MS
        );
        int x_offset = signed_x * SMASH_DUST_SPREAD_X * motion_age
                / (256 * SMASH_DUST_LIFETIME_MS);
        int y_offset = rise_scale * SMASH_DUST_RISE_Y * motion_age
                / (256 * SMASH_DUST_LIFETIME_MS);
        int draw_x = effect->x + shake_x + x_offset - particle_radius;
        int draw_y = effect->y + shake_y - y_offset - ground_jitter - particle_radius;

        renderer_draw_color_rect(
                framebuffer,
                draw_x,
                draw_y,
                particle_radius * 2,
                particle_radius * 2,
                color
        );
    }
}

#if SMASH_SHOCKWAVE_ENABLED
static void effect_draw_smash_shockwave(
        Framebuffer* framebuffer,
        const GameEffect* effect,
        int shake_x,
        int shake_y
) {
    int motion_age = effect_ease_out_age(effect->age_ms, SMASH_SHOCKWAVE_LIFETIME_MS);
    int width = effect_lerp(
            SMASH_SHOCKWAVE_WIDTH_START,
            SMASH_SHOCKWAVE_WIDTH_END,
            motion_age,
            SMASH_SHOCKWAVE_LIFETIME_MS
    );
    int height = effect_lerp(
            SMASH_SHOCKWAVE_HEIGHT_START,
            SMASH_SHOCKWAVE_HEIGHT_END,
            motion_age,
            SMASH_SHOCKWAVE_LIFETIME_MS
    );
    uint32_t color = effect_fade_color(
            SMASH_SHOCKWAVE_COLOR,
            effect->age_ms,
            SMASH_SHOCKWAVE_LIFETIME_MS
    );
    int half_width = width / 2;
    int half_height = height / 2;
    int center_x = effect->x + shake_x;
    int center_y = effect->y + shake_y;

    for (int direction_index = 0; direction_index < EFFECT_DIRECTION_COUNT; ++direction_index) {
        int next_index = (direction_index + 1) % EFFECT_DIRECTION_COUNT;
        int x0 = center_x
                + EFFECT_DIRECTIONS_X[direction_index] * half_width / EFFECT_DIRECTION_SCALE;
        int y0 = center_y
                + EFFECT_DIRECTIONS_Y[direction_index] * half_height / EFFECT_DIRECTION_SCALE;
        int x1 = center_x
                + EFFECT_DIRECTIONS_X[next_index] * half_width / EFFECT_DIRECTION_SCALE;
        int y1 = center_y
                + EFFECT_DIRECTIONS_Y[next_index] * half_height / EFFECT_DIRECTION_SCALE;

        effect_draw_line(
                framebuffer,
                x0,
                y0,
                x1,
                y1,
                SMASH_SHOCKWAVE_LINE_WIDTH,
                color
        );
    }
}
#endif

static void effect_draw_smash_impact(
        Framebuffer* framebuffer,
        const GameEffect* effect,
        int shake_x,
        int shake_y
) {
    if (effect->age_ms < SMASH_FLASH_LIFETIME_MS) {
        effect_draw_smash_flash(framebuffer, effect, shake_x, shake_y);
    }
    if (effect->age_ms < SMASH_IMPACT_LIFETIME_MS) {
        effect_draw_smash_rays(framebuffer, effect, shake_x, shake_y);
    }
    if (effect->age_ms < SMASH_DUST_LIFETIME_MS) {
        effect_draw_smash_dust(framebuffer, effect, shake_x, shake_y);
    }
    #if SMASH_SHOCKWAVE_ENABLED
    if (effect->age_ms < SMASH_SHOCKWAVE_LIFETIME_MS) {
        effect_draw_smash_shockwave(framebuffer, effect, shake_x, shake_y);
    }
    #endif
}

void game_effects_init(void) {
    game_effects_clear();
}

void game_effects_spawn_smash_impact(int x, int y, uint32_t seed) {
    GameEffect* available = 0;

    for (int effect_index = 0; effect_index < EFFECTS_MAX_ACTIVE; ++effect_index) {
        GameEffect* effect = game_effects + effect_index;

        if (!effect->active) {
            available = effect;
            break;
        }
        if (available == 0 || effect->age_ms > available->age_ms) {
            available = effect;
        }
    }

    if (available == 0) {
        return;
    }

    available->kind = GAME_EFFECT_SMASH_IMPACT;
    available->x = x;
    available->y = y;
    available->age_ms = 0;
    available->lifetime_ms = effect_smash_lifetime_ms();
    available->seed = seed != 0u ? seed : 1u;
    available->active = 1;
}

void game_effects_update(int dt_ms) {
    if (dt_ms < 0) {
        dt_ms = 0;
    }

    for (int effect_index = 0; effect_index < EFFECTS_MAX_ACTIVE; ++effect_index) {
        GameEffect* effect = game_effects + effect_index;

        if (!effect->active) {
            continue;
        }

        effect->age_ms += dt_ms;
        if (effect->age_ms >= effect->lifetime_ms) {
            effect->active = 0;
        }
    }
}

void game_effects_render(Framebuffer* framebuffer, int shake_x, int shake_y) {
    if (framebuffer == 0 || framebuffer->bits == 0) {
        return;
    }

    for (int effect_index = 0; effect_index < EFFECTS_MAX_ACTIVE; ++effect_index) {
        const GameEffect* effect = game_effects + effect_index;

        if (!effect->active) {
            continue;
        }

        if (effect->kind == GAME_EFFECT_SMASH_IMPACT) {
            effect_draw_smash_impact(framebuffer, effect, shake_x, shake_y);
        }
    }
}

void game_effects_clear(void) {
    for (int effect_index = 0; effect_index < EFFECTS_MAX_ACTIVE; ++effect_index) {
        game_effects[effect_index].kind = GAME_EFFECT_NONE;
        game_effects[effect_index].x = 0;
        game_effects[effect_index].y = 0;
        game_effects[effect_index].age_ms = 0;
        game_effects[effect_index].lifetime_ms = 0;
        game_effects[effect_index].seed = 0u;
        game_effects[effect_index].active = 0;
    }
}
