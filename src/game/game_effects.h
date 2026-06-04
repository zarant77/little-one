#ifndef LITTLE_ONE_GAME_EFFECTS_H
#define LITTLE_ONE_GAME_EFFECTS_H

#include <stdint.h>

#include "../renderer/renderer.h"

void game_effects_init(void);
void game_effects_spawn_smash_impact(int x, int y, uint32_t seed);
void game_effects_update(int dt_ms);
void game_effects_render(Framebuffer* framebuffer, int shake_x, int shake_y);
void game_effects_clear(void);

#endif
