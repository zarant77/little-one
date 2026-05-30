#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
        { SPRITE_CMD_RECT, 40, 101, 58, 136, 0, 0x9b4f18ff },
        { SPRITE_CMD_CIRCLE, 40, 34, 35, 0, 0, 0xcd7a2aff },
        { SPRITE_CMD_CIRCLE, 40, 34, 22, 0, 0, 0x8a4514ff },
        { SPRITE_CMD_CIRCLE, 40, 34, 10, 0, 0, 0x5c2e0dff },
        { SPRITE_CMD_RECT, 29, 100, 8, 98, 0, 0x6b340fff },
        { SPRITE_CMD_RECT, 52, 102, 7, 84, 0, 0x6b340fff },
        { SPRITE_CMD_RECT, 19, 167, 32, 14, 0, 0x6b340fff },
        { SPRITE_CMD_RECT, 61, 167, 32, 14, 0, 0x6b340fff },
};

const SpriteDefinition STUMP_SPRITE = {
        .id = "stump",

        .width = 80,
        .height = 180,

        .pivot_x = 40,
        .pivot_y = 90,

        .commands = COMMANDS,
        .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};
