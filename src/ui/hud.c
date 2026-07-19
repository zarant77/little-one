#include "hud.h"

#include <stdio.h>

#include "../fonts/font_renderer.h"
#include "../sprites/generated_sprite.h"

#define HUD_FONT_ID "vector_16_basic"
#define HUD_SCORE_COLOR 0x111111ff
#define HUD_SCORE_SHADOW_COLOR 0xffffff99

typedef struct
{
    int star_size;
    int margin_x;
    int margin_y;
    int star_score_gap;
    int score_scale;
} HudLayout;

static const HudLayout HUD_LAYOUT = {
    .star_size = 102,
    .margin_x = 48,
    .margin_y = 48,
    .star_score_gap = 18,
    .score_scale = 5,
};

typedef struct
{
    const GeneratedSprite *star;
    const PackedFont *score_font;
    int initialized;
} HudResources;

static HudResources HUD_RESOURCES;

void hud_initialize(void)
{
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

    if (HUD_RESOURCES.star == 0 || HUD_RESOURCES.score_font == 0)
    {
        return 0;
    }

    return &HUD_RESOURCES;
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

void hud_render(Framebuffer *framebuffer, const GameState *game)
{
    const HudResources *resources;

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

    hud_draw_sprite(
        framebuffer,
        resources->star,
        HUD_LAYOUT.margin_x,
        HUD_LAYOUT.margin_y,
        HUD_LAYOUT.star_size,
        HUD_LAYOUT.star_size);

    hud_draw_score(
        framebuffer,
        resources->score_font,
        HUD_LAYOUT.margin_x + HUD_LAYOUT.star_size + HUD_LAYOUT.star_score_gap,
        HUD_LAYOUT.margin_y
            + (HUD_LAYOUT.star_size - resources->score_font->grid_size * HUD_LAYOUT.score_scale) / 2,
        game->score);
}
