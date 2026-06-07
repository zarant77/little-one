#include "localization.h"

typedef struct
{
    const char *english;
    const char *ukrainian;
} LocalizedText;

static const LocalizedText LOCALIZED_TEXTS[] = {
    [LOCALIZED_TEXT_BACK] = {"BACK", "НАЗАД"},
    [LOCALIZED_TEXT_BEST] = {"BEST", "РЕКОРД"},
    [LOCALIZED_TEXT_CHANGE_CAT] = {"CHANGE CAT", "ЗМІНИТИ КОТА"},
    [LOCALIZED_TEXT_CHOOSE_CAT] = {"CHOOSE CAT", "ОБЕРИ КОТА"},
    [LOCALIZED_TEXT_CONGRATULATIONS] = {"CONGRATULATIONS", "ВІТАЄМО"},
    [LOCALIZED_TEXT_CONTINUE] = {"CONTINUE", "ДАЛІ"},
    [LOCALIZED_TEXT_GAME_OVER] = {"GAME OVER", "КІНЕЦЬ ГРИ"},
    [LOCALIZED_TEXT_EXIT] = {"EXIT", "ВИХІД"},
    [LOCALIZED_TEXT_LANGUAGE] = {"LANGUAGE", "МОВА"},
    [LOCALIZED_TEXT_LANGUAGE_ENGLISH] = {"EN", "АНГЛ"},
    [LOCALIZED_TEXT_LANGUAGE_UKRAINIAN] = {"UK", "УКР"},
    [LOCALIZED_TEXT_LOCK] = {"LOCK", "ЗАМОК"},
    [LOCALIZED_TEXT_LOCKED] = {"LOCKED", "ЗАКРИТО"},
    [LOCALIZED_TEXT_MUSIC] = {"MUSIC", "МУЗИКА"},
    [LOCALIZED_TEXT_ALL_CATS_READY] = {"ALL CATS READY", "УСІ КОТИ ГОТОВІ"},
    [LOCALIZED_TEXT_PAUSED] = {"PAUSED", "ПАУЗА"},
    [LOCALIZED_TEXT_RESUME] = {"RESUME", "ПРОДОВЖИТИ"},
    [LOCALIZED_TEXT_RETRY] = {"RETRY", "ЩЕ РАЗ"},
    [LOCALIZED_TEXT_RUNS] = {"RUNS", "СПРОБИ"},
    [LOCALIZED_TEXT_SCORE] = {"SCORE", "РАХУНОК"},
    [LOCALIZED_TEXT_SETTINGS] = {"SETTINGS", "НАЛАШТУВАННЯ"},
    [LOCALIZED_TEXT_SFX] = {"SFX", "ЗВУКИ"},
    [LOCALIZED_TEXT_START] = {"START", "СТАРТ"},
    [LOCALIZED_TEXT_TIME] = {"TIME", "ЧАС"},
    [LOCALIZED_TEXT_TOTAL] = {"TOTAL", "ВСЬОГО"},
    [LOCALIZED_TEXT_CAT_AGENT00CAT_NAME] = {"AGENT 00CAT", "АГЕНТ 00КІТ"},
    [LOCALIZED_TEXT_CAT_CARRAMBACAT_NAME] = {"CARRAMBACAT", "КАРРАМБАКІТ"},
    [LOCALIZED_TEXT_CAT_CATBUSTER_NAME] = {"CATBUSTER", "КОТОЛОВЕЦЬ"},
    [LOCALIZED_TEXT_CAT_DARTHCAT_NAME] = {"DARTH CAT", "ДАРТ КІТ"},
    [LOCALIZED_TEXT_CAT_DOOMCAT_NAME] = {"DOOMCAT", "ДУМКІТ"},
    [LOCALIZED_TEXT_CAT_HARRYPURRTER_NAME] = {"HARRY PURRTER", "ГАРРІ МУРКОТЕР"},
    [LOCALIZED_TEXT_CAT_IMPAWSIBLECAT_NAME] = {"IMPAWSIBLE CAT", "НЯВМОЖЛИВИЙ КІТ"},
    [LOCALIZED_TEXT_CAT_INDICAT_NAME] = {"INDICAT", "ІНДІКІТ"},
    [LOCALIZED_TEXT_CAT_JAWSCAT_NAME] = {"PAWS", "ПАЗУРІ"},
    [LOCALIZED_TEXT_CAT_PINKPAWTHER_NAME] = {"PINK PAWTHER", "РОЖЕВА ЛАПТЕРА"},
    [LOCALIZED_TEXT_CAT_ROBOCAT_NAME] = {"ROBOCAT", "РОБОКІТ"},
    [LOCALIZED_TEXT_CAT_ROCKPAW_NAME] = {"ROCKY PAWBOA", "РОКІ ЛАПБОА"},
    [LOCALIZED_TEXT_CAT_SCARCAT_NAME] = {"TONY CATANA", "ТОНІ КОТАНА"},
    [LOCALIZED_TEXT_CAT_TERMICATOR_NAME] = {"TERMICATOR", "ТЕРМІКАТОР"},
    [LOCALIZED_TEXT_CAT_AGENT00CAT_INTRO] = {"LICENSED TO PURR", "ЛІЦЕНЗІЯ НА МУРКОТІННЯ"},
    [LOCALIZED_TEXT_CAT_CARRAMBACAT_INTRO] = {"CARRAMBA CLAWS READY", "КАРРАМБА КІГТІ ГОТОВІ"},
    [LOCALIZED_TEXT_CAT_CATBUSTER_INTRO] = {"WHO YOU GONNA CALL?", "КОГО ПОКЛИЧЕШ?"},
    [LOCALIZED_TEXT_CAT_DARTHCAT_INTRO] = {"YOU UNDERESTIMATE MY PURR", "ТИ НЕДООЦІНЮЄШ МОЄ МУРКОТІННЯ"},
    [LOCALIZED_TEXT_CAT_DOOMCAT_INTRO] = {"RIP AND PURR", "РВИ І МУРКОЧИ"},
    [LOCALIZED_TEXT_CAT_HARRYPURRTER_INTRO] = {"EXPECTO PAWTRONUM", "ЕКСПЕКТО ЛАПТРОНУМ"},
    [LOCALIZED_TEXT_CAT_IMPAWSIBLECAT_INTRO] = {"YOUR MISSION SHOULD YOU CHOOSE TO ACCEPT IT...", "ВАША МІСІЯ, ЯКЩО ВИ ВИРІШИТЕ ПРИЙНЯТИ ЇЇ..."},
    [LOCALIZED_TEXT_CAT_INDICAT_INTRO] = {"THIS BELONGS IN A MUSEUM", "ЦЕ МАЄ БУТИ В МУЗЕЇ"},
    [LOCALIZED_TEXT_CAT_JAWSCAT_INTRO] = {"WE NEED A BIGGER BOWL", "НАМ ТРЕБА БІЛЬША МИСКА"},
    [LOCALIZED_TEXT_CAT_PINKPAWTHER_INTRO] = {"SMOOTH PAWS ONLY", "ТІЛЬКИ ПЛАВНІ ЛАПИ"},
    [LOCALIZED_TEXT_CAT_ROBOCAT_INTRO] = {"DIRECTIVE 5. PURR", "ДИРЕКТИВА 5. МУРКОТІННЯ"},
    [LOCALIZED_TEXT_CAT_ROCKPAW_INTRO] = {"YO ADRIAN. I DID IT!", "ЙО ЕДРІАН. Я ЗРОБИВ ЦЕ!"},
    [LOCALIZED_TEXT_CAT_SCARCAT_INTRO] = {"SAY HELLO TO MY LITTLE CLAWS", "СКАЖИ ПРИВІТ МОЇМ МАЛЕНЬКИМ КІГТЯМ"},
    [LOCALIZED_TEXT_CAT_TERMICATOR_INTRO] = {"I SAID ILL BE BACK", "Я Ж КАЗАВ ЩО ПОВЕРНУСЬ"}};

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
