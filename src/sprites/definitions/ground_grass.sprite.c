#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
    {SPRITE_CMD_RECT, 151, 17, 300, 40, 0, 0x3f7f43ff},
    {SPRITE_CMD_CIRCLE, 20, 14, 24, 0, 0, 0x54a45aff},
    {SPRITE_CMD_CIRCLE, 64, 12, 28, 0, 0, 0x65b866ff},
    {SPRITE_CMD_CIRCLE, 114, 16, 24, 0, 0, 0x4f9c55ff},
    {SPRITE_CMD_CIRCLE, 168, 12, 30, 0, 0, 0x6abb69ff},
    {SPRITE_CMD_CIRCLE, 222, 16, 26, 0, 0, 0x58a95bff},
    {SPRITE_CMD_TRIANGLE, 0, 23, 12, 28, 0, 0x2e8f3dff},
    {SPRITE_CMD_TRIANGLE, 39, 21, 10, 24, 0, 0x7acb6eff},
    {SPRITE_CMD_TRIANGLE, 91, 18, 12, 26, 0, 0x2f9440ff},
    {SPRITE_CMD_TRIANGLE, 138, 18, 10, 22, 0, 0x7bd06fff},
    {SPRITE_CMD_TRIANGLE, 197, 18, 12, 26, 0, 0x349d45ff},
    {SPRITE_CMD_RECT, 206, 72, 36, 6, 0, 0x3f2819ff},
    {SPRITE_CMD_CIRCLE, 276, 12, 30, 0, 0, 0x6abb69ff},
    {SPRITE_CMD_TRIANGLE, 248, 20, 10, 24, 0, 0x75c969ff},
    {SPRITE_CMD_RECT, 150, 57, 300, 58, 0, 0x6b4a2eff},
    {SPRITE_CMD_RECT, 24, 44, 34, 6, 0, 0x7c5738ff},
    {SPRITE_CMD_RECT, 92, 58, 44, 6, 0, 0x553720ff},
    {SPRITE_CMD_RECT, 146, 41, 38, 6, 0, 0x805b3aff},
    {SPRITE_CMD_RECT, 150, 184, 300, 232, 0, 0x4d321fff},
    {SPRITE_CMD_RECT, 215, 55, 44, 6, 0, 0x553720ff},
    {SPRITE_CMD_RECT, 272, 38, 34, 6, 0, 0x7c5738ff},
};

const SpriteDefinition GROUND_GRASS_SPRITE = {
    .id = "ground_grass",

    .width = 300,
    .height = 300,

    .pivot_x = 150,
    .pivot_y = 150,

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};