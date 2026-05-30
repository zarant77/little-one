#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
        { SPRITE_CMD_CIRCLE, 67, 97, 52, 0, 0, 0x9b8f9eff },
        { SPRITE_CMD_CIRCLE, 111, 89, 60, 0, 0, 0x817886ff },
        { SPRITE_CMD_RECT, 80, 67, 72, 58, 0, 0xb2a8b6ff },
        { SPRITE_CMD_RECT, 89, 126, 118, 48, 0, 0x6f6872ff },
        { SPRITE_CMD_RECT, 139, 116, 48, 46, 0, 0x5d5660ff },
};

const SpriteDefinition ROCK_SPRITE = {
        .id = "rock",

        .width = 150,
        .height = 150,

        .pivot_x = 75,
        .pivot_y = 75,

        .commands = COMMANDS,
        .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};
