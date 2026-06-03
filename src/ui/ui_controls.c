#include "ui_controls.h"

#include <stdio.h>

#define UI_COLOR_PANEL 0x293241ee
#define UI_COLOR_PANEL_BORDER 0xf2cc8fff
#define UI_COLOR_BUTTON 0x3d405bff
#define UI_COLOR_BUTTON_BORDER 0x81b29aff
#define UI_COLOR_TEXT 0xffffffff
#define UI_COLOR_TEXT_SHADOW 0x00000099
#define UI_COLOR_SLIDER_TRACK 0x111827ff
#define UI_COLOR_SLIDER_FILL 0x81b29aff
#define UI_COLOR_SLIDER_HANDLE 0xf4f1deff
#define UI_PANEL_BORDER_SIZE 4
#define UI_BUTTON_BORDER_SIZE 3
#define UI_SLIDER_TRACK_HEIGHT 14
#define UI_SLIDER_HANDLE_WIDTH 30

static int ui_clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

static int ui_text_width(const PackedFont* font, int scale, const char* text)
{
    int width = 0;

    if (font == 0 || scale <= 0 || text == 0)
    {
        return 0;
    }

    for (int index = 0; text[index] != 0; ++index)
    {
        width += (int)font->default_advance * scale;
    }

    return width;
}

static void ui_draw_rect_outline(Framebuffer* framebuffer, UiRect rect, int thickness, uint32_t color)
{
    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, thickness, color);
    renderer_draw_color_rect(
            framebuffer,
            rect.x,
            rect.y + rect.height - thickness,
            rect.width,
            thickness,
            color
    );
    renderer_draw_color_rect(framebuffer, rect.x, rect.y, thickness, rect.height, color);
    renderer_draw_color_rect(
            framebuffer,
            rect.x + rect.width - thickness,
            rect.y,
            thickness,
            rect.height,
            color
    );
}

int ui_rect_contains(const UiRect* rect, int x, int y)
{
    if (rect == 0)
    {
        return 0;
    }

    return x >= rect->x
            && y >= rect->y
            && x < rect->x + rect->width
            && y < rect->y + rect->height;
}

void ui_draw_panel(Framebuffer* framebuffer, UiRect rect)
{
    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, rect.height, UI_COLOR_PANEL);
    ui_draw_rect_outline(framebuffer, rect, UI_PANEL_BORDER_SIZE, UI_COLOR_PANEL_BORDER);
}

void ui_draw_label(
        Framebuffer* framebuffer,
        const PackedFont* font,
        int x,
        int y,
        int scale,
        uint32_t color,
        const char* label
)
{
    font_draw_text(framebuffer, font, x + 1, y + 1, scale, UI_COLOR_TEXT_SHADOW, label);
    font_draw_text(framebuffer, font, x, y, scale, color, label);
}

void ui_draw_button(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect rect,
        const char* label
)
{
    int scale = 2;
    int text_width;
    int text_height;
    int text_x;
    int text_y;

    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, rect.height, UI_COLOR_BUTTON);
    ui_draw_rect_outline(framebuffer, rect, UI_BUTTON_BORDER_SIZE, UI_COLOR_BUTTON_BORDER);

    if (font == 0 || label == 0)
    {
        return;
    }

    text_width = ui_text_width(font, scale, label);
    text_height = (int)font->grid_size * scale;
    text_x = rect.x + (rect.width - text_width) / 2;
    text_y = rect.y + (rect.height - text_height) / 2;

    ui_draw_label(framebuffer, font, text_x, text_y, scale, UI_COLOR_TEXT, label);
}

void ui_draw_slider(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect rect,
        const char* label,
        int value
)
{
    char value_text[8];
    int scale = 2;
    int label_y = rect.y;
    int track_y = rect.y + rect.height - 30;
    int track_x = rect.x;
    int track_width = rect.width;
    int fill_width;
    int handle_x;

    value = ui_clamp_int(value, 0, 100);
    fill_width = (track_width * value) / 100;
    handle_x = track_x + fill_width - UI_SLIDER_HANDLE_WIDTH / 2;
    handle_x = ui_clamp_int(handle_x, track_x, track_x + track_width - UI_SLIDER_HANDLE_WIDTH);

    ui_draw_label(framebuffer, font, rect.x, label_y, scale, UI_COLOR_TEXT, label);
    snprintf(value_text, sizeof(value_text), "%d", value);
    ui_draw_label(
            framebuffer,
            font,
            rect.x + rect.width - ui_text_width(font, scale, value_text),
            label_y,
            scale,
            UI_COLOR_TEXT,
            value_text
    );

    renderer_draw_color_rect(
            framebuffer,
            track_x,
            track_y,
            track_width,
            UI_SLIDER_TRACK_HEIGHT,
            UI_COLOR_SLIDER_TRACK
    );
    renderer_draw_color_rect(
            framebuffer,
            track_x,
            track_y,
            fill_width,
            UI_SLIDER_TRACK_HEIGHT,
            UI_COLOR_SLIDER_FILL
    );
    renderer_draw_color_rect(
            framebuffer,
            handle_x,
            track_y - 14,
            UI_SLIDER_HANDLE_WIDTH,
            UI_SLIDER_TRACK_HEIGHT + 28,
            UI_COLOR_SLIDER_HANDLE
    );
}

int ui_slider_value_from_touch(UiRect rect, int touch_x)
{
    int local_x;

    if (rect.width <= 0)
    {
        return 0;
    }

    local_x = ui_clamp_int(touch_x - rect.x, 0, rect.width);
    return (local_x * 100 + rect.width / 2) / rect.width;
}
