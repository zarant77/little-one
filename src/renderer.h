#ifndef LITTLE_ONE_RENDERER_H
#define LITTLE_ONE_RENDERER_H

#include <android/native_window.h>

#include "generated_sprite.h"
#include "game.h"

typedef ANativeWindow_Buffer Framebuffer;

void renderer_draw_generated_sprite(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        int x,
        int y
);

void renderer_draw_frame(ANativeWindow_Buffer* buffer, const GameState* game);

#endif
