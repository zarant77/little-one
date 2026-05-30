#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
    // Tail, back to front
    { SPRITE_CMD_CIRCLE,   18, 40, 10,  0, 0, 0x3f434bff },
    { SPRITE_CMD_CIRCLE,   14, 52, 12,  0, 0, 0x3f434bff },
    { SPRITE_CMD_CIRCLE,   17, 64, 12,  0, 0, 0x3f434bff },
    { SPRITE_CMD_CIRCLE,   25, 70, 10,  0, 0, 0x3f434bff },
    { SPRITE_CMD_CIRCLE,   29, 68,  7,  0, 0, 0xf0f0f0ff },

    // Body
    { SPRITE_CMD_RECT,     49, 76, 42, 26, 0, 0x555a64ff },
    { SPRITE_CMD_TRIANGLE, 30, 83, 18, 22, 3141, 0x555a64ff },
    { SPRITE_CMD_TRIANGLE, 67, 66, 32, 26, 3141, 0x484c55ff },

    // Legs
    { SPRITE_CMD_RECT,     34, 92, 10, 24, 0, 0x484c55ff },
    { SPRITE_CMD_RECT,     52, 92, 10, 24, 0, 0x484c55ff },
    { SPRITE_CMD_RECT,     70, 92, 10, 24, 0, 0x484c55ff },
    { SPRITE_CMD_RECT,     86, 92, 10, 24, 0, 0x484c55ff },

    // Paws
    { SPRITE_CMD_RECT,     34,105, 14,  7, 0, 0x8b929cff },
    { SPRITE_CMD_RECT,     52,105, 14,  7, 0, 0x8b929cff },
    { SPRITE_CMD_RECT,     70,105, 14,  7, 0, 0x8b929cff },
    { SPRITE_CMD_RECT,     86,105, 14,  7, 0, 0x8b929cff },

    // Outer ears
    { SPRITE_CMD_TRIANGLE, 54, 29, 22, 32, -785, 0x3f434bff },
    { SPRITE_CMD_TRIANGLE, 88, 29, 22, 32,  785, 0x3f434bff },

    // Inner ears
    { SPRITE_CMD_TRIANGLE, 56, 31, 12, 18, -785, 0xff858cff },
    { SPRITE_CMD_TRIANGLE, 86, 31, 12, 18,  785, 0xff858cff },

    // Head
    { SPRITE_CMD_CIRCLE,   72, 52, 33,  0, 0, 0x626773ff },

    // Chest shadow
    { SPRITE_CMD_TRIANGLE, 73, 75, 28, 28, 3141, 0x484c55ff },

    // Eyes
    { SPRITE_CMD_CIRCLE,   62, 49,  8,  0, 0, 0xf6df43ff },
    { SPRITE_CMD_CIRCLE,   84, 49,  8,  0, 0, 0xf6df43ff },
    { SPRITE_CMD_CIRCLE,   64, 49,  4,  0, 0, 0x11141aff },
    { SPRITE_CMD_CIRCLE,   86, 49,  4,  0, 0, 0x11141aff },
    { SPRITE_CMD_CIRCLE,   66, 46,  2,  0, 0, 0xffffffff },
    { SPRITE_CMD_CIRCLE,   88, 46,  2,  0, 0, 0xffffffff },

    // Nose
    { SPRITE_CMD_TRIANGLE, 74, 59,  9,  8, 3141, 0xff858cff },

    // Mouth, built from tiny rotated rectangles
    { SPRITE_CMD_RECT,     74, 66,  3, 10, 0,    0x171a20ff },
    { SPRITE_CMD_RECT,     70, 70,  3, 10, 785,  0x171a20ff },
    { SPRITE_CMD_RECT,     79, 70,  3, 10, -785, 0x171a20ff },

    // Left whiskers
    { SPRITE_CMD_RECT,     45, 55, 24,  2, 100,  0x171a20ff },
    { SPRITE_CMD_RECT,     45, 62, 24,  2, -100, 0x171a20ff },
    { SPRITE_CMD_RECT,     47, 69, 24,  2, -420, 0x171a20ff },

    // Right whiskers
    { SPRITE_CMD_RECT,    101, 55, 24,  2, -100, 0x171a20ff },
    { SPRITE_CMD_RECT,    101, 62, 24,  2, 100,  0x171a20ff },
    { SPRITE_CMD_RECT,     99, 69, 24,  2, 420,  0x171a20ff }
};

const SpriteDefinition PLAYER_SPRITE = {
    .id = "player",

    .width = 128,
    .height = 128,

    .pivot_x = 64,
    .pivot_y = 64,

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};