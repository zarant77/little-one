#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
    {SPRITE_CMD_TRIANGLE, 1052, 145, 357, 256, 0, 0x7592a7ff},
    {SPRITE_CMD_TRIANGLE, 1052, 148, 281, 203, 0, 0x8da9baff},
    {SPRITE_CMD_TRIANGLE, 144, 154, 357, 256, 0, 0x7592a7ff},
    {SPRITE_CMD_TRIANGLE, 144, 157, 281, 203, 0, 0x8da9baff},
    {SPRITE_CMD_TRIANGLE, 358, 139, 432, 278, 0, 0x66849bff},
    {SPRITE_CMD_TRIANGLE, 358, 146, 326, 222, 0, 0x82a0b5ff},
    {SPRITE_CMD_TRIANGLE, 611, 163, 432, 237, 0, 0x5f7c94ff},
    {SPRITE_CMD_TRIANGLE, 611, 170, 331, 180, 0, 0x7897adff},
    {SPRITE_CMD_TRIANGLE, 849, 150, 357, 259, 0, 0x6e8ba1ff},
    {SPRITE_CMD_TRIANGLE, 849, 157, 281, 203, 0, 0x88a5b7ff},
    {SPRITE_CMD_RECT, 601, 285, 1202, 101, 0, 0x5d7b94ff},
    {SPRITE_CMD_RECT, 601, 313, 1202, 60, 0, 0x4f708aff},
};

const SpriteDefinition BG_MOUNTAINS_SPRITE = {
    .id = "bg_mountains",

    .width = 1200,
    .height = 340,

    .pivot_x = 600,
    .pivot_y = 170,

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};