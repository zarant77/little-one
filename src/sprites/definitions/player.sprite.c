#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
    {SPRITE_CMD_CIRCLE, 49, 23, 22, 0, 353, 0x3f434bff},
    {SPRITE_CMD_CIRCLE, 31, 46, 26, 0, 353, 0x3f434bff},
    {SPRITE_CMD_CIRCLE, 28, 72, 26, 0, 353, 0x3f434bff},
    {SPRITE_CMD_CIRCLE, 40, 90, 22, 0, 353, 0x3f434bff},
    {SPRITE_CMD_RECT, 143, 132, 142, 102, 0, 0x424242ff},
    {SPRITE_CMD_CIRCLE, 76, 132, 51, 0, 0, 0x424242ff},
    {SPRITE_CMD_TRIANGLE, 198, 141, 70, 57, 3141, 0x484c55ff},
    {SPRITE_CMD_TRIANGLE, 150, 50, 48, 70, -785, 0x3f434bff},
    {SPRITE_CMD_TRIANGLE, 154, 55, 26, 39, -785, 0xff858cff},
    {SPRITE_CMD_TRIANGLE, 267, 49, 48, 70, 785, 0x3f434bff},
    {SPRITE_CMD_TRIANGLE, 263, 54, 26, 39, 785, 0xff858cff},
    {SPRITE_CMD_CIRCLE, 209, 111, 72, 0, 0, 0x626773ff},
    {SPRITE_CMD_CIRCLE, 187, 104, 17, 0, 0, 0xf6df43ff},
    {SPRITE_CMD_CIRCLE, 235, 104, 17, 0, 0, 0xf6df43ff},
    {SPRITE_CMD_CIRCLE, 192, 104, 9, 0, 0, 0x11141aff},
    {SPRITE_CMD_CIRCLE, 240, 104, 9, 0, 0, 0x11141aff},
    {SPRITE_CMD_CIRCLE, 196, 98, 4, 0, 0, 0xffffffff},
    {SPRITE_CMD_CIRCLE, 244, 98, 4, 0, 0, 0xffffffff},
    {SPRITE_CMD_TRIANGLE, 214, 126, 20, 17, 3141, 0xff858cff},
    {SPRITE_CMD_RECT, 214, 141, 7, 22, 0, 0x171a20ff},
    {SPRITE_CMD_RECT, 205, 150, 7, 22, 785, 0x171a20ff},
    {SPRITE_CMD_RECT, 224, 150, 7, 22, -785, 0x171a20ff},
    {SPRITE_CMD_RECT, 150, 117, 52, 4, 100, 0x171a20ff},
    {SPRITE_CMD_RECT, 150, 132, 52, 4, -100, 0x171a20ff},
    {SPRITE_CMD_RECT, 155, 148, 52, 4, -420, 0x171a20ff},
    {SPRITE_CMD_RECT, 273, 117, 52, 4, -100, 0x171a20ff},
    {SPRITE_CMD_RECT, 273, 132, 52, 4, 100, 0x171a20ff},
    {SPRITE_CMD_RECT, 268, 148, 52, 4, 420, 0x171a20ff},
};

const SpriteDefinition PLAYER_SPRITE = {
    .id = "player",

    .width = 300,
    .height = 184,

    .pivot_x = 150,
    .pivot_y = 92,

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};