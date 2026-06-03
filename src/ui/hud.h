#ifndef LITTLE_ONE_HUD_H
#define LITTLE_ONE_HUD_H

#include "../game/game.h"
#include "../renderer/renderer.h"

void hud_initialize(void);
void hud_render(Framebuffer* framebuffer, const GameState* game);

#endif
