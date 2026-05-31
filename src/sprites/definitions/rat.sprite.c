#include "../sprite_definition.h"

static const SpriteCommand COMMANDS[] = {
    {SPRITE_CMD_CIRCLE, 262, 278, 206, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 262, 278, 194, 0, 0, 0x66666bff},
    {SPRITE_CMD_CIRCLE, 360, 310, 150, 0, 0, 0x69696eff},
    {SPRITE_CMD_CIRCLE, 176, 292, 126, 0, 0, 0x5c5c61ff},

    {SPRITE_CMD_CIRCLE, 356, 404, 118, 0, 0, 0x9f9fa4ff},
    {SPRITE_CMD_CIRCLE, 386, 420, 102, 0, 0, 0xb5b5baff},

    {SPRITE_CMD_CIRCLE, 104, 116, 70, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 108, 118, 58, 0, 0, 0x6b6b70ff},
    {SPRITE_CMD_CIRCLE, 98, 116, 42, 0, 0, 0xff7f7aff},
    {SPRITE_CMD_CIRCLE, 86, 104, 22, 0, 0, 0xff9a91ff},

    {SPRITE_CMD_CIRCLE, 266, 98, 78, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 268, 100, 66, 0, 0, 0x707075ff},
    {SPRITE_CMD_CIRCLE, 276, 102, 51, 0, 0, 0xff837fff},
    {SPRITE_CMD_CIRCLE, 258, 91, 24, 0, 0, 0xff9e96ff},

    {SPRITE_CMD_CIRCLE, 108, 218, 72, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 108, 218, 62, 0, 0, 0xffffffff},
    {SPRITE_CMD_CIRCLE, 93, 216, 28, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 82, 204, 8, 0, 0, 0xffffffff},

    {SPRITE_CMD_CIRCLE, 190, 218, 72, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 190, 218, 62, 0, 0, 0xffffffff},
    {SPRITE_CMD_CIRCLE, 174, 216, 28, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 164, 204, 8, 0, 0, 0xffffffff},

    {SPRITE_CMD_RECT, 129, 175, 60, 12, -300, 0x050505ff},
    {SPRITE_CMD_RECT, 190, 170, 56, 12, -250, 0x050505ff},

    {SPRITE_CMD_TRIANGLE, 62, 262, 96, 96, -1570, 0x050505ff},
    {SPRITE_CMD_TRIANGLE, 64, 262, 82, 82, -1570, 0x55555aff},
    {SPRITE_CMD_CIRCLE, 31, 286, 32, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 31, 286, 24, 0, 0, 0xff7774ff},
    {SPRITE_CMD_CIRCLE, 20, 280, 10, 0, 0, 0xffaba4ff},

    {SPRITE_CMD_RECT, 94, 330, 20, 52, -120, 0xffffffff},
    {SPRITE_CMD_RECT, 115, 330, 20, 52, 120, 0xffffffff},
    {SPRITE_CMD_RECT, 105, 356, 46, 8, 0, 0x050505ff},

    {SPRITE_CMD_RECT, 96, 276, 84, 7, -120, 0x050505ff},
    {SPRITE_CMD_RECT, 96, 295, 82, 7, 80, 0x050505ff},
    {SPRITE_CMD_RECT, 96, 314, 72, 7, 260, 0x050505ff},

    {SPRITE_CMD_RECT, 150, 279, 86, 7, 160, 0x050505ff},
    {SPRITE_CMD_RECT, 150, 298, 88, 7, 20, 0x050505ff},
    {SPRITE_CMD_RECT, 150, 318, 76, 7, -170, 0x050505ff},

    {SPRITE_CMD_CIRCLE, 354, 142, 16, 0, 0, 0x4e4e53ff},
    {SPRITE_CMD_CIRCLE, 382, 170, 13, 0, 0, 0x4e4e53ff},
    {SPRITE_CMD_CIRCLE, 416, 182, 14, 0, 0, 0x4e4e53ff},

    {SPRITE_CMD_RECT, 418, 320, 112, 42, 180, 0x050505ff},
    {SPRITE_CMD_RECT, 420, 320, 100, 30, 180, 0xff7d78ff},
    {SPRITE_CMD_CIRCLE, 464, 304, 42, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 464, 304, 31, 0, 0, 0xff857dff},
    {SPRITE_CMD_CIRCLE, 494, 274, 46, 0, 0, 0x050505ff},
    {SPRITE_CMD_CIRCLE, 494, 274, 34, 0, 0, 0xff8b82ff},

    {SPRITE_CMD_RECT, 414, 319, 10, 34, 150, 0xd65f63ff},
    {SPRITE_CMD_RECT, 446, 310, 10, 36, 200, 0xd65f63ff},
    {SPRITE_CMD_RECT, 476, 292, 10, 38, 260, 0xd65f63ff},

    {SPRITE_CMD_RECT, 158, 96, 74, 14, 540, 0x050505ff},
    {SPRITE_CMD_RECT, 181, 76, 80, 14, -180, 0x050505ff},

    {SPRITE_CMD_CIRCLE, 259, 502, 164, 0, 0, 0x00000022}};

const SpriteDefinition RAT_SPRITE = {
    .id = "rat",

    .width = 512,
    .height = 512,

    .pivot_x = 256,
    .pivot_y = 256,

    .commands = COMMANDS,
    .command_count = sizeof(COMMANDS) / sizeof(COMMANDS[0]),
};