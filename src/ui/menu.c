#include "menu.h"

#include <stdio.h>

#include "../audio/audio.h"
#include "../fonts/font_renderer.h"
#include "../game/cat_profile.h"
#include "../input/input.h"
#include "../localization/localization.h"
#include "../settings/game_settings.h"
#include "ui_controls.h"

#define MENU_FONT_ID "vector_16_basic"
#define MENU_DIM_COLOR 0x00000088
#define MENU_TEXT_COLOR 0xffffffff
#define MENU_PAUSE_BUTTON_SIZE 120
#define MENU_PAUSE_BUTTON_MARGIN 50
#define MENU_PANEL_MAX_WIDTH 620
#define MENU_CAT_PANEL_MAX_WIDTH 860
#define MENU_PANEL_PADDING 44
#define MENU_PANEL_GAP 28
#define MENU_TITLE_SCALE 3
#define MENU_BUTTON_HEIGHT 92
#define MENU_SETTINGS_PANEL_HEIGHT 740
#define MENU_PAUSE_PANEL_HEIGHT 530
#define MENU_GAME_OVER_PANEL_HEIGHT 500
#define MENU_CAT_SELECT_PANEL_HEIGHT 700
#define MENU_CAT_UNLOCK_PANEL_HEIGHT 780
#define MENU_CATS_COMPLETE_PANEL_HEIGHT 700
#define MENU_SLIDER_HEIGHT 94
#define MENU_NO_POINTER -1
#define MENU_LOCK_COLOR 0x00000099
#define MENU_EXIT_BUTTON_COLOR 0x8f1d2cff
#define MENU_EXIT_BUTTON_BORDER_COLOR 0xff6b6bff
#define MENU_FLAG_BORDER_COLOR 0xffffffff
#define MENU_FLAG_SELECTED_COLOR 0xf2cc8fff
#define MENU_CAT_PANEL_BG_SLICE_LEFT 48
#define MENU_CAT_PANEL_BG_SLICE_TOP 48
#define MENU_CAT_PANEL_BG_SLICE_RIGHT 48
#define MENU_CAT_PANEL_BG_SLICE_BOTTOM 48

typedef enum {
    MENU_DRAG_NONE = 0,
    MENU_DRAG_MUSIC = 1,
    MENU_DRAG_SFX = 2
} MenuDragTarget;

typedef struct {
    const PackedFont* font;
    const GeneratedSprite* pause_button;
    const GeneratedSprite* cat_panel_bg;
    const GeneratedSprite* arrow_left;
    const GeneratedSprite* arrow_right;
    int initialized;
} MenuResources;

typedef struct {
    UiRect pause_button;
    UiRect panel;
    UiRect resume_button;
    UiRect settings_button;
    UiRect exit_button;
    UiRect back_button;
    UiRect cats_button;
    UiRect prev_button;
    UiRect next_button;
    UiRect start_button;
    UiRect preview;
    UiRect music_slider;
    UiRect sfx_slider;
    UiRect language_label;
    UiRect language_en_button;
    UiRect language_uk_button;
} MenuLayout;

static MenuResources MENU_RESOURCES;
static int menu_pointer_id = MENU_NO_POINTER;
static MenuDragTarget menu_drag_target = MENU_DRAG_NONE;

static int menu_min_int(int a, int b)
{
    return a < b ? a : b;
}

static int menu_ceil_div_int(int value, int divisor)
{
    if (divisor <= 0)
    {
        return 0;
    }

    return (value + divisor - 1) / divisor;
}

static int menu_text_width(const PackedFont* font, int scale, const char* text)
{
    return font_measure_text(font, scale, text);
}

static GameLocale menu_locale(const GameState* game)
{
    if (game == 0)
    {
        return GAME_LOCALE_ENGLISH;
    }

    return game_settings_normalize_locale(game->settings.locale);
}

static const char* menu_text(const GameState* game, LocalizedTextId text_id)
{
    return localization_text(menu_locale(game), text_id);
}

static const char* menu_profile_text(const GameState* game, LocalizedTextId text_id)
{
    return localization_text(menu_locale(game), text_id);
}

static const MenuResources* menu_resources_get(void)
{
    if (!MENU_RESOURCES.initialized)
    {
        menu_initialize();
    }

    if (MENU_RESOURCES.font == 0
            || MENU_RESOURCES.pause_button == 0
            || MENU_RESOURCES.cat_panel_bg == 0
            || MENU_RESOURCES.arrow_left == 0
            || MENU_RESOURCES.arrow_right == 0)
    {
        return 0;
    }

    return &MENU_RESOURCES;
}

static UiRect menu_pause_button_rect(const GameState* game)
{
    UiRect rect;

    rect.width = MENU_PAUSE_BUTTON_SIZE;
    rect.height = MENU_PAUSE_BUTTON_SIZE;
    rect.x = game->screenWidth - MENU_PAUSE_BUTTON_MARGIN - rect.width;
    rect.y = MENU_PAUSE_BUTTON_MARGIN;

    return rect;
}

static MenuLayout menu_layout_get_ex(const GameState* game, int panel_height, int panel_max_width)
{
    MenuLayout layout;
    int panel_width = menu_min_int(panel_max_width, game->screenWidth - MENU_PANEL_PADDING * 2);
    int max_panel_height = game->screenHeight - 24;

    if (panel_width < 260)
    {
        panel_width = game->screenWidth - 24;
    }
    if (max_panel_height > 0 && panel_height > max_panel_height)
    {
        panel_height = max_panel_height;
    }

    layout.pause_button = menu_pause_button_rect(game);
    layout.panel.width = panel_width;
    layout.panel.height = panel_height;
    layout.panel.x = (game->screenWidth - layout.panel.width) / 2;
    layout.panel.y = (game->screenHeight - layout.panel.height) / 2;
    if (layout.panel.y + layout.panel.height > game->screenHeight - 12)
    {
        layout.panel.y = game->screenHeight - 12 - layout.panel.height;
    }
    if (layout.panel.y < 12)
    {
        layout.panel.y = 12;
    }

    layout.resume_button.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.resume_button.y = layout.panel.y + 132;
    layout.resume_button.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    layout.resume_button.height = MENU_BUTTON_HEIGHT;

    layout.settings_button = layout.resume_button;
    layout.settings_button.y += MENU_BUTTON_HEIGHT + MENU_PANEL_GAP;

    layout.exit_button = layout.settings_button;
    layout.exit_button.y += MENU_BUTTON_HEIGHT + MENU_PANEL_GAP;

    layout.back_button = layout.resume_button;
    layout.back_button.y = layout.panel.y + layout.panel.height - MENU_PANEL_PADDING - MENU_BUTTON_HEIGHT;

    layout.cats_button = layout.back_button;
    layout.cats_button.y -= MENU_BUTTON_HEIGHT + MENU_PANEL_GAP;

    layout.prev_button.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.prev_button.y = layout.panel.y + 300;
    layout.prev_button.width = 110;
    layout.prev_button.height = MENU_BUTTON_HEIGHT;

    layout.next_button = layout.prev_button;
    layout.next_button.x = layout.panel.x + layout.panel.width - MENU_PANEL_PADDING - layout.next_button.width;

    layout.preview.width = layout.panel.width - MENU_PANEL_PADDING * 2 - 260;
    if (layout.preview.width < 180)
    {
        layout.preview.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    }
    layout.preview.height = 180;
    layout.preview.x = layout.panel.x + (layout.panel.width - layout.preview.width) / 2;
    layout.preview.y = layout.panel.y + 150;

    layout.start_button.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.start_button.y = layout.panel.y + layout.panel.height - MENU_PANEL_PADDING - MENU_BUTTON_HEIGHT;
    layout.start_button.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    layout.start_button.height = MENU_BUTTON_HEIGHT;

    layout.music_slider.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.music_slider.y = layout.panel.y + 142;
    layout.music_slider.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    layout.music_slider.height = MENU_SLIDER_HEIGHT;

    layout.sfx_slider = layout.music_slider;
    layout.sfx_slider.y += MENU_SLIDER_HEIGHT + MENU_PANEL_GAP;

    layout.language_label.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.language_label.y = layout.sfx_slider.y + MENU_SLIDER_HEIGHT + 18;
    layout.language_label.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    layout.language_label.height = 36;

    layout.language_en_button.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.language_en_button.y = layout.language_label.y + layout.language_label.height + 18;
    layout.language_en_button.width = (layout.panel.width - MENU_PANEL_PADDING * 2 - MENU_PANEL_GAP) / 2;
    layout.language_en_button.height = MENU_BUTTON_HEIGHT;

    layout.language_uk_button = layout.language_en_button;
    layout.language_uk_button.x = layout.language_en_button.x + layout.language_en_button.width + MENU_PANEL_GAP;

    return layout;
}

static MenuLayout menu_layout_get(const GameState* game, int panel_height)
{
    return menu_layout_get_ex(game, panel_height, MENU_PANEL_MAX_WIDTH);
}

static MenuLayout menu_cat_layout_get(const GameState* game)
{
    return menu_layout_get_ex(game, MENU_CAT_SELECT_PANEL_HEIGHT, MENU_CAT_PANEL_MAX_WIDTH);
}

static MenuLayout menu_cat_unlock_layout_get(const GameState* game)
{
    MenuLayout layout = menu_layout_get_ex(game, MENU_CAT_UNLOCK_PANEL_HEIGHT, MENU_CAT_PANEL_MAX_WIDTH);

    layout.preview.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    if (layout.preview.width > 520)
    {
        layout.preview.width = 520;
    }
    layout.preview.height = 300;
    layout.preview.x = layout.panel.x + (layout.panel.width - layout.preview.width) / 2;
    layout.preview.y = layout.panel.y + 132;

    return layout;
}

static MenuLayout menu_cat_select_layout_get(const GameState* game)
{
    MenuLayout layout = menu_cat_layout_get(game);
    int button_width = (layout.panel.width - MENU_PANEL_PADDING * 2 - MENU_PANEL_GAP) / 2;

    layout.start_button.width = button_width;
    layout.exit_button = layout.start_button;
    layout.exit_button.x = layout.start_button.x + button_width + MENU_PANEL_GAP;

    return layout;
}

static MenuLayout menu_cats_complete_layout_get(const GameState* game)
{
    return menu_layout_get_ex(game, MENU_CATS_COMPLETE_PANEL_HEIGHT, MENU_CAT_PANEL_MAX_WIDTH);
}

static void menu_draw_title(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect panel,
        const char* title
)
{
    int scale = MENU_TITLE_SCALE;
    int max_width = panel.width - MENU_PANEL_PADDING;
    int text_width;
    int x;
    int y = panel.y + MENU_PANEL_PADDING;

    while (scale > 1 && menu_text_width(font, scale, title) > max_width)
    {
        scale -= 1;
    }

    text_width = menu_text_width(font, scale, title);
    x = panel.x + (panel.width - text_width) / 2;
    ui_draw_label(framebuffer, font, x, y, scale, MENU_TEXT_COLOR, title);
}

static void menu_draw_centered_label(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect panel,
        int y,
        int scale,
        const char* label
)
{
    int max_width = panel.width - 16;
    int text_width;
    int x;

    while (scale > 1 && menu_text_width(font, scale, label) > max_width)
    {
        scale -= 1;
    }

    text_width = menu_text_width(font, scale, label);
    x = panel.x + (panel.width - text_width) / 2;
    ui_draw_label(framebuffer, font, x, y, scale, MENU_TEXT_COLOR, label);
}

static void menu_draw_rect_outline(Framebuffer* framebuffer, UiRect rect, int thickness, uint32_t color)
{
    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, thickness, color);
    renderer_draw_color_rect(framebuffer, rect.x, rect.y + rect.height - thickness, rect.width, thickness, color);
    renderer_draw_color_rect(framebuffer, rect.x, rect.y, thickness, rect.height, color);
    renderer_draw_color_rect(framebuffer, rect.x + rect.width - thickness, rect.y, thickness, rect.height, color);
}

static void menu_draw_ukrainian_flag(Framebuffer* framebuffer, UiRect rect)
{
    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, rect.height / 2, 0x0057b8ff);
    renderer_draw_color_rect(
            framebuffer,
            rect.x,
            rect.y + rect.height / 2,
            rect.width,
            rect.height - rect.height / 2,
            0xffd700ff
    );
}

static void menu_draw_british_flag(Framebuffer* framebuffer, UiRect rect)
{
    int cross_w = rect.width / 5;
    int cross_h = rect.height / 5;
    int diagonal = rect.height / 6;

    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, rect.height, 0x012169ff);

    renderer_draw_color_rect(framebuffer, rect.x, rect.y, diagonal, diagonal, 0xffffffff);
    renderer_draw_color_rect(framebuffer, rect.x + rect.width - diagonal, rect.y, diagonal, diagonal, 0xffffffff);
    renderer_draw_color_rect(framebuffer, rect.x, rect.y + rect.height - diagonal, diagonal, diagonal, 0xffffffff);
    renderer_draw_color_rect(
            framebuffer,
            rect.x + rect.width - diagonal,
            rect.y + rect.height - diagonal,
            diagonal,
            diagonal,
            0xffffffff
    );

    renderer_draw_color_rect(
            framebuffer,
            rect.x + (rect.width - cross_w) / 2,
            rect.y,
            cross_w,
            rect.height,
            0xffffffff
    );
    renderer_draw_color_rect(
            framebuffer,
            rect.x,
            rect.y + (rect.height - cross_h) / 2,
            rect.width,
            cross_h,
            0xffffffff
    );
    renderer_draw_color_rect(
            framebuffer,
            rect.x + (rect.width - cross_w / 2) / 2,
            rect.y,
            cross_w / 2,
            rect.height,
            0xc8102eff
    );
    renderer_draw_color_rect(
            framebuffer,
            rect.x,
            rect.y + (rect.height - cross_h / 2) / 2,
            rect.width,
            cross_h / 2,
            0xc8102eff
    );
}

static void menu_draw_language_flag(
        Framebuffer* framebuffer,
        UiRect rect,
        GameLocale locale,
        int selected
)
{
    UiRect flag = rect;
    int inset = 12;

    renderer_draw_color_rect(framebuffer, rect.x, rect.y, rect.width, rect.height, 0x111827ff);
    menu_draw_rect_outline(
            framebuffer,
            rect,
            selected ? 5 : 3,
            selected ? MENU_FLAG_SELECTED_COLOR : MENU_FLAG_BORDER_COLOR
    );

    flag.x += inset;
    flag.y += inset;
    flag.width -= inset * 2;
    flag.height -= inset * 2;

    if (locale == GAME_LOCALE_UKRAINIAN)
    {
        menu_draw_ukrainian_flag(framebuffer, flag);
    }
    else
    {
        menu_draw_british_flag(framebuffer, flag);
    }

    menu_draw_rect_outline(framebuffer, flag, 2, 0x00000088);
}

static void menu_draw_cat_panel_background(
        Framebuffer* framebuffer,
        const MenuResources* resources,
        UiRect panel
)
{
    UiNineSlice slice = {
        .left = MENU_CAT_PANEL_BG_SLICE_LEFT,
        .top = MENU_CAT_PANEL_BG_SLICE_TOP,
        .right = MENU_CAT_PANEL_BG_SLICE_RIGHT,
        .bottom = MENU_CAT_PANEL_BG_SLICE_BOTTOM,
    };

    ui_draw_nine_slice_panel(framebuffer, resources->cat_panel_bg, panel, slice);
}

static void menu_draw_arrow_button(
        Framebuffer* framebuffer,
        const GeneratedSprite* arrow,
        UiRect rect
)
{
    if (arrow == 0)
    {
        return;
    }

    renderer_draw_generated_sprite_fit(
            framebuffer,
            arrow,
            rect.x,
            rect.y,
            rect.width,
            rect.height,
            SPRITE_FIT_STRETCH
    );
}

static void menu_render_game_over(
        Framebuffer* framebuffer,
        const PackedFont* font,
        const GameState* game
)
{
    MenuLayout layout = menu_layout_get(game, MENU_GAME_OVER_PANEL_HEIGHT);
    char score_text[32];
    char time_text[32];

    snprintf(score_text, sizeof(score_text), "%s %d", menu_text(game, LOCALIZED_TEXT_SCORE), game->score);
    snprintf(time_text, sizeof(time_text), "%s %dS", menu_text(game, LOCALIZED_TEXT_TIME), game->runTimeMs / 1000);

    renderer_draw_color_rect(framebuffer, 0, 0, framebuffer->width, framebuffer->height, MENU_DIM_COLOR);
    ui_draw_panel(framebuffer, layout.panel);
    menu_draw_title(framebuffer, font, layout.panel, menu_text(game, LOCALIZED_TEXT_GAME_OVER));
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 170, 2, score_text);
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 240, 2, time_text);
    ui_draw_button(framebuffer, font, layout.cats_button, menu_text(game, LOCALIZED_TEXT_CHANGE_CAT));
    ui_draw_button(framebuffer, font, layout.back_button, menu_text(game, LOCALIZED_TEXT_RETRY));
}

static void menu_cat_requirement_text(
        const CatProfile* profile,
        const ProgressionState* progress,
        GameLocale locale,
        char* text,
        int text_size
)
{
    int written = 0;

    if (text == 0 || text_size <= 0)
    {
        return;
    }

    if (profile == 0)
    {
        text[0] = 0;
        return;
    }

    text[0] = 0;
    if (profile->required_best_score > 0)
    {
        written += snprintf(
                text + written,
                (size_t)(text_size - written),
                "%s %d/%d",
                localization_text(locale, LOCALIZED_TEXT_BEST),
                progress != 0 ? progress->best_score : 0,
                profile->required_best_score
        );
    }

    if (profile->required_total_score > 0 && written < text_size)
    {
        written += snprintf(
                text + written,
                (size_t)(text_size - written),
                "%s%s %d/%d",
                written > 0 ? " " : "",
                localization_text(locale, LOCALIZED_TEXT_TOTAL),
                progress != 0 ? progress->total_score : 0,
                profile->required_total_score
        );
    }

    if (profile->required_runs > 0 && written < text_size)
    {
        snprintf(
                text + written,
                (size_t)(text_size - written),
                "%s%s %d/%d",
                written > 0 ? " " : "",
                localization_text(locale, LOCALIZED_TEXT_RUNS),
                progress != 0 ? progress->total_runs : 0,
                profile->required_runs
        );
    }
}

static void menu_render_cat_unlocked(
        Framebuffer* framebuffer,
        const PackedFont* font,
        const MenuResources* resources,
        const GameState* game
)
{
    MenuLayout layout = menu_cat_unlock_layout_get(game);
    const CatProfile* profile = cat_profile_get(game->unlockedCatIndex);
    const GeneratedSprite* sprite = generated_sprite_get_by_id(profile->sprite_id);

    renderer_draw_color_rect(framebuffer, 0, 0, framebuffer->width, framebuffer->height, MENU_DIM_COLOR);
    menu_draw_cat_panel_background(framebuffer, resources, layout.panel);
    menu_draw_title(framebuffer, font, layout.panel, menu_profile_text(game, profile->name_text_id));

    if (sprite != 0)
    {
        int animation_ms = game->catUnlockPresentationMs;
        int scale_percent;
        int draw_width;
        int draw_height;
        int draw_x;
        int draw_y;

        if (animation_ms < 0)
        {
            animation_ms = 0;
        }
        if (animation_ms < 360)
        {
            scale_percent = 72 + animation_ms * 34 / 360;
        }
        else if (animation_ms < 520)
        {
            scale_percent = 106 - (animation_ms - 360) * 6 / 160;
        }
        else
        {
            scale_percent = 100;
        }

        draw_width = layout.preview.width * scale_percent / 100;
        draw_height = layout.preview.height * scale_percent / 100;
        draw_x = layout.preview.x + (layout.preview.width - draw_width) / 2;
        draw_y = layout.preview.y + (layout.preview.height - draw_height) / 2;
        if (animation_ms < 360)
        {
            draw_y += 28 - animation_ms * 28 / 360;
        }

        renderer_draw_generated_sprite_fit(
                framebuffer,
                sprite,
                draw_x,
                draw_y,
                draw_width,
                draw_height,
                SPRITE_FIT_CONTAIN
        );
    }
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 470, 2, menu_profile_text(game, profile->intro_text_id));
    ui_draw_button(framebuffer, font, layout.start_button, menu_text(game, LOCALIZED_TEXT_CONTINUE));
}

static void menu_render_cats_complete(
        Framebuffer* framebuffer,
        const PackedFont* font,
        const MenuResources* resources,
        const GameState* game
)
{
    MenuLayout layout = menu_cats_complete_layout_get(game);
    int count = cat_profile_count();
    int grid_x = layout.panel.x + MENU_PANEL_PADDING;
    int grid_y = layout.panel.y + 150;
    int grid_width = layout.panel.width - MENU_PANEL_PADDING * 2;
    int grid_height = layout.start_button.y - grid_y - MENU_PANEL_GAP;
    int columns = grid_width >= 760 ? 7 : (grid_width >= 420 ? 4 : 3);
    int rows = menu_ceil_div_int(count, columns);
    int gap = 12;
    int cell_width;
    int cell_height;
    int sprite_width;
    int sprite_height;

    if (rows < 1)
    {
        rows = 1;
    }

    cell_width = (grid_width - gap * (columns - 1)) / columns;
    cell_height = (grid_height - gap * (rows - 1)) / rows;
    sprite_width = cell_width;
    sprite_height = (sprite_width * 16) / 25;
    if (sprite_height > cell_height)
    {
        sprite_height = cell_height;
        sprite_width = (sprite_height * 25) / 16;
    }

    renderer_draw_color_rect(framebuffer, 0, 0, framebuffer->width, framebuffer->height, MENU_DIM_COLOR);
    menu_draw_cat_panel_background(framebuffer, resources, layout.panel);
    menu_draw_title(framebuffer, font, layout.panel, menu_text(game, LOCALIZED_TEXT_CONGRATULATIONS));
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 104, 2, menu_text(game, LOCALIZED_TEXT_ALL_CATS_READY));

    for (int index = 0; index < count; ++index)
    {
        const CatProfile* profile = cat_profile_get(index);
        const GeneratedSprite* sprite = generated_sprite_get_by_id(profile->sprite_id);
        int column = index % columns;
        int row = index / columns;
        int cell_x = grid_x + column * (cell_width + gap);
        int cell_y = grid_y + row * (cell_height + gap);
        int draw_width = sprite_width;
        int draw_height = sprite_height;
        int draw_x;
        int draw_y;

        if (sprite == 0)
        {
            continue;
        }

        if (index == game->unlockedCatIndex)
        {
            int animation_ms = game->catUnlockPresentationMs;
            int scale_percent;

            if (animation_ms < 0)
            {
                animation_ms = 0;
            }
            if (animation_ms < 360)
            {
                scale_percent = 72 + animation_ms * 34 / 360;
            }
            else if (animation_ms < 520)
            {
                scale_percent = 106 - (animation_ms - 360) * 6 / 160;
            }
            else
            {
                scale_percent = 100;
            }

            draw_width = sprite_width * scale_percent / 100;
            draw_height = sprite_height * scale_percent / 100;
        }

        draw_x = cell_x + (cell_width - draw_width) / 2;
        draw_y = cell_y + (cell_height - draw_height) / 2;
        if (index == game->unlockedCatIndex && game->catUnlockPresentationMs < 360)
        {
            draw_y += 18 - game->catUnlockPresentationMs * 18 / 360;
        }

        renderer_draw_generated_sprite_fit(
                framebuffer,
                sprite,
                draw_x,
                draw_y,
                draw_width,
                draw_height,
                SPRITE_FIT_CONTAIN
        );
    }

    ui_draw_button(framebuffer, font, layout.start_button, menu_text(game, LOCALIZED_TEXT_CONTINUE));
}

static void menu_render_cat_select(
        Framebuffer* framebuffer,
        const PackedFont* font,
        const MenuResources* resources,
        const GameState* game
)
{
    MenuLayout layout = menu_cat_select_layout_get(game);
    const CatProfile* profile = cat_profile_get(game->progress.selected_cat_index);
    int unlocked = cat_profile_is_unlocked(profile, &game->progress);
    const GeneratedSprite* sprite = generated_sprite_get_by_id(profile->sprite_id);
    char count_text[24];
    char requirement_text[128];

    renderer_draw_color_rect(framebuffer, 0, 0, framebuffer->width, framebuffer->height, MENU_DIM_COLOR);
    menu_draw_cat_panel_background(framebuffer, resources, layout.panel);
    menu_draw_title(framebuffer, font, layout.panel, menu_text(game, LOCALIZED_TEXT_CHOOSE_CAT));

    snprintf(
            count_text,
            sizeof(count_text),
            "%d/%d",
            game->progress.selected_cat_index + 1,
            cat_profile_count()
    );

    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 104, 2, count_text);

    if (sprite != 0)
    {
        renderer_draw_generated_sprite_fit(
                framebuffer,
                sprite,
                layout.preview.x,
                layout.preview.y,
                layout.preview.width,
                layout.preview.height,
                SPRITE_FIT_CONTAIN
        );
    }
    if (!unlocked)
    {
        renderer_draw_color_rect(
                framebuffer,
                layout.preview.x,
                layout.preview.y,
                layout.preview.width,
                layout.preview.height,
                MENU_LOCK_COLOR
        );
        menu_draw_centered_label(
                framebuffer,
                font,
                layout.preview,
                layout.preview.y + layout.preview.height / 2 - 18,
                3,
                menu_text(game, LOCALIZED_TEXT_LOCKED)
        );
    }

    menu_draw_arrow_button(framebuffer, resources->arrow_left, layout.prev_button);
    menu_draw_arrow_button(framebuffer, resources->arrow_right, layout.next_button);
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 390, 3, menu_profile_text(game, profile->name_text_id));

    if (unlocked)
    {
        ui_draw_button(framebuffer, font, layout.start_button, menu_text(game, LOCALIZED_TEXT_START));
    }
    else
    {
        menu_cat_requirement_text(profile, &game->progress, menu_locale(game), requirement_text, sizeof(requirement_text));
        menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 462, 2, requirement_text);
    }

    ui_draw_button_colored(
            framebuffer,
            font,
            layout.exit_button,
            menu_text(game, LOCALIZED_TEXT_EXIT),
            MENU_EXIT_BUTTON_COLOR,
            MENU_EXIT_BUTTON_BORDER_COLOR
    );
}

static void menu_draw_pause_button(
        Framebuffer* framebuffer,
        const GeneratedSprite* sprite,
        UiRect rect
)
{
    renderer_draw_generated_sprite_fit(
            framebuffer,
            sprite,
            rect.x,
            rect.y,
            rect.width,
            rect.height,
            SPRITE_FIT_STRETCH
    );
}

static void menu_set_state(GameState* game, GameUiState state)
{
    if (game == 0)
    {
        return;
    }

    game->uiState = state;
    menu_pointer_id = MENU_NO_POINTER;
    menu_drag_target = MENU_DRAG_NONE;
}

static void menu_update_slider(GameState* game, MenuDragTarget target, UiRect rect, int x)
{
    int value = ui_slider_value_from_touch(rect, x);

    if (target == MENU_DRAG_MUSIC)
    {
        game_settings_set_music_volume(&game->settings, value);
        audio_set_music_volume(game->settings.music_volume);
        return;
    }

    if (target == MENU_DRAG_SFX)
    {
        game_settings_set_sfx_volume(&game->settings, value);
        audio_set_sfx_volume(game->settings.sfx_volume);
    }
}

void menu_initialize(void)
{
    MENU_RESOURCES.font = font_registry_find(MENU_FONT_ID);
    MENU_RESOURCES.pause_button = generated_sprite_get_by_id("btn_pause");
    MENU_RESOURCES.cat_panel_bg = generated_sprite_get_by_id("cat_select_panel_bg");
    MENU_RESOURCES.arrow_left = generated_sprite_get_by_id("arrow_left");
    MENU_RESOURCES.arrow_right = generated_sprite_get_by_id("arrow_right");
    MENU_RESOURCES.initialized = 1;
}

void menu_pause(GameState* game)
{
    menu_set_state(game, GAME_UI_PAUSED);
}

void menu_render(Framebuffer* framebuffer, const GameState* game)
{
    const MenuResources* resources;
    MenuLayout layout;

    if (framebuffer == 0 || game == 0)
    {
        return;
    }

    resources = menu_resources_get();
    if (resources == 0)
    {
        return;
    }

    if (game->uiState == GAME_UI_CATS_COMPLETE)
    {
        menu_render_cats_complete(framebuffer, resources->font, resources, game);
        return;
    }

    if (game->uiState == GAME_UI_CAT_UNLOCKED)
    {
        menu_render_cat_unlocked(framebuffer, resources->font, resources, game);
        return;
    }

    if (game->gameOver)
    {
        menu_render_game_over(framebuffer, resources->font, game);
        return;
    }

    if (game->uiState == GAME_UI_CAT_SELECT)
    {
        menu_render_cat_select(framebuffer, resources->font, resources, game);
        return;
    }

    if (game->uiState == GAME_UI_PLAYING)
    {
        menu_draw_pause_button(framebuffer, resources->pause_button, menu_pause_button_rect(game));
        return;
    }

    renderer_draw_color_rect(framebuffer, 0, 0, framebuffer->width, framebuffer->height, MENU_DIM_COLOR);

    if (game->uiState == GAME_UI_PAUSED)
    {
        layout = menu_layout_get(game, MENU_PAUSE_PANEL_HEIGHT);
        ui_draw_panel(framebuffer, layout.panel);
        menu_draw_title(framebuffer, resources->font, layout.panel, menu_text(game, LOCALIZED_TEXT_PAUSED));
        ui_draw_button(framebuffer, resources->font, layout.resume_button, menu_text(game, LOCALIZED_TEXT_RESUME));
        ui_draw_button(framebuffer, resources->font, layout.settings_button, menu_text(game, LOCALIZED_TEXT_SETTINGS));
        ui_draw_button_colored(
                framebuffer,
                resources->font,
                layout.exit_button,
                menu_text(game, LOCALIZED_TEXT_EXIT),
                MENU_EXIT_BUTTON_COLOR,
                MENU_EXIT_BUTTON_BORDER_COLOR
        );
        return;
    }

    if (game->uiState == GAME_UI_SETTINGS)
    {
        layout = menu_layout_get(game, MENU_SETTINGS_PANEL_HEIGHT);

        ui_draw_panel(framebuffer, layout.panel);
        menu_draw_title(framebuffer, resources->font, layout.panel, menu_text(game, LOCALIZED_TEXT_SETTINGS));
        ui_draw_slider(
                framebuffer,
                resources->font,
                layout.music_slider,
                menu_text(game, LOCALIZED_TEXT_MUSIC),
                game->settings.music_volume
        );
        ui_draw_slider(
                framebuffer,
                resources->font,
                layout.sfx_slider,
                menu_text(game, LOCALIZED_TEXT_SFX),
                game->settings.sfx_volume
        );
        menu_draw_centered_label(
                framebuffer,
                resources->font,
                layout.language_label,
                layout.language_label.y,
                2,
                menu_text(game, LOCALIZED_TEXT_LANGUAGE)
        );
        menu_draw_language_flag(
                framebuffer,
                layout.language_en_button,
                GAME_LOCALE_ENGLISH,
                menu_locale(game) == GAME_LOCALE_ENGLISH
        );
        menu_draw_language_flag(
                framebuffer,
                layout.language_uk_button,
                GAME_LOCALE_UKRAINIAN,
                menu_locale(game) == GAME_LOCALE_UKRAINIAN
        );
        ui_draw_button(framebuffer, resources->font, layout.back_button, menu_text(game, LOCALIZED_TEXT_BACK));
    }
}

int menu_handle_touch(GameState* game, int action_type, int pointer_id, int x, int y)
{
    MenuLayout layout;

    if (game == 0)
    {
        return 0;
    }

    if (game->uiState == GAME_UI_CAT_UNLOCKED || game->uiState == GAME_UI_CATS_COMPLETE)
    {
        if (action_type == INPUT_TOUCH_DOWN)
        {
            layout = game->uiState == GAME_UI_CATS_COMPLETE
                    ? menu_cats_complete_layout_get(game)
                    : menu_cat_unlock_layout_get(game);
            if (ui_rect_contains(&layout.start_button, x, y))
            {
                game_dismiss_cat_unlocked(game);
                return 1;
            }
        }

        return 1;
    }

    if (game->gameOver)
    {
        if (action_type == INPUT_TOUCH_DOWN)
        {
            layout = menu_layout_get(game, MENU_GAME_OVER_PANEL_HEIGHT);
            if (ui_rect_contains(&layout.cats_button, x, y))
            {
                game_show_cat_select(game);
                return 1;
            }
            if (ui_rect_contains(&layout.back_button, x, y))
            {
                game_try_restart_after_game_over(game);
                return 1;
            }
            return 1;
        }

        return 0;
    }

    if (action_type == INPUT_TOUCH_CANCEL)
    {
        menu_pointer_id = MENU_NO_POINTER;
        menu_drag_target = MENU_DRAG_NONE;
        return game->uiState != GAME_UI_PLAYING;
    }

    if (game->uiState == GAME_UI_PLAYING)
    {
        UiRect pause_button = menu_pause_button_rect(game);
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&pause_button, x, y))
        {
            menu_pause(game);
            return 1;
        }

        return 0;
    }

    if (game->uiState == GAME_UI_CAT_SELECT)
    {
        if (action_type == INPUT_TOUCH_DOWN)
        {
            layout = menu_cat_select_layout_get(game);
            if (ui_rect_contains(&layout.prev_button, x, y))
            {
                int count = cat_profile_count();
                game->progress.selected_cat_index =
                        (game->progress.selected_cat_index + count - 1) % count;
                game->progressDirty = 1;
                return 1;
            }
            if (ui_rect_contains(&layout.next_button, x, y))
            {
                int count = cat_profile_count();
                game->progress.selected_cat_index =
                        (game->progress.selected_cat_index + 1) % count;
                game->progressDirty = 1;
                return 1;
            }
            if (ui_rect_contains(&layout.start_button, x, y))
            {
                game_start_selected_cat(game);
                return 1;
            }
            if (ui_rect_contains(&layout.exit_button, x, y))
            {
                game->exitRequested = 1;
                return 1;
            }
        }

        return 1;
    }

    if (action_type == INPUT_TOUCH_UP)
    {
        menu_pointer_id = MENU_NO_POINTER;
        menu_drag_target = MENU_DRAG_NONE;
        return 1;
    }

    if (action_type != INPUT_TOUCH_DOWN && action_type != INPUT_TOUCH_MOVE)
    {
        return 1;
    }

    if (game->uiState == GAME_UI_PAUSED)
    {
        layout = menu_layout_get(game, MENU_PAUSE_PANEL_HEIGHT);
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.resume_button, x, y))
        {
            menu_set_state(game, GAME_UI_PLAYING);
            return 1;
        }
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.settings_button, x, y))
        {
            menu_set_state(game, GAME_UI_SETTINGS);
            return 1;
        }
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.exit_button, x, y))
        {
            game_show_cat_select(game);
            return 1;
        }

        return 1;
    }

    if (game->uiState == GAME_UI_SETTINGS)
    {
        layout = menu_layout_get(game, MENU_SETTINGS_PANEL_HEIGHT);
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.back_button, x, y))
        {
            menu_pause(game);
            return 1;
        }

        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.language_en_button, x, y))
        {
            game_settings_set_locale(&game->settings, GAME_LOCALE_ENGLISH);
            return 1;
        }

        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.language_uk_button, x, y))
        {
            game_settings_set_locale(&game->settings, GAME_LOCALE_UKRAINIAN);
            return 1;
        }

        if (action_type == INPUT_TOUCH_DOWN)
        {
            if (ui_rect_contains(&layout.music_slider, x, y))
            {
                menu_pointer_id = pointer_id;
                menu_drag_target = MENU_DRAG_MUSIC;
                menu_update_slider(game, menu_drag_target, layout.music_slider, x);
                return 1;
            }
            if (ui_rect_contains(&layout.sfx_slider, x, y))
            {
                menu_pointer_id = pointer_id;
                menu_drag_target = MENU_DRAG_SFX;
                menu_update_slider(game, menu_drag_target, layout.sfx_slider, x);
                return 1;
            }
        }

        if (action_type == INPUT_TOUCH_MOVE
                && pointer_id == menu_pointer_id
                && menu_drag_target != MENU_DRAG_NONE)
        {
            UiRect slider = menu_drag_target == MENU_DRAG_MUSIC
                    ? layout.music_slider
                    : layout.sfx_slider;
            menu_update_slider(game, menu_drag_target, slider, x);
        }
    }

    return 1;
}
