#pragma once

#include <stdint.h>

typedef uint16_t PackedFontLine;

typedef struct {
    uint32_t codepoint;
    uint8_t advance;
    const PackedFontLine *lines;
    uint8_t line_count;
} PackedFontGlyph;

typedef struct {
    const char *id;

    uint8_t grid_size;
    uint8_t default_advance;
    uint8_t line_thickness;

    const PackedFontGlyph *glyphs;
    uint16_t glyph_count;
} PackedFont;

extern const PackedFont *PACKED_FONT_DEFINITIONS[];
extern const uint16_t PACKED_FONT_DEFINITION_COUNT;
