#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
        { SPRITE_CMD_CIRCLE, 56, 70, 42, 0, 0, 0xc06e00ff },
        { SPRITE_CMD_RECT, 76, 92, 82, 42, 0, 0xc06e00ff },
        { SPRITE_CMD_CIRCLE, 120, 62, 30, 0, 0, 0xd9872eff },
        { SPRITE_CMD_CIRCLE, 131, 54, 7, 0, 0, 0x111111ff },
        { SPRITE_CMD_RECT, 145, 80, 22, 9, 0, 0xf0e0c0ff },
        { SPRITE_CMD_RECT, 49, 131, 14, 36, 0, 0x4a260cff },
        { SPRITE_CMD_RECT, 95, 131, 14, 36, 0, 0x4a260cff },
};

const SpriteDefinition BOAR_SPRITE = {
        .id = "boar",

        .width = 160,
        .height = 160,

        .pivot_x = 80,
        .pivot_y = 80,

        .commands = COMMANDS,
        .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};
