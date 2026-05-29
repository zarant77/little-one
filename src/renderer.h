#ifndef LITTLE_ONE_RENDERER_H
#define LITTLE_ONE_RENDERER_H

#include <android/native_window.h>

#include "game.h"

void renderer_draw_frame(ANativeWindow_Buffer* buffer, const GameState* game);

#endif
