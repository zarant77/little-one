#include "hud.h"

#include <stdio.h>

#include "../config/player_config.h"
#include "../fonts/font_renderer.h"
#include "../sprites/generated_sprite.h"

#define HUD_FONT_ID "vector_16_basic"
#define HUD_SCORE_COLOR 0x111111ff
#define HUD_SCORE_SHADOW_COLOR 0xffffff99

typedef struct
{
    int heart_size;
    int star_size;
    int margin_y;
    int heart_gap;
    int row_gap;
    int star_score_gap;
    int score_scale;
} HudLayout;

static const HudLayout HUD_LAYOUT = {
    .heart_size = 102,
    .star_size = 102,
    .margin_y = 48,
    .heart_gap = 15,
    .row_gap = 21,
    .star_score_gap = 18,
    .score_scale = 5,
};

typedef struct
{
    const GeneratedSprite *heart_full;
    const GeneratedSprite *heart_empty;
    const GeneratedSprite *star;
    const PackedFont *score_font;
    int initialized;
} HudResources;

static HudResources HUD_RESOURCES;

void hud_initialize(void)
{
    HUD_RESOURCES.heart_full = generated_sprite_get_by_id("heart_full");
    HUD_RESOURCES.heart_empty = generated_sprite_get_by_id("heart_empty");
    HUD_RESOURCES.star = generated_sprite_get_by_id("star");
    HUD_RESOURCES.score_font = font_registry_find(HUD_FONT_ID);
    HUD_RESOURCES.initialized = 1;
}

static const HudResources *hud_resources_get(void)
{
    if (!HUD_RESOURCES.initialized)
    {
        hud_initialize();
    }

    if (HUD_RESOURCES.heart_full == 0 || HUD_RESOURCES.heart_empty == 0 || HUD_RESOURCES.star == 0 || HUD_RESOURCES.score_font == 0)
    {
        return 0;
    }

    return &HUD_RESOURCES;
}

static int hud_clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

static void hud_draw_sprite(
    Framebuffer *framebuffer,
    const GeneratedSprite *sprite,
    int x,
    int y,
    int width,
    int height)
{
    renderer_draw_generated_sprite_fit(
        framebuffer,
        sprite,
        x,
        y,
        width,
        height,
        SPRITE_FIT_CONTAIN);
}

static void hud_draw_score(
    Framebuffer *framebuffer,
    const PackedFont *font,
    int x,
    int y,
    int score)
{
    char text[16];

    if (score < 0)
    {
        score = 0;
    }

    snprintf(text, sizeof(text), "%d", score);

    font_draw_text(
        framebuffer,
        font,
        x + 1,
        y + 1,
        HUD_LAYOUT.score_scale,
        HUD_SCORE_SHADOW_COLOR,
        text);

    font_draw_text(
        framebuffer,
        font,
        x,
        y,
        HUD_LAYOUT.score_scale,
        HUD_SCORE_COLOR,
        text);
}

static void hud_draw_hearts(
    Framebuffer *framebuffer,
    const HudResources *resources,
    int x,
    int y,
    int current_hp,
    int max_hp)
{
    for (int hp_slot = 0; hp_slot < max_hp; ++hp_slot)
    {
        const GeneratedSprite *heart = hp_slot < current_hp
                                           ? resources->heart_full
                                           : resources->heart_empty;

        int heart_x = x + hp_slot * (HUD_LAYOUT.heart_size + HUD_LAYOUT.heart_gap);

        hud_draw_sprite(
            framebuffer,
            heart,
            heart_x,
            y,
            HUD_LAYOUT.heart_size,
            HUD_LAYOUT.heart_size);
    }
}

void hud_render(Framebuffer *framebuffer, const GameState *game)
{
    const HudResources *resources;
    int max_hp;
    int current_hp;
    int heart_row_width;
    int heart_row_x;
    int score_row_width;
    int score_row_x;
    int score_y;
    char score_text[16];

    if (framebuffer == 0 || game == 0)
    {
        return;
    }

    if (game->uiState != GAME_UI_PLAYING || game->gameOver)
    {
        return;
    }

    resources = hud_resources_get();
    if (resources == 0)
    {
        return;
    }

    max_hp = game_player_config(game)->hp;
    if (max_hp < 0)
    {
        max_hp = 0;
    }

    current_hp = hud_clamp_int(game->playerHp, 0, max_hp);
    heart_row_width = max_hp > 0
        ? max_hp * HUD_LAYOUT.heart_size + (max_hp - 1) * HUD_LAYOUT.heart_gap
        : 0;
    heart_row_x = (framebuffer->width - heart_row_width) / 2;

    snprintf(score_text, sizeof(score_text), "%d", game->score < 0 ? 0 : game->score);
    score_row_width = HUD_LAYOUT.star_size
        + HUD_LAYOUT.star_score_gap
        + font_measure_text(resources->score_font, HUD_LAYOUT.score_scale, score_text);
    score_row_x = (framebuffer->width - score_row_width) / 2;
    score_y = HUD_LAYOUT.margin_y + HUD_LAYOUT.heart_size + HUD_LAYOUT.row_gap;

    hud_draw_hearts(
        framebuffer,
        resources,
        heart_row_x,
        HUD_LAYOUT.margin_y,
        current_hp,
        max_hp);

    hud_draw_sprite(
        framebuffer,
        resources->star,
        score_row_x,
        score_y,
        HUD_LAYOUT.star_size,
        HUD_LAYOUT.star_size);

    hud_draw_score(
        framebuffer,
        resources->score_font,
        score_row_x + HUD_LAYOUT.star_size + HUD_LAYOUT.star_score_gap,
        score_y
            + (HUD_LAYOUT.star_size - resources->score_font->grid_size * HUD_LAYOUT.score_scale) / 2,
        game->score);
}
