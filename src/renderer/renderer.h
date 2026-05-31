#ifndef LITTLE_ONE_RENDERER_H
#define LITTLE_ONE_RENDERER_H

#include <android/native_window.h>

#include "../config/background_config.h"
#include "../game/game.h"
#include "../sprites/generated_sprite.h"

typedef ANativeWindow_Buffer Framebuffer;

typedef enum {
    SPRITE_FIT_STRETCH = 0,
    SPRITE_FIT_CONTAIN = 1,
    SPRITE_FIT_COVER = 2
} SpriteFitMode;

void renderer_draw_generated_sprite_fit(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height,
        SpriteFitMode fit_mode
);

void renderer_draw_generated_sprite_scaled(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int dst_x,
        int dst_y,
        int dst_width,
        int dst_height
);

void renderer_fill_vertical_gradient(
        Framebuffer* framebuffer,
        uint32_t top_color,
        uint32_t bottom_color
);

void render_background(
        Framebuffer* framebuffer,
        const BackgroundConfig* background,
        int32_t world_scroll_x,
        int32_t gameplay_ground_y,
        int32_t shake_x,
        int32_t shake_y
);

void render_ground(
        Framebuffer* framebuffer,
        const GroundVisualConfig* ground,
        int32_t world_scroll_x,
        int32_t gameplay_ground_y,
        int32_t shake_x,
        int32_t shake_y
);

void renderer_draw_frame(ANativeWindow_Buffer* buffer, const GameState* game);

#endif
