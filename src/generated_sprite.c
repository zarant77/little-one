#include "generated_sprite.h"

static const uint32_t PLAYER_SPRITE_DATA[] = {
        DRAW_TRIANGLE, 32, 28, 34, 48, 0, 0xffd36bff,
        DRAW_TRIANGLE, 96, 28, 34, 48, 0, 0xffd36bff,

        DRAW_CIRCLE, 64, 48, 42, 0, 0, 0xffd36bff,
        DRAW_CIRCLE, 49, 42, 7, 0, 0, 0x111111ff,
        DRAW_CIRCLE, 79, 42, 7, 0, 0, 0x111111ff,

        DRAW_RECT, 64, 96, 58, 56, 0, 0xff8a3dff,
};

const GeneratedSprite PLAYER_SPRITE = {
        .data = PLAYER_SPRITE_DATA,
        .command_count = 6,
        .width = 128,
        .height = 128,
};

static const uint32_t BOAR_SPRITE_DATA[] = {
        DRAW_CIRCLE, 56, 70, 42, 0, 0, 0xc06e00ff,
        DRAW_RECT, 76, 92, 82, 42, 0, 0xc06e00ff,
        DRAW_CIRCLE, 120, 62, 30, 0, 0, 0xd9872eff,
        DRAW_CIRCLE, 131, 54, 7, 0, 0, 0x111111ff,
        DRAW_TRIANGLE, 139, 81, 20, 16, 0, 0xf0e0c0ff,
        DRAW_RECT, 49, 131, 14, 36, 0, 0x4a260cff,
        DRAW_RECT, 95, 131, 14, 36, 0, 0x4a260cff,
};

static const GeneratedSprite BOAR_SPRITE = {
        .data = BOAR_SPRITE_DATA,
        .command_count = sizeof(BOAR_SPRITE_DATA) / sizeof(BOAR_SPRITE_DATA[0])
                / DRAW_COMMAND_SIZE,
        .width = 160,
        .height = 160,
};

static const uint32_t ORK_SPRITE_DATA[] = {
        DRAW_TRIANGLE, 52, 39, 34, 40, 0, 0x79c753ff,
        DRAW_TRIANGLE, 148, 39, 34, 40, 0, 0x79c753ff,
        DRAW_RECT, 100, 126, 88, 92, 0, 0x147d2eff,
        DRAW_CIRCLE, 100, 64, 50, 0, 0, 0x2eb85cff,
        DRAW_RECT, 100, 82, 86, 26, 0, 0x1f8f45ff,
        DRAW_CIRCLE, 79, 56, 8, 0, 0, 0x111111ff,
        DRAW_CIRCLE, 121, 56, 8, 0, 0, 0x111111ff,
        DRAW_RECT, 100, 88, 42, 8, 0, 0x0f4d24ff,
};

static const GeneratedSprite ORK_SPRITE = {
        .data = ORK_SPRITE_DATA,
        .command_count = sizeof(ORK_SPRITE_DATA) / sizeof(ORK_SPRITE_DATA[0])
                / DRAW_COMMAND_SIZE,
        .width = 200,
        .height = 200,
};

static const uint32_t RAT_SPRITE_DATA[] = {
        DRAW_RECT, 55, 38, 76, 26, 0, 0x666666ff,
        DRAW_CIRCLE, 95, 31, 22, 0, 0, 0x777777ff,
        DRAW_CIRCLE, 86, 14, 9, 0, 0, 0x888888ff,
        DRAW_CIRCLE, 108, 15, 8, 0, 0, 0x888888ff,
        DRAW_CIRCLE, 103, 28, 4, 0, 0, 0x111111ff,
        DRAW_RECT, 17, 33, 34, 6, 0, 0x999999ff,
        DRAW_TRIANGLE, 4, 33, 12, 8, 0, 0x999999ff,
};

static const GeneratedSprite RAT_SPRITE = {
        .data = RAT_SPRITE_DATA,
        .command_count = sizeof(RAT_SPRITE_DATA) / sizeof(RAT_SPRITE_DATA[0])
                / DRAW_COMMAND_SIZE,
        .width = 128,
        .height = 64,
};

static const uint32_t ROCK_SPRITE_DATA[] = {
        DRAW_CIRCLE, 67, 97, 52, 0, 0, 0x9b8f9eff,
        DRAW_CIRCLE, 111, 89, 60, 0, 0, 0x817886ff,
        DRAW_TRIANGLE, 80, 55, 72, 78, 0, 0xb2a8b6ff,
        DRAW_RECT, 89, 126, 118, 48, 0, 0x6f6872ff,
        DRAW_TRIANGLE, 139, 113, 48, 46, 0, 0x5d5660ff,
};

static const GeneratedSprite ROCK_SPRITE = {
        .data = ROCK_SPRITE_DATA,
        .command_count = sizeof(ROCK_SPRITE_DATA) / sizeof(ROCK_SPRITE_DATA[0])
                / DRAW_COMMAND_SIZE,
        .width = 150,
        .height = 150,
};

static const uint32_t STUMP_SPRITE_DATA[] = {
        DRAW_RECT, 40, 101, 58, 136, 0, 0x9b4f18ff,
        DRAW_CIRCLE, 40, 34, 35, 0, 0, 0xcd7a2aff,
        DRAW_CIRCLE, 40, 34, 22, 0, 0, 0x8a4514ff,
        DRAW_CIRCLE, 40, 34, 10, 0, 0, 0x5c2e0dff,
        DRAW_RECT, 29, 100, 8, 98, 0, 0x6b340fff,
        DRAW_RECT, 52, 102, 7, 84, 0, 0x6b340fff,
        DRAW_TRIANGLE, 18, 170, 32, 24, 0, 0x6b340fff,
        DRAW_TRIANGLE, 62, 170, 32, 24, 0, 0x6b340fff,
};

static const GeneratedSprite STUMP_SPRITE = {
        .data = STUMP_SPRITE_DATA,
        .command_count = sizeof(STUMP_SPRITE_DATA) / sizeof(STUMP_SPRITE_DATA[0])
                / DRAW_COMMAND_SIZE,
        .width = 80,
        .height = 180,
};

static const GeneratedSprite* GENERATED_SPRITES[SPRITE_COUNT] = {
        &PLAYER_SPRITE,
        &BOAR_SPRITE,
        &ORK_SPRITE,
        &RAT_SPRITE,
        &ROCK_SPRITE,
        &STUMP_SPRITE,
};

const GeneratedSprite* generated_sprite_get(SpriteId sprite_id)
{
    if (sprite_id < 0 || sprite_id >= SPRITE_COUNT)
    {
        return 0;
    }

    return GENERATED_SPRITES[sprite_id];
}
