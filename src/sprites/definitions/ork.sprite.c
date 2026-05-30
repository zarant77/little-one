#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
        { SPRITE_CMD_RECT, 52, 39, 30, 36, 0, 0x79c753ff },
        { SPRITE_CMD_RECT, 148, 39, 30, 36, 0, 0x79c753ff },
        { SPRITE_CMD_RECT, 100, 126, 88, 92, 0, 0x147d2eff },
        { SPRITE_CMD_CIRCLE, 100, 64, 50, 0, 0, 0x2eb85cff },
        { SPRITE_CMD_RECT, 100, 82, 86, 26, 0, 0x1f8f45ff },
        { SPRITE_CMD_CIRCLE, 79, 56, 8, 0, 0, 0x111111ff },
        { SPRITE_CMD_CIRCLE, 121, 56, 8, 0, 0, 0x111111ff },
        { SPRITE_CMD_RECT, 100, 88, 42, 8, 0, 0x0f4d24ff },
};

const SpriteDefinition ORK_SPRITE = {
        .id = "ork",

        .width = 200,
        .height = 200,

        .pivot_x = 100,
        .pivot_y = 100,

        .commands = COMMANDS,
        .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};
