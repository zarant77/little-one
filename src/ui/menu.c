#include "menu.h"

#include <stdio.h>

#include "../audio/audio.h"
#include "../fonts/font_renderer.h"
#include "../input/input.h"
#include "../settings/game_settings.h"
#include "ui_controls.h"

#define MENU_FONT_ID "vector_16_basic"
#define MENU_DIM_COLOR 0x00000088
#define MENU_TEXT_COLOR 0xffffffff
#define MENU_PAUSE_BUTTON_SIZE 120
#define MENU_PAUSE_BUTTON_MARGIN 50
#define MENU_PANEL_MAX_WIDTH 620
#define MENU_PANEL_PADDING 44
#define MENU_PANEL_GAP 28
#define MENU_TITLE_SCALE 3
#define MENU_BUTTON_HEIGHT 92
#define MENU_SETTINGS_PANEL_HEIGHT 620
#define MENU_PAUSE_PANEL_HEIGHT 410
#define MENU_GAME_OVER_PANEL_HEIGHT 500
#define MENU_SLIDER_HEIGHT 94
#define MENU_NO_POINTER -1

typedef enum {
    MENU_DRAG_NONE = 0,
    MENU_DRAG_MUSIC = 1,
    MENU_DRAG_SFX = 2
} MenuDragTarget;

typedef struct {
    const PackedFont* font;
    const GeneratedSprite* pause_button;
    int initialized;
} MenuResources;

typedef struct {
    UiRect pause_button;
    UiRect panel;
    UiRect resume_button;
    UiRect settings_button;
    UiRect back_button;
    UiRect music_slider;
    UiRect sfx_slider;
} MenuLayout;

static MenuResources MENU_RESOURCES;
static int menu_pointer_id = MENU_NO_POINTER;
static MenuDragTarget menu_drag_target = MENU_DRAG_NONE;

static int menu_min_int(int a, int b)
{
    return a < b ? a : b;
}

static int menu_text_width(const PackedFont* font, int scale, const char* text)
{
    int width = 0;

    if (font == 0 || scale <= 0 || text == 0)
    {
        return 0;
    }

    for (int index = 0; text[index] != 0; ++index)
    {
        width += (int)font->default_advance * scale;
    }

    return width;
}

static const MenuResources* menu_resources_get(void)
{
    if (!MENU_RESOURCES.initialized)
    {
        menu_initialize();
    }

    if (MENU_RESOURCES.font == 0 || MENU_RESOURCES.pause_button == 0)
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

static MenuLayout menu_layout_get(const GameState* game, int panel_height)
{
    MenuLayout layout;
    int panel_width = menu_min_int(MENU_PANEL_MAX_WIDTH, game->screenWidth - MENU_PANEL_PADDING * 2);

    if (panel_width < 260)
    {
        panel_width = game->screenWidth - 24;
    }

    layout.pause_button = menu_pause_button_rect(game);
    layout.panel.width = panel_width;
    layout.panel.height = panel_height;
    layout.panel.x = (game->screenWidth - layout.panel.width) / 2;
    layout.panel.y = (game->screenHeight - layout.panel.height) / 2;
    if (layout.panel.y < MENU_PANEL_PADDING)
    {
        layout.panel.y = MENU_PANEL_PADDING;
    }

    layout.resume_button.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.resume_button.y = layout.panel.y + 132;
    layout.resume_button.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    layout.resume_button.height = MENU_BUTTON_HEIGHT;

    layout.settings_button = layout.resume_button;
    layout.settings_button.y += MENU_BUTTON_HEIGHT + MENU_PANEL_GAP;

    layout.back_button = layout.resume_button;
    layout.back_button.y = layout.panel.y + layout.panel.height - MENU_PANEL_PADDING - MENU_BUTTON_HEIGHT;

    layout.music_slider.x = layout.panel.x + MENU_PANEL_PADDING;
    layout.music_slider.y = layout.panel.y + 142;
    layout.music_slider.width = layout.panel.width - MENU_PANEL_PADDING * 2;
    layout.music_slider.height = MENU_SLIDER_HEIGHT;

    layout.sfx_slider = layout.music_slider;
    layout.sfx_slider.y += MENU_SLIDER_HEIGHT + MENU_PANEL_GAP;

    return layout;
}

static void menu_draw_title(
        Framebuffer* framebuffer,
        const PackedFont* font,
        UiRect panel,
        const char* title
)
{
    int text_width = menu_text_width(font, MENU_TITLE_SCALE, title);
    int x = panel.x + (panel.width - text_width) / 2;
    int y = panel.y + MENU_PANEL_PADDING;

    ui_draw_label(framebuffer, font, x, y, MENU_TITLE_SCALE, MENU_TEXT_COLOR, title);
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
    int text_width = menu_text_width(font, scale, label);
    int x = panel.x + (panel.width - text_width) / 2;

    ui_draw_label(framebuffer, font, x, y, scale, MENU_TEXT_COLOR, label);
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

    snprintf(score_text, sizeof(score_text), "SCORE %d", game->score);
    snprintf(time_text, sizeof(time_text), "TIME %dS", game->runTimeMs / 1000);

    renderer_draw_color_rect(framebuffer, 0, 0, framebuffer->width, framebuffer->height, MENU_DIM_COLOR);
    ui_draw_panel(framebuffer, layout.panel);
    menu_draw_title(framebuffer, font, layout.panel, "GAME OVER");
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 170, 2, score_text);
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 240, 2, time_text);
    menu_draw_centered_label(framebuffer, font, layout.panel, layout.panel.y + 350, 2, "TAP TO RETRY");
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

    if (game->gameOver)
    {
        menu_render_game_over(framebuffer, resources->font, game);
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
        menu_draw_title(framebuffer, resources->font, layout.panel, "PAUSED");
        ui_draw_button(framebuffer, resources->font, layout.resume_button, "RESUME");
        ui_draw_button(framebuffer, resources->font, layout.settings_button, "SETTINGS");
        return;
    }

    if (game->uiState == GAME_UI_SETTINGS)
    {
        layout = menu_layout_get(game, MENU_SETTINGS_PANEL_HEIGHT);
        ui_draw_panel(framebuffer, layout.panel);
        menu_draw_title(framebuffer, resources->font, layout.panel, "SETTINGS");
        ui_draw_slider(
                framebuffer,
                resources->font,
                layout.music_slider,
                "MUSIC",
                game->settings.music_volume
        );
        ui_draw_slider(
                framebuffer,
                resources->font,
                layout.sfx_slider,
                "SFX",
                game->settings.sfx_volume
        );
        ui_draw_button(framebuffer, resources->font, layout.back_button, "BACK");
    }
}

int menu_handle_touch(GameState* game, int action_type, int pointer_id, int x, int y)
{
    MenuLayout layout;

    if (game == 0)
    {
        return 0;
    }

    if (game->gameOver)
    {
        if (action_type == INPUT_TOUCH_DOWN)
        {
            game_try_restart_after_game_over(game);
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
