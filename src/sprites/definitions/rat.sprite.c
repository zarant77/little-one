#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
        { SPRITE_CMD_RECT, 55, 38, 76, 26, 0, 0x666666ff },
        { SPRITE_CMD_CIRCLE, 95, 31, 22, 0, 0, 0x777777ff },
        { SPRITE_CMD_CIRCLE, 86, 14, 9, 0, 0, 0x888888ff },
        { SPRITE_CMD_CIRCLE, 108, 15, 8, 0, 0, 0x888888ff },
        { SPRITE_CMD_CIRCLE, 103, 28, 4, 0, 0, 0x111111ff },
        { SPRITE_CMD_RECT, 17, 33, 34, 6, 0, 0x999999ff },
        { SPRITE_CMD_CIRCLE, 5, 33, 6, 0, 0, 0x999999ff },
};

const SpriteDefinition RAT_SPRITE = {
        .id = "rat",

        .width = 128,
        .height = 64,

        .pivot_x = 64,
        .pivot_y = 32,

        .commands = COMMANDS,
        .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};
