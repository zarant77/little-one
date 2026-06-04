#ifndef LITTLE_ONE_UI_CONTROLS_H
#define LITTLE_ONE_UI_CONTROLS_H

#include <stdint.h>

#include "../fonts/font_renderer.h"
#include "../renderer/renderer.h"

typedef struct {
    int x;
    int y;
    int width;
    int height;
} UiRect;

int ui_rect_contains(const UiRect* rect, int x, int y);
void ui_draw_panel(Framebuffer* framebuffer, UiRect rect);
void ui_draw_button(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect rect,
        const char* label
);
void ui_draw_label(
        Framebuffer* framebuffer,
        const PackedFont* font,
        int x,
        int y,
        int scale,
        uint32_t color,
        const char* label
);
void ui_draw_slider(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect rect,
        const char* label,
        int value
);
int ui_slider_value_from_touch(UiRect rect, int touch_x);

#endif
