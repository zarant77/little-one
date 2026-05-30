#ifndef LITTLE_ONE_RENDERER_H
#define LITTLE_ONE_RENDERER_H

#include <android/native_window.h>

#include "../game/game.h"
#include "../sprites/generated_sprite.h"

typedef ANativeWindow_Buffer Framebuffer;

void renderer_draw_generated_sprite(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int pivot_x,
        int pivot_y
);

void renderer_draw_frame(ANativeWindow_Buffer* buffer, const GameState* game);

#endif
