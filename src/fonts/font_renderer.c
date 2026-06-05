#include "font_renderer.h"

#include <stddef.h>
#include <string.h>

const PackedFont* font_registry_find(const char* id) {
    uint16_t font_index;

    if (id == 0) {
        return 0;
    }

    for (font_index = 0; font_index < PACKED_FONT_DEFINITION_COUNT; ++font_index) {
        const PackedFont* font = PACKED_FONT_DEFINITIONS[font_index];
        if (font != 0 && font->id != 0 && strcmp(font->id, id) == 0) {
            return font;
        }
    }

    return 0;
}

const PackedFontGlyph* font_glyph_find(const PackedFont* font, uint32_t codepoint) {
    uint16_t glyph_index;

    if (font == 0) {
        return 0;
    }

    for (glyph_index = 0; glyph_index < font->glyph_count; ++glyph_index) {
        const PackedFontGlyph* glyph = font->glyphs + glyph_index;
        if (glyph->codepoint == codepoint) {
            return glyph;
        }
    }

    return 0;
}

static uint32_t font_read_utf8_codepoint(const char** text) {
    const unsigned char* cursor;
    unsigned char first;
    uint32_t codepoint;

    if (text == 0 || *text == 0 || **text == '\0') {
        return 0;
    }

    cursor = (const unsigned char*)*text;
    first = cursor[0];
    if (first < 0x80u) {
        *text += 1;
        return (uint32_t)first;
    }

    if ((first & 0xe0u) == 0xc0u
            && (cursor[1] & 0xc0u) == 0x80u) {
        codepoint = ((uint32_t)(first & 0x1fu) << 6)
                | (uint32_t)(cursor[1] & 0x3fu);
        *text += 2;
        return codepoint;
    }

    if ((first & 0xf0u) == 0xe0u
            && (cursor[1] & 0xc0u) == 0x80u
            && (cursor[2] & 0xc0u) == 0x80u) {
        codepoint = ((uint32_t)(first & 0x0fu) << 12)
                | ((uint32_t)(cursor[1] & 0x3fu) << 6)
                | (uint32_t)(cursor[2] & 0x3fu);
        *text += 3;
        return codepoint;
    }

    if ((first & 0xf8u) == 0xf0u
            && (cursor[1] & 0xc0u) == 0x80u
            && (cursor[2] & 0xc0u) == 0x80u
            && (cursor[3] & 0xc0u) == 0x80u) {
        codepoint = ((uint32_t)(first & 0x07u) << 18)
                | ((uint32_t)(cursor[1] & 0x3fu) << 12)
                | ((uint32_t)(cursor[2] & 0x3fu) << 6)
                | (uint32_t)(cursor[3] & 0x3fu);
        *text += 4;
        return codepoint;
    }

    *text += 1;
    return '?';
}

int font_measure_text(const PackedFont* font, int scale, const char* text) {
    int width = 0;

    if (font == 0 || scale <= 0 || text == 0) {
        return 0;
    }

    while (*text != '\0') {
        uint32_t codepoint = font_read_utf8_codepoint(&text);
        const PackedFontGlyph* glyph;

        if (codepoint == 0) {
            continue;
        }

        if (codepoint == ' ') {
            width += (int)font->default_advance * scale;
            continue;
        }

        glyph = font_glyph_find(font, codepoint);
        if (glyph == 0) {
            glyph = font_glyph_find(font, '?');
        }

        if (glyph != 0) {
            width += (int)glyph->advance * scale;
        }
    }

    return width;
}

static void font_draw_scaled_point(
        Framebuffer* framebuffer,
        int x,
        int y,
        int scale,
        uint32_t color
) {
    renderer_draw_color_rect(framebuffer, x, y, scale, scale, color);
}

static int font_abs_int(int value) {
    return value < 0 ? -value : value;
}

static int font_sign_int(int value) {
    if (value < 0) {
        return -1;
    }
    if (value > 0) {
        return 1;
    }
    return 0;
}

static void font_draw_line(
        Framebuffer* framebuffer,
        int x1,
        int y1,
        int x2,
        int y2,
        int scale,
        uint32_t color
) {
    int dx = font_abs_int(x2 - x1);
    int dy = font_abs_int(y2 - y1);
    int step_x = font_sign_int(x2 - x1);
    int step_y = font_sign_int(y2 - y1);
    int error = dx - dy;

    for (;;) {
        int error2;

        font_draw_scaled_point(framebuffer, x1, y1, scale, color);
        if (x1 == x2 && y1 == y2) {
            break;
        }

        error2 = error * 2;
        if (error2 > -dy) {
            error -= dy;
            x1 += step_x * scale;
        }
        if (error2 < dx) {
            error += dx;
            y1 += step_y * scale;
        }
    }
}

static void font_draw_glyph(
        Framebuffer* framebuffer,
        const PackedFontGlyph* glyph,
        int x,
        int y,
        int scale,
        uint32_t color
) {
    uint8_t line_index;

    for (line_index = 0; line_index < glyph->line_count; ++line_index) {
        PackedFontLine line = glyph->lines[line_index];
        int x1 = x + (int)((line >> 12) & 0x0f) * scale;
        int y1 = y + (int)((line >> 8) & 0x0f) * scale;
        int x2 = x + (int)((line >> 4) & 0x0f) * scale;
        int y2 = y + (int)(line & 0x0f) * scale;

        font_draw_line(framebuffer, x1, y1, x2, y2, scale, color);
    }
}

void font_draw_text(
        Framebuffer* framebuffer,
        const PackedFont* font,
        int x,
        int y,
        int scale,
        uint32_t color,
        const char* text
) {
    int cursor_x = x;

    if (framebuffer == 0 || font == 0 || scale <= 0 || text == 0) {
        return;
    }

    while (*text != '\0') {
        uint32_t codepoint = font_read_utf8_codepoint(&text);
        const PackedFontGlyph* glyph;

        if (codepoint == 0) {
            continue;
        }

        if (codepoint == ' ') {
            cursor_x += (int)font->default_advance * scale;
            continue;
        }

        glyph = font_glyph_find(font, codepoint);
        if (glyph == 0) {
            glyph = font_glyph_find(font, '?');
        }

        if (glyph != 0) {
            font_draw_glyph(framebuffer, glyph, cursor_x, y, scale, color);
            cursor_x += (int)glyph->advance * scale;
        }
    }
}
