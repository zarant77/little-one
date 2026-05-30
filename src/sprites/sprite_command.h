#ifndef LITTLE_ONE_SPRITE_COMMAND_H
#define LITTLE_ONE_SPRITE_COMMAND_H

#include <stdint.h>

/* Shared procedural sprite contract with Cat Paint. See SPRITE_FORMAT.md. */
typedef enum {
    SPRITE_CMD_RECT = 0,
    SPRITE_CMD_CIRCLE = 1,
    SPRITE_CMD_TRIANGLE = 2
} SpriteCommandKind;

#pragma pack(push, 1)
typedef struct {
    uint8_t kind;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    int16_t rotation;
    uint32_t color;
} SpriteCommand;
#pragma pack(pop)

typedef char SpriteCommandSizeCheck[(sizeof(SpriteCommand) == 15) ? 1 : -1];

#endif
