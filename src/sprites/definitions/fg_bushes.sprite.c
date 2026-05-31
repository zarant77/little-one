#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
    {SPRITE_CMD_CIRCLE, 656, 93, 56, 0, 0, 0x1d5f37ff},
    {SPRITE_CMD_CIRCLE, 42, 90, 56, 0, 0, 0x1d5f37ff},
    {SPRITE_CMD_CIRCLE, 94, 74, 72, 0, 0, 0x267644ff},
    {SPRITE_CMD_CIRCLE, 162, 92, 62, 0, 0, 0x1f693bff},
    {SPRITE_CMD_CIRCLE, 254, 80, 74, 0, 0, 0x2b7d48ff},
    {SPRITE_CMD_CIRCLE, 330, 98, 58, 0, 0, 0x1e6238ff},
    {SPRITE_CMD_CIRCLE, 434, 76, 76, 0, 0, 0x2e854dff},
    {SPRITE_CMD_CIRCLE, 514, 94, 64, 0, 0, 0x236f40ff},
    {SPRITE_CMD_CIRCLE, 598, 80, 72, 0, 0, 0x2a7a47ff},
    {SPRITE_CMD_RECT, 350, 143, 700, 35, 0, 0x1d5f37ff},
};

const SpriteDefinition FG_BUSHES_SPRITE = {
    .id = "fg_bushes",

    .width = 700,
    .height = 160,

    .pivot_x = 350,
    .pivot_y = 80,

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};