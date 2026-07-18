#include "menu.h"

#include <stdio.h>

#include "../audio/audio.h"
#include "../fonts/font_renderer.h"
#include "../input/input.h"
#include "../localization/localization.h"
#include "../settings/game_settings.h"
#include "ui_controls.h"

#define MENU_FONT_ID "vector_16_basic"
#define MENU_DIM_COLOR 0x00000088
#define MENU_TEXT_COLOR 0xffffffff
#define MENU_ACCENT_COLOR 0xf2cc8fff
#define MENU_EXIT_COLOR 0x8f1d2cff
#define MENU_EXIT_BORDER_COLOR 0xff6b6bff
#define MENU_PANEL_MAX_WIDTH 620
#define MENU_PANEL_PADDING 44
#define MENU_GAP 28
#define MENU_BUTTON_HEIGHT 92
#define MENU_PAUSE_BUTTON_SIZE 120
#define MENU_PAUSE_BUTTON_MARGIN 50
#define MENU_NO_POINTER -1

typedef enum {
    MENU_DRAG_NONE = 0,
    MENU_DRAG_MUSIC = 1,
    MENU_DRAG_SFX = 2
} MenuDragTarget;

typedef struct {
    UiRect panel;
    UiRect first;
    UiRect second;
    UiRect third;
    UiRect music_slider;
    UiRect sfx_slider;
    UiRect language_label;
    UiRect language_en;
    UiRect language_uk;
} MenuLayout;

static const PackedFont* menu_font;
static const GeneratedSprite* pause_sprite;
static int menu_pointer_id = MENU_NO_POINTER;
static MenuDragTarget menu_drag_target = MENU_DRAG_NONE;

static int menu_min(int left, int right)
{
    return left < right ? left : right;
}

static GameLocale menu_locale(const GameState* game)
{
    return game_settings_normalize_locale(game->settings.locale);
}

static const char* menu_text(const GameState* game, LocalizedTextId id)
{
    return localization_text(menu_locale(game), id);
}

static UiRect menu_pause_button_rect(const GameState* game)
{
    UiRect rect = {
        .x = game->screenWidth - MENU_PAUSE_BUTTON_MARGIN - MENU_PAUSE_BUTTON_SIZE,
        .y = MENU_PAUSE_BUTTON_MARGIN,
        .width = MENU_PAUSE_BUTTON_SIZE,
        .height = MENU_PAUSE_BUTTON_SIZE,
    };
    return rect;
}

static MenuLayout menu_layout(const GameState* game, int height)
{
    MenuLayout layout;
    int width = menu_min(MENU_PANEL_MAX_WIDTH, game->screenWidth - 24);

    if (height > game->screenHeight - 24) height = game->screenHeight - 24;
    layout.panel = (UiRect){
        .x = (game->screenWidth - width) / 2,
        .y = (game->screenHeight - height) / 2,
        .width = width,
        .height = height,
    };
    layout.first = (UiRect){
        .x = layout.panel.x + MENU_PANEL_PADDING,
        .y = layout.panel.y + 132,
        .width = layout.panel.width - MENU_PANEL_PADDING * 2,
        .height = MENU_BUTTON_HEIGHT,
    };
    layout.second = layout.first;
    layout.second.y += MENU_BUTTON_HEIGHT + MENU_GAP;
    layout.third = layout.second;
    layout.third.y += MENU_BUTTON_HEIGHT + MENU_GAP;
    layout.music_slider = layout.first;
    layout.sfx_slider = layout.second;
    layout.language_label = layout.third;
    layout.language_label.height = 36;
    layout.language_en = (UiRect){
        .x = layout.panel.x + MENU_PANEL_PADDING,
        .y = layout.language_label.y + 52,
        .width = (layout.panel.width - MENU_PANEL_PADDING * 2 - MENU_GAP) / 2,
        .height = MENU_BUTTON_HEIGHT,
    };
    layout.language_uk = layout.language_en;
    layout.language_uk.x += layout.language_en.width + MENU_GAP;
    return layout;
}

static void menu_centered_label(
        Framebuffer* framebuffer,
        UiRect panel,
        int y,
        int scale,
        const char* text)
{
    int width = font_measure_text(menu_font, scale, text);
    ui_draw_label(
            framebuffer,
            menu_font,
            panel.x + (panel.width - width) / 2,
            y,
            scale,
            MENU_TEXT_COLOR,
            text);
}

static void menu_title(Framebuffer* framebuffer, UiRect panel, const char* text)
{
    int scale = 3;
    while (scale > 1 && font_measure_text(menu_font, scale, text) > panel.width - 32) scale -= 1;
    menu_centered_label(framebuffer, panel, panel.y + MENU_PANEL_PADDING, scale, text);
}

static void menu_draw_base(Framebuffer* framebuffer, UiRect panel)
{
    renderer_draw_color_rect(framebuffer, 0, 0, framebuffer->width, framebuffer->height, MENU_DIM_COLOR);
    ui_draw_panel(framebuffer, panel);
}

static void menu_render_main(Framebuffer* framebuffer, const GameState* game)
{
    MenuLayout layout = menu_layout(game, 530);
    menu_draw_base(framebuffer, layout.panel);
    menu_title(framebuffer, layout.panel, "LITTLE ONE");
    ui_draw_button(framebuffer, menu_font, layout.first, menu_text(game, LOCALIZED_TEXT_START));
    ui_draw_button(framebuffer, menu_font, layout.second, menu_text(game, LOCALIZED_TEXT_SETTINGS));
    ui_draw_button_colored(framebuffer, menu_font, layout.third, menu_text(game, LOCALIZED_TEXT_EXIT), MENU_EXIT_COLOR, MENU_EXIT_BORDER_COLOR);
}

static void menu_render_pause(Framebuffer* framebuffer, const GameState* game)
{
    MenuLayout layout = menu_layout(game, 530);
    menu_draw_base(framebuffer, layout.panel);
    menu_title(framebuffer, layout.panel, menu_text(game, LOCALIZED_TEXT_PAUSED));
    ui_draw_button(framebuffer, menu_font, layout.first, menu_text(game, LOCALIZED_TEXT_RESUME));
    ui_draw_button(framebuffer, menu_font, layout.second, menu_text(game, LOCALIZED_TEXT_SETTINGS));
    ui_draw_button_colored(framebuffer, menu_font, layout.third, menu_text(game, LOCALIZED_TEXT_EXIT), MENU_EXIT_COLOR, MENU_EXIT_BORDER_COLOR);
}

static void menu_render_settings(Framebuffer* framebuffer, const GameState* game)
{
    MenuLayout layout = menu_layout(game, 740);
    UiRect back = layout.panel;
    back.x += MENU_PANEL_PADDING;
    back.width -= MENU_PANEL_PADDING * 2;
    back.height = MENU_BUTTON_HEIGHT;
    back.y = layout.panel.y + layout.panel.height - MENU_PANEL_PADDING - MENU_BUTTON_HEIGHT;

    menu_draw_base(framebuffer, layout.panel);
    menu_title(framebuffer, layout.panel, menu_text(game, LOCALIZED_TEXT_SETTINGS));
    ui_draw_slider(framebuffer, menu_font, layout.music_slider, menu_text(game, LOCALIZED_TEXT_MUSIC), game->settings.music_volume);
    ui_draw_slider(framebuffer, menu_font, layout.sfx_slider, menu_text(game, LOCALIZED_TEXT_SFX), game->settings.sfx_volume);
    menu_centered_label(framebuffer, layout.language_label, layout.language_label.y, 2, menu_text(game, LOCALIZED_TEXT_LANGUAGE));
    ui_draw_button_colored(framebuffer, menu_font, layout.language_en, "EN", menu_locale(game) == GAME_LOCALE_ENGLISH ? MENU_ACCENT_COLOR : 0x111827ff, MENU_TEXT_COLOR);
    ui_draw_button_colored(framebuffer, menu_font, layout.language_uk, "UK", menu_locale(game) == GAME_LOCALE_UKRAINIAN ? MENU_ACCENT_COLOR : 0x111827ff, MENU_TEXT_COLOR);
    ui_draw_button(framebuffer, menu_font, back, menu_text(game, LOCALIZED_TEXT_BACK));
}

static void menu_render_game_over(Framebuffer* framebuffer, const GameState* game)
{
    MenuLayout layout = menu_layout(game, 620);
    char score[32];
    char best[32];

    snprintf(score, sizeof(score), "%s %d", menu_text(game, LOCALIZED_TEXT_SCORE), game->score);
    snprintf(best, sizeof(best), "%s %d", menu_text(game, LOCALIZED_TEXT_BEST), game->bestScore);
    layout.first.y += 70;
    layout.second = layout.first;
    layout.second.y += MENU_BUTTON_HEIGHT + MENU_GAP;

    menu_draw_base(framebuffer, layout.panel);
    menu_title(framebuffer, layout.panel, menu_text(game, LOCALIZED_TEXT_GAME_OVER));
    menu_centered_label(framebuffer, layout.panel, layout.panel.y + 130, 2, score);
    menu_centered_label(framebuffer, layout.panel, layout.panel.y + 180, 2, best);
    ui_draw_button(framebuffer, menu_font, layout.first, menu_text(game, LOCALIZED_TEXT_RETRY));
    ui_draw_button(framebuffer, menu_font, layout.second, menu_text(game, LOCALIZED_TEXT_BACK));
}

static void menu_update_slider(GameState* game, MenuDragTarget target, UiRect rect, int x)
{
    int value = ui_slider_value_from_touch(rect, x);
    if (target == MENU_DRAG_MUSIC) {
        game_settings_set_music_volume(&game->settings, value);
        audio_set_music_volume(game->settings.music_volume);
    } else {
        game_settings_set_sfx_volume(&game->settings, value);
        audio_set_sfx_volume(game->settings.sfx_volume);
    }
    game->settingsDirty = 1;
}

void menu_initialize(void)
{
    menu_font = font_registry_find(MENU_FONT_ID);
    pause_sprite = generated_sprite_get_by_id("btn_pause");
}

void menu_pause(GameState* game)
{
    if (game != 0 && game->uiState == GAME_UI_PLAYING && !game->gameOver) game->uiState = GAME_UI_PAUSED;
}

void menu_render(Framebuffer* framebuffer, const GameState* game)
{
    if (framebuffer == 0 || game == 0 || menu_font == 0) return;
    if (game->gameOver) { menu_render_game_over(framebuffer, game); return; }
    if (game->uiState == GAME_UI_MENU) { menu_render_main(framebuffer, game); return; }
    if (game->uiState == GAME_UI_PAUSED) { menu_render_pause(framebuffer, game); return; }
    if (game->uiState == GAME_UI_SETTINGS) { menu_render_settings(framebuffer, game); return; }
    if (game->uiState == GAME_UI_PLAYING && pause_sprite != 0) {
        UiRect rect = menu_pause_button_rect(game);
        renderer_draw_generated_sprite_fit(framebuffer, pause_sprite, rect.x, rect.y, rect.width, rect.height, SPRITE_FIT_STRETCH);
    }
}

int menu_handle_touch(GameState* game, int action_type, int pointer_id, int x, int y)
{
    MenuLayout layout;
    if (game == 0) return 0;
    if (action_type == INPUT_TOUCH_CANCEL || action_type == INPUT_TOUCH_UP) {
        menu_pointer_id = MENU_NO_POINTER;
        menu_drag_target = MENU_DRAG_NONE;
        return game->uiState != GAME_UI_PLAYING || game->gameOver;
    }
    if (game->gameOver) {
        if (action_type != INPUT_TOUCH_DOWN) return 1;
        layout = menu_layout(game, 620);
        layout.first.y += 70;
        layout.second = layout.first;
        layout.second.y += MENU_BUTTON_HEIGHT + MENU_GAP;
        if (ui_rect_contains(&layout.first, x, y)) game_try_restart_after_game_over(game);
        else if (ui_rect_contains(&layout.second, x, y)) game_show_menu(game);
        return 1;
    }
    if (game->uiState == GAME_UI_PLAYING) {
        UiRect pause = menu_pause_button_rect(game);
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&pause, x, y)) { menu_pause(game); return 1; }
        return 0;
    }
    if (game->uiState == GAME_UI_MENU) {
        if (action_type != INPUT_TOUCH_DOWN) return 1;
        layout = menu_layout(game, 530);
        if (ui_rect_contains(&layout.first, x, y)) game_start_run(game);
        else if (ui_rect_contains(&layout.second, x, y)) game->uiState = GAME_UI_SETTINGS;
        else if (ui_rect_contains(&layout.third, x, y)) game->exitRequested = 1;
        return 1;
    }
    if (game->uiState == GAME_UI_PAUSED) {
        if (action_type != INPUT_TOUCH_DOWN) return 1;
        layout = menu_layout(game, 530);
        if (ui_rect_contains(&layout.first, x, y)) game->uiState = GAME_UI_PLAYING;
        else if (ui_rect_contains(&layout.second, x, y)) game->uiState = GAME_UI_SETTINGS;
        else if (ui_rect_contains(&layout.third, x, y)) game_show_menu(game);
        return 1;
    }
    if (game->uiState == GAME_UI_SETTINGS) {
        UiRect back;
        layout = menu_layout(game, 740);
        back = layout.panel;
        back.x += MENU_PANEL_PADDING;
        back.width -= MENU_PANEL_PADDING * 2;
        back.height = MENU_BUTTON_HEIGHT;
        back.y = layout.panel.y + layout.panel.height - MENU_PANEL_PADDING - MENU_BUTTON_HEIGHT;
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&back, x, y)) { game_show_menu(game); return 1; }
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.language_en, x, y)) { game_settings_set_locale(&game->settings, GAME_LOCALE_ENGLISH); game->settingsDirty = 1; return 1; }
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.language_uk, x, y)) { game_settings_set_locale(&game->settings, GAME_LOCALE_UKRAINIAN); game->settingsDirty = 1; return 1; }
        if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.music_slider, x, y)) { menu_pointer_id = pointer_id; menu_drag_target = MENU_DRAG_MUSIC; }
        else if (action_type == INPUT_TOUCH_DOWN && ui_rect_contains(&layout.sfx_slider, x, y)) { menu_pointer_id = pointer_id; menu_drag_target = MENU_DRAG_SFX; }
        if ((action_type == INPUT_TOUCH_DOWN || action_type == INPUT_TOUCH_MOVE) && pointer_id == menu_pointer_id && menu_drag_target != MENU_DRAG_NONE) {
            menu_update_slider(game, menu_drag_target, menu_drag_target == MENU_DRAG_MUSIC ? layout.music_slider : layout.sfx_slider, x);
        }
        return 1;
    }
    return 1;
}
