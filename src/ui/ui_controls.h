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

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} UiNineSlice;

int ui_rect_contains(const UiRect* rect, int x, int y);
void ui_draw_panel(Framebuffer* framebuffer, UiRect rect);
void ui_draw_nine_slice_panel(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        UiRect rect,
        UiNineSlice slice
);
void ui_draw_button(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect rect,
        const char* label
);
void ui_draw_button_colored(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect rect,
        const char* label,
        uint32_t fill_color,
        uint32_t border_color
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
