#ifndef LITTLE_ONE_MENU_H
#define LITTLE_ONE_MENU_H

#include "../game/game.h"
#include "../renderer/renderer.h"

void menu_initialize(void);
void menu_render(Framebuffer* framebuffer, const GameState* game);
int menu_handle_touch(GameState* game, int action_type, int pointer_id, int x, int y);

#endif
