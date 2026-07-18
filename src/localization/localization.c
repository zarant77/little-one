#include "localization.h"

typedef struct
{
    const char *english;
    const char *ukrainian;
} LocalizedText;

static const LocalizedText LOCALIZED_TEXTS[] = {
    [LOCALIZED_TEXT_BACK] = {"BACK", "НАЗАД"},
    [LOCALIZED_TEXT_BEST] = {"BEST", "РЕКОРД"},
    [LOCALIZED_TEXT_GAME_OVER] = {"GAME OVER", "КІНЕЦЬ ГРИ"},
    [LOCALIZED_TEXT_EXIT] = {"EXIT", "ВИХІД"},
    [LOCALIZED_TEXT_LANGUAGE] = {"LANGUAGE", "МОВА"},
    [LOCALIZED_TEXT_LANGUAGE_ENGLISH] = {"EN", "АНГЛ"},
    [LOCALIZED_TEXT_LANGUAGE_UKRAINIAN] = {"UK", "УКР"},
    [LOCALIZED_TEXT_MUSIC] = {"MUSIC", "МУЗИКА"},
    [LOCALIZED_TEXT_PAUSED] = {"PAUSED", "ПАУЗА"},
    [LOCALIZED_TEXT_RESUME] = {"RESUME", "ПРОДОВЖИТИ"},
    [LOCALIZED_TEXT_RETRY] = {"RETRY", "ЩЕ РАЗ"},
    [LOCALIZED_TEXT_RUNS] = {"RUNS", "СПРОБИ"},
    [LOCALIZED_TEXT_SCORE] = {"SCORE", "РАХУНОК"},
    [LOCALIZED_TEXT_SETTINGS] = {"SETTINGS", "НАЛАШТУВАННЯ"},
    [LOCALIZED_TEXT_SFX] = {"SFX", "ЗВУКИ"},
    [LOCALIZED_TEXT_START] = {"START", "СТАРТ"},
    [LOCALIZED_TEXT_TIME] = {"TIME", "ЧАС"},
    [LOCALIZED_TEXT_TOTAL] = {"TOTAL", "ВСЬОГО"}};

const char *localization_text(GameLocale locale, LocalizedTextId text_id)
{
    if (text_id < 0 || (int)text_id >= (int)(sizeof(LOCALIZED_TEXTS) / sizeof(LOCALIZED_TEXTS[0])))
    {
        return "";
    }

    if (game_settings_normalize_locale(locale) == GAME_LOCALE_UKRAINIAN)
    {
        return LOCALIZED_TEXTS[text_id].ukrainian;
    }

    return LOCALIZED_TEXTS[text_id].english;
}
