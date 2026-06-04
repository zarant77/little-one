#ifndef LITTLE_ONE_FONT_RENDERER_H
#define LITTLE_ONE_FONT_RENDERER_H

#include <stdint.h>

#include "fonts_packed.h"
#include "../renderer/renderer.h"

const PackedFont* font_registry_find(const char* id);
const PackedFontGlyph* font_glyph_find(const PackedFont* font, uint32_t codepoint);

void font_draw_text(
        Framebuffer* framebuffer,
        const PackedFont* font,
        int x,
        int y,
        int scale,
        uint32_t color,
        const char* text
);

#endif
