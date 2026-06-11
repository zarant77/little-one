#pragma once

#include <stdint.h>

#define PACKED_SPRITE_CMD_RECT 0
#define PACKED_SPRITE_CMD_CIRCLE 1
#define PACKED_SPRITE_CMD_TRIANGLE 2

#pragma pack(push, 1)

typedef struct {
    uint8_t kind;
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
    uint8_t rotation;
    uint8_t color_index;
    uint8_t bank_mask;
} PackedSpriteCommand;

typedef struct {
    const char *id;

    uint16_t width;
    uint16_t height;

    uint16_t pivot_x;
    uint16_t pivot_y;

    const PackedSpriteCommand *commands;
    uint16_t command_count;
} PackedSpriteDefinition;

#pragma pack(pop)

extern const uint32_t SPRITE_PALETTE[];
extern const uint16_t SPRITE_PALETTE_COUNT;

extern const PackedSpriteDefinition *PACKED_SPRITE_DEFINITIONS[];
extern const uint16_t PACKED_SPRITE_DEFINITION_COUNT;
