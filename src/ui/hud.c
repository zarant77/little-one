#include "hud.h"

#include <stdio.h>

#include "../config/player_config.h"
#include "../fonts/font_renderer.h"
#include "../sprites/generated_sprite.h"
#include "ui_controls.h"

#define HUD_FONT_ID "vector_16_basic"
#define HUD_SCORE_COLOR 0xffffffff
#define HUD_SCORE_SHADOW_COLOR 0x00000099
#define HUD_BG_SLICE_LEFT 180
#define HUD_BG_SLICE_TOP 24
#define HUD_BG_SLICE_RIGHT 32
#define HUD_BG_SLICE_BOTTOM 24

typedef struct
{
    int bg_width;
    int bg_height;

    int cat_size;
    int heart_size;
    int star_size;

    int margin_x;
    int margin_y;

    int padding_x;
    int padding_y;

    int cat_content_gap;
    int heart_gap;
    int row_gap;
    int star_score_gap;

    int score_scale;
} HudLayout;

static const HudLayout HUD_LAYOUT = {
    .bg_width = 440,
    .bg_height = 176,

    .cat_size = 128,
    .heart_size = 56,
    .star_size = 56,

    .margin_x = 30,
    .margin_y = 30,

    .padding_x = 30,
    .padding_y = 20,

    .cat_content_gap = 50,
    .heart_gap = 8,
    .row_gap = 12,
    .star_score_gap = 12,

    .score_scale = 2,
};

typedef struct
{
    const GeneratedSprite *background;
    const GeneratedSprite *heart_full;
    const GeneratedSprite *heart_empty;
    const GeneratedSprite *star;
    const PackedFont *score_font;
    int initialized;
} HudResources;

static HudResources HUD_RESOURCES;

void hud_initialize(void)
{
    HUD_RESOURCES.background = generated_sprite_get_by_id("hud_bg");
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

    if (HUD_RESOURCES.background == 0 || HUD_RESOURCES.heart_full == 0 || HUD_RESOURCES.heart_empty == 0 || HUD_RESOURCES.star == 0 || HUD_RESOURCES.score_font == 0)
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

static int hud_max_int(int a, int b)
{
    return a > b ? a : b;
}

static int hud_min_int(int a, int b)
{
    return a < b ? a : b;
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

static void hud_draw_background(
    Framebuffer *framebuffer,
    const GeneratedSprite *sprite,
    int x,
    int y,
    int width)
{
    UiRect rect;
    UiNineSlice slice;

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = HUD_LAYOUT.bg_height;

    slice.left = HUD_BG_SLICE_LEFT;
    slice.top = HUD_BG_SLICE_TOP;
    slice.right = HUD_BG_SLICE_RIGHT;
    slice.bottom = HUD_BG_SLICE_BOTTOM;

    ui_draw_nine_slice_panel(
        framebuffer,
        sprite,
        rect,
        slice);
}

static void hud_draw_current_cat_sprite(
    Framebuffer *framebuffer,
    const GameState *game,
    int x,
    int y,
    int width,
    int height)
{
    const GeneratedSprite *sprite = generated_sprite_get(game_player_config(game)->visual.sprite_id);

    if (sprite == 0)
    {
        return;
    }

    renderer_draw_generated_sprite_region_scaled(
        framebuffer,
        sprite,
        sprite->width / 2,
        0,
        sprite->width - sprite->width / 2,
        sprite->height,
        x,
        y,
        width,
        height);
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
    int bg_x;
    int bg_y;
    int bg_width;

    int cat_x;
    int cat_y;

    int right_x;

    int score_text_height;
    int bottom_row_height;
    int right_content_height;
    int right_content_y;

    int hearts_y;
    int bottom_y;
    int star_y;
    int score_y;

    int max_hp;
    int current_hp;
    int heart_row_width;
    int score_width;
    int bottom_row_width;
    int right_content_width;
    char score_text[16];

    if (framebuffer == 0 || game == 0)
    {
        return;
    }

    resources = hud_resources_get();
    if (resources == 0)
    {
        return;
    }

    bg_x = HUD_LAYOUT.margin_x;
    bg_y = HUD_LAYOUT.margin_y;

    max_hp = game_player_config(game)->hp;
    if (max_hp < 0)
    {
        max_hp = 0;
    }

    heart_row_width = max_hp > 0
        ? max_hp * HUD_LAYOUT.heart_size + (max_hp - 1) * HUD_LAYOUT.heart_gap
        : 0;
    snprintf(score_text, sizeof(score_text), "%d", game->score < 0 ? 0 : game->score);
    score_width = font_measure_text(resources->score_font, HUD_LAYOUT.score_scale, score_text);
    bottom_row_width = HUD_LAYOUT.star_size + HUD_LAYOUT.star_score_gap + score_width;
    right_content_width = hud_max_int(heart_row_width, bottom_row_width);
    bg_width = HUD_LAYOUT.padding_x * 2
        + HUD_LAYOUT.cat_size
        + HUD_LAYOUT.cat_content_gap
        + right_content_width;
    bg_width = hud_max_int(bg_width, HUD_LAYOUT.bg_width);
    bg_width = hud_min_int(bg_width, framebuffer->width - HUD_LAYOUT.margin_x * 2);

    hud_draw_background(framebuffer, resources->background, bg_x, bg_y, bg_width);

    cat_x = bg_x + HUD_LAYOUT.padding_x;
    cat_y = bg_y + (HUD_LAYOUT.bg_height - HUD_LAYOUT.cat_size) / 2;

    hud_draw_current_cat_sprite(
        framebuffer,
        game,
        cat_x,
        cat_y,
        HUD_LAYOUT.cat_size,
        HUD_LAYOUT.cat_size);

    right_x = cat_x + HUD_LAYOUT.cat_size + HUD_LAYOUT.cat_content_gap;

    score_text_height = resources->score_font->grid_size * HUD_LAYOUT.score_scale;
    bottom_row_height = hud_max_int(HUD_LAYOUT.star_size, score_text_height);

    right_content_height = HUD_LAYOUT.heart_size + HUD_LAYOUT.row_gap + bottom_row_height;

    right_content_y = bg_y + (HUD_LAYOUT.bg_height - right_content_height) / 2;

    hearts_y = right_content_y;
    bottom_y = hearts_y + HUD_LAYOUT.heart_size + HUD_LAYOUT.row_gap;

    star_y = bottom_y + (bottom_row_height - HUD_LAYOUT.star_size) / 2;
    score_y = bottom_y + (bottom_row_height - score_text_height) / 2;

    current_hp = hud_clamp_int(game->playerHp, 0, max_hp);

    hud_draw_hearts(
        framebuffer,
        resources,
        right_x,
        hearts_y,
        current_hp,
        max_hp);

    hud_draw_sprite(
        framebuffer,
        resources->star,
        right_x,
        star_y,
        HUD_LAYOUT.star_size,
        HUD_LAYOUT.star_size);

    hud_draw_score(
        framebuffer,
        resources->score_font,
        right_x + HUD_LAYOUT.star_size + HUD_LAYOUT.star_score_gap,
        score_y,
        game->score);
}
