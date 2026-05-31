#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
    {SPRITE_CMD_RECT, 605, 243, 1211, 155, 0, 0x31563dff},
    {SPRITE_CMD_TRIANGLE, 64, 161, 167, 220, 0, 0x244832ff},
    {SPRITE_CMD_TRIANGLE, 174, 139, 189, 254, 0, 0x2f6242ff},
    {SPRITE_CMD_TRIANGLE, 314, 180, 148, 201, 0, 0x264d35ff},
    {SPRITE_CMD_TRIANGLE, 431, 131, 204, 261, 0, 0x37704aff},
    {SPRITE_CMD_TRIANGLE, 590, 169, 174, 223, 0, 0x28563aff},
    {SPRITE_CMD_TRIANGLE, 727, 143, 197, 250, 0, 0x315f42ff},
    {SPRITE_CMD_TRIANGLE, 882, 188, 155, 197, 0, 0x234731ff},
    {SPRITE_CMD_TRIANGLE, 1007, 146, 204, 246, 0, 0x3a724dff},
    {SPRITE_CMD_TRIANGLE, 1149, 163, 155, 197, 0, 0x234731ff},
    {SPRITE_CMD_RECT, 605, 300, 1211, 76, 0, 0x274832ff},
};

const SpriteDefinition BG_FOREST_SPRITE = {
    .id = "bg_forest",

    .width = 1200,
    .height = 330,

    .pivot_x = 600,
    .pivot_y = 165,

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};