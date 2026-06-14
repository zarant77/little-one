#include "cat_profile.h"

#define CAT_PROFILE(profile_id, name_text, intro_text, sprite_name, music_name, sprite_enum, total_score) \
    {                                                                                                     \
        .id = profile_id,                                                                                 \
        .name_text_id = name_text,                                                                        \
        .intro_text_id = intro_text,                                                                      \
        .sprite_id = sprite_name,                                                                         \
        .music_id = music_name,                                                                           \
        .required_best_score = 0,                                                                         \
        .required_total_score = total_score,                                                              \
        .required_runs = 0,                                                                               \
        .config = {                                                                                       \
            .hp = 3,                                                                                      \
            .moveSpeed = 600.0f,                                                                          \
            .jumpVelocity = -1800.0f,                                                                     \
            .smashVelocity = 4000.0f,                                                                     \
            .hurt_zone = {.x = 0, .y = 0, .radius = 70},                                                  \
            .boundary = {.x = 0, .y = 0, .width = 250, .height = 160},                                    \
            .visual = {                                                                                   \
                .width = 250,                                                                             \
                .height = 160,                                                                            \
                .color = 0xffffffff,                                                                      \
                .sprite_id = sprite_enum,                                                                 \
                .animationId = "idle",                                                                    \
                .deathAnimationId = "player_death_fall",                                                  \
            },                                                                                            \
        },                                                                                                \
    }

static const CatProfile CAT_PROFILES[] = {
    // Test profiles with very low requirements for unlocking, for testing purposes
    CAT_PROFILE("pinkpawther", LOCALIZED_TEXT_CAT_PINKPAWTHER_NAME, LOCALIZED_TEXT_CAT_PINKPAWTHER_INTRO, "pinkpawther", "pinkpawther", SPRITE_PINKPAWTHER, 0),
    CAT_PROFILE("agent00cat", LOCALIZED_TEXT_CAT_AGENT00CAT_NAME, LOCALIZED_TEXT_CAT_AGENT00CAT_INTRO, "agent00cat", "agent00cat", SPRITE_AGENT00CAT, 20),
    CAT_PROFILE("rockpaw", LOCALIZED_TEXT_CAT_ROCKPAW_NAME, LOCALIZED_TEXT_CAT_ROCKPAW_INTRO, "rockpaw", "rockpaw", SPRITE_ROCKPAW, 40),
    CAT_PROFILE("impawsiblecat", LOCALIZED_TEXT_CAT_IMPAWSIBLECAT_NAME, LOCALIZED_TEXT_CAT_IMPAWSIBLECAT_INTRO, "impawsiblecat", "impawsiblecat", SPRITE_IMPAWSIBLECAT, 60),
    CAT_PROFILE("indicat", LOCALIZED_TEXT_CAT_INDICAT_NAME, LOCALIZED_TEXT_CAT_INDICAT_INTRO, "indicat", "indicat", SPRITE_INDICAT, 80),
    CAT_PROFILE("catbuster", LOCALIZED_TEXT_CAT_CATBUSTER_NAME, LOCALIZED_TEXT_CAT_CATBUSTER_INTRO, "catbuster", "catbuster", SPRITE_CATBUSTER, 100),
    CAT_PROFILE("harrypurrter", LOCALIZED_TEXT_CAT_HARRYPURRTER_NAME, LOCALIZED_TEXT_CAT_HARRYPURRTER_INTRO, "harrypurrter", "harrypurrter", SPRITE_HARRYPURRTER, 120),
    CAT_PROFILE("carrambacat", LOCALIZED_TEXT_CAT_CARRAMBACAT_NAME, LOCALIZED_TEXT_CAT_CARRAMBACAT_INTRO, "carrambacat", "carrambacat", SPRITE_CARRAMBACAT, 140),
    CAT_PROFILE("scarcat", LOCALIZED_TEXT_CAT_SCARCAT_NAME, LOCALIZED_TEXT_CAT_SCARCAT_INTRO, "scarcat", "scarcat", SPRITE_SCARCAT, 160),
    CAT_PROFILE("robocat", LOCALIZED_TEXT_CAT_ROBOCAT_NAME, LOCALIZED_TEXT_CAT_ROBOCAT_INTRO, "robocat", "robocat", SPRITE_ROBOCAT, 180),
    CAT_PROFILE("termicator", LOCALIZED_TEXT_CAT_TERMICATOR_NAME, LOCALIZED_TEXT_CAT_TERMICATOR_INTRO, "termicator", "termicator", SPRITE_TERMICATOR, 200),
    CAT_PROFILE("doomcat", LOCALIZED_TEXT_CAT_DOOMCAT_NAME, LOCALIZED_TEXT_CAT_DOOMCAT_INTRO, "doomcat", "doomcat", SPRITE_DOOMCAT, 220),
    CAT_PROFILE("jawscat", LOCALIZED_TEXT_CAT_JAWSCAT_NAME, LOCALIZED_TEXT_CAT_JAWSCAT_INTRO, "jawscat", "jawscat", SPRITE_JAWSCAT, 240),
    CAT_PROFILE("darthcat", LOCALIZED_TEXT_CAT_DARTHCAT_NAME, LOCALIZED_TEXT_CAT_DARTHCAT_INTRO, "darthcat", "darthcat", SPRITE_DARTHCAT, 260),

    // Original requirements for unlocking profiles, for actual gameplay
    // CAT_PROFILE("pinkpawther", LOCALIZED_TEXT_CAT_PINKPAWTHER_NAME, LOCALIZED_TEXT_CAT_PINKPAWTHER_INTRO, "pinkpawther", "pinkpawther", SPRITE_PINKPAWTHER, 0),
    // CAT_PROFILE("agent00cat", LOCALIZED_TEXT_CAT_AGENT00CAT_NAME, LOCALIZED_TEXT_CAT_AGENT00CAT_INTRO, "agent00cat", "agent00cat", SPRITE_AGENT00CAT, 50),
    // CAT_PROFILE("rockpaw", LOCALIZED_TEXT_CAT_ROCKPAW_NAME, LOCALIZED_TEXT_CAT_ROCKPAW_INTRO, "rockpaw", "rockpaw", SPRITE_ROCKPAW, 100),
    // CAT_PROFILE("impawsiblecat", LOCALIZED_TEXT_CAT_IMPAWSIBLECAT_NAME, LOCALIZED_TEXT_CAT_IMPAWSIBLECAT_INTRO, "impawsiblecat", "impawsiblecat", SPRITE_IMPAWSIBLECAT, 150),
    // CAT_PROFILE("indicat", LOCALIZED_TEXT_CAT_INDICAT_NAME, LOCALIZED_TEXT_CAT_INDICAT_INTRO, "indicat", "indicat", SPRITE_INDICAT, 250),
    // CAT_PROFILE("catbuster", LOCALIZED_TEXT_CAT_CATBUSTER_NAME, LOCALIZED_TEXT_CAT_CATBUSTER_INTRO, "catbuster", "catbuster", SPRITE_CATBUSTER, 350),
    // CAT_PROFILE("harrypurrter", LOCALIZED_TEXT_CAT_HARRYPURRTER_NAME, LOCALIZED_TEXT_CAT_HARRYPURRTER_INTRO, "harrypurrter", "harrypurrter", SPRITE_HARRYPURRTER, 500),
    // CAT_PROFILE("carrambacat", LOCALIZED_TEXT_CAT_CARRAMBACAT_NAME, LOCALIZED_TEXT_CAT_CARRAMBACAT_INTRO, "carrambacat", "carrambacat", SPRITE_CARRAMBACAT, 650),
    // CAT_PROFILE("scarcat", LOCALIZED_TEXT_CAT_SCARCAT_NAME, LOCALIZED_TEXT_CAT_SCARCAT_INTRO, "scarcat", "scarcat", SPRITE_SCARCAT, 800),
    // CAT_PROFILE("robocat", LOCALIZED_TEXT_CAT_ROBOCAT_NAME, LOCALIZED_TEXT_CAT_ROBOCAT_INTRO, "robocat", "robocat", SPRITE_ROBOCAT, 1000),
    // CAT_PROFILE("termicator", LOCALIZED_TEXT_CAT_TERMICATOR_NAME, LOCALIZED_TEXT_CAT_TERMICATOR_INTRO, "termicator", "termicator", SPRITE_TERMICATOR, 1200),
    // CAT_PROFILE("doomcat", LOCALIZED_TEXT_CAT_DOOMCAT_NAME, LOCALIZED_TEXT_CAT_DOOMCAT_INTRO, "doomcat", "doomcat", SPRITE_DOOMCAT, 1400),
    // CAT_PROFILE("jawscat", LOCALIZED_TEXT_CAT_JAWSCAT_NAME, LOCALIZED_TEXT_CAT_JAWSCAT_INTRO, "jawscat", "jawscat", SPRITE_JAWSCAT, 1600),
    // CAT_PROFILE("darthcat", LOCALIZED_TEXT_CAT_DARTHCAT_NAME, LOCALIZED_TEXT_CAT_DARTHCAT_INTRO, "darthcat", "darthcat", SPRITE_DARTHCAT, 1800),
};

#undef CAT_PROFILE

int cat_profile_count(void)
{
    return (int)(sizeof(CAT_PROFILES) / sizeof(CAT_PROFILES[0]));
}

int cat_profile_clamp_index(int index)
{
    int count = cat_profile_count();

    if (index < 0)
    {
        return 0;
    }

    if (index >= count)
    {
        return count - 1;
    }

    return index;
}

const CatProfile *cat_profile_get(int index)
{
    return CAT_PROFILES + cat_profile_clamp_index(index);
}

const CatProfile *cat_profile_default(void)
{
    return CAT_PROFILES;
}

int cat_profile_is_unlocked(const CatProfile *profile, const ProgressionState *progress)
{
    int best_score;
    int total_score;
    int total_runs;

    if (profile == 0)
    {
        return 0;
    }

    if (profile->required_best_score <= 0 && profile->required_total_score <= 0 && profile->required_runs <= 0)
    {
        return 1;
    }

    if (progress == 0)
    {
        return 0;
    }

    best_score = progress->best_score;
    total_score = progress->total_score;
    total_runs = progress->total_runs;

    return (profile->required_best_score <= 0 || best_score >= profile->required_best_score) && (profile->required_total_score <= 0 || total_score >= profile->required_total_score) && (profile->required_runs <= 0 || total_runs >= profile->required_runs);
}

int cat_profile_first_new_unlock(
    const ProgressionState *before,
    const ProgressionState *after)
{
    int count = cat_profile_count();

    for (int index = 0; index < count; ++index)
    {
        const CatProfile *profile = cat_profile_get(index);

        if (!cat_profile_is_unlocked(profile, before) && cat_profile_is_unlocked(profile, after))
        {
            return index;
        }
    }

    return -1;
}
