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
#define UI_PANEL_SKIN_ID "panel_bg"
#define UI_PANEL_SLICE_LEFT 24
#define UI_PANEL_SLICE_TOP 24
#define UI_PANEL_SLICE_RIGHT 24
#define UI_PANEL_SLICE_BOTTOM 24

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
    return font_measure_text(font, scale, text);
}

static int ui_min_int(int a, int b)
{
    return a < b ? a : b;
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
    const GeneratedSprite* sprite = generated_sprite_get_by_id(UI_PANEL_SKIN_ID);
    UiNineSlice slice = {
        .left = UI_PANEL_SLICE_LEFT,
        .top = UI_PANEL_SLICE_TOP,
        .right = UI_PANEL_SLICE_RIGHT,
        .bottom = UI_PANEL_SLICE_BOTTOM,
    };

    if (sprite != 0)
    {
        ui_draw_nine_slice_panel(framebuffer, sprite, rect, slice);
        return;
    }

    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, rect.height, UI_COLOR_PANEL);
    ui_draw_rect_outline(framebuffer, rect, UI_PANEL_BORDER_SIZE, UI_COLOR_PANEL_BORDER);
}

void ui_draw_nine_slice_panel(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        UiRect rect,
        UiNineSlice slice
)
{
    int left;
    int top;
    int right;
    int bottom;
    int src_left;
    int src_top;
    int src_right;
    int src_bottom;
    int center_src_w;
    int center_src_h;
    int center_dst_w;
    int center_dst_h;

    if (framebuffer == 0
            || sprite == 0
            || sprite->pixels == 0
            || sprite->width <= 0
            || sprite->height <= 0
            || rect.width <= 0
            || rect.height <= 0)
    {
        return;
    }

    src_left = ui_clamp_int(slice.left, 0, sprite->width);
    src_top = ui_clamp_int(slice.top, 0, sprite->height);
    src_right = ui_clamp_int(slice.right, 0, sprite->width - src_left);
    src_bottom = ui_clamp_int(slice.bottom, 0, sprite->height - src_top);

    left = ui_min_int(src_left, rect.width / 2);
    right = ui_min_int(src_right, rect.width - left);
    top = ui_min_int(src_top, rect.height / 2);
    bottom = ui_min_int(src_bottom, rect.height - top);

    center_src_w = sprite->width - src_left - src_right;
    center_src_h = sprite->height - src_top - src_bottom;
    if (center_src_w < 1)
    {
        center_src_w = 1;
    }
    if (center_src_h < 1)
    {
        center_src_h = 1;
    }

    center_dst_w = rect.width - left - right;
    center_dst_h = rect.height - top - bottom;

    renderer_draw_generated_sprite_region_scaled(
            framebuffer,
            sprite,
            0,
            0,
            src_left,
            src_top,
            rect.x,
            rect.y,
            left,
            top
    );
    renderer_draw_generated_sprite_region_scaled(
            framebuffer,
            sprite,
            sprite->width - src_right,
            0,
            src_right,
            src_top,
            rect.x + rect.width - right,
            rect.y,
            right,
            top
    );
    renderer_draw_generated_sprite_region_scaled(
            framebuffer,
            sprite,
            0,
            sprite->height - src_bottom,
            src_left,
            src_bottom,
            rect.x,
            rect.y + rect.height - bottom,
            left,
            bottom
    );
    renderer_draw_generated_sprite_region_scaled(
            framebuffer,
            sprite,
            sprite->width - src_right,
            sprite->height - src_bottom,
            src_right,
            src_bottom,
            rect.x + rect.width - right,
            rect.y + rect.height - bottom,
            right,
            bottom
    );

    if (center_dst_w > 0)
    {
        renderer_draw_generated_sprite_region_scaled(
                framebuffer,
                sprite,
                src_left,
                0,
                center_src_w,
                src_top,
                rect.x + left,
                rect.y,
                center_dst_w,
                top
        );
        renderer_draw_generated_sprite_region_scaled(
                framebuffer,
                sprite,
                src_left,
                sprite->height - src_bottom,
                center_src_w,
                src_bottom,
                rect.x + left,
                rect.y + rect.height - bottom,
                center_dst_w,
                bottom
        );
    }

    if (center_dst_h > 0)
    {
        renderer_draw_generated_sprite_region_scaled(
                framebuffer,
                sprite,
                0,
                src_top,
                src_left,
                center_src_h,
                rect.x,
                rect.y + top,
                left,
                center_dst_h
        );
        renderer_draw_generated_sprite_region_scaled(
                framebuffer,
                sprite,
                sprite->width - src_right,
                src_top,
                src_right,
                center_src_h,
                rect.x + rect.width - right,
                rect.y + top,
                right,
                center_dst_h
        );
    }

    if (center_dst_w > 0 && center_dst_h > 0)
    {
        renderer_draw_generated_sprite_region_scaled(
                framebuffer,
                sprite,
                src_left,
                src_top,
                center_src_w,
                center_src_h,
                rect.x + left,
                rect.y + top,
                center_dst_w,
                center_dst_h
        );
    }
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
    ui_draw_button_colored(
            framebuffer,
            font,
            rect,
            label,
            UI_COLOR_BUTTON,
            UI_COLOR_BUTTON_BORDER
    );
}

void ui_draw_button_colored(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect rect,
        const char* label,
        uint32_t fill_color,
        uint32_t border_color
)
{
    int scale = 2;
    int max_text_width = rect.width - 16;
    int text_width;
    int text_height;
    int text_x;
    int text_y;

    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, rect.height, fill_color);
    ui_draw_rect_outline(framebuffer, rect, UI_BUTTON_BORDER_SIZE, border_color);

    if (font == 0 || label == 0)
    {
        return;
    }

    while (scale > 1 && ui_text_width(font, scale, label) > max_text_width)
    {
        scale -= 1;
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
