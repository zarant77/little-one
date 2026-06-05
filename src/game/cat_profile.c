#include "cat_profile.h"

#define CAT_PROFILE(profile_id, display_name, sprite_name, music_name, sprite_enum, best_score, total_score, runs, tint) \
    { \
        .id = profile_id, \
        .name = display_name, \
        .sprite_id = sprite_name, \
        .music_id = music_name, \
        .required_best_score = best_score, \
        .required_total_score = total_score, \
        .required_runs = runs, \
        .config = { \
            .hp = 3, \
            .moveSpeed = 600.0f, \
            .jumpVelocity = -1800.0f, \
            .smashVelocity = 4000.0f, \
            .hurt_zone = {.x = 0, .y = 0, .radius = 70}, \
            .boundary = {.x = 0, .y = 0, .width = 250, .height = 160}, \
            .visual = { \
                .width = 250, \
                .height = 160, \
                .color = tint, \
                .sprite_id = sprite_enum, \
                .animationId = "idle", \
                .deathAnimationId = "player_death_fall", \
            }, \
        }, \
    }

static const CatProfile CAT_PROFILES[] = {
    CAT_PROFILE("batcat", "BATCAT", "batcat", "batcat", SPRITE_BATCAT, 0, 0, 0, 0x171a20ff),
    CAT_PROFILE("iron_cat", "IRON CAT", "iron_cat", "iron_cat", SPRITE_IRON_CAT, 0, 0, 0, 0xb11f2aff),
    CAT_PROFILE("darkcat", "DARKCAT", "darkcat", "darkcat", SPRITE_DARKCAT, 30, 0, 0, 0x1d1f28ff),
    CAT_PROFILE("robocat", "ROBOCAT", "robocat", "robocat", SPRITE_ROBOCAT, 0, 120, 0, 0x9ca6afff),
    CAT_PROFILE("termicator", "TERMICATOR", "termicator", "termicator", SPRITE_TERMICATOR, 60, 0, 5, 0x4b4f56ff),
    CAT_PROFILE("punishcat", "PUNISHCAT", "punishcat", "punishcat", SPRITE_PUNISHCAT, 0, 220, 0, 0x101218ff),
    CAT_PROFILE("carrambacat", "CARRAMBACAT", "carrambacat", "carrambacat", SPRITE_CARRAMBACAT, 90, 0, 0, 0x7a4a2aff),
    CAT_PROFILE("commandocat", "COMMANDOCAT", "commandocat", "commandocat", SPRITE_COMMANDOCAT, 120, 0, 8, 0x355d34ff),
    CAT_PROFILE("silverpaw", "SILVERPAW", "silverpaw", "silverpaw", SPRITE_SILVERPAW, 0, 420, 0, 0xc9d4ddff),
    CAT_PROFILE("samurcat", "SAMURCAT", "samurcat", "samurcat", SPRITE_SAMURCAT, 150, 0, 12, 0x2f415cff),
    CAT_PROFILE("zombocat", "ZOMBOCAT", "zombocat", "zombocat", SPRITE_ZOMBOCAT, 0, 620, 0, 0x6f9b61ff),
    CAT_PROFILE("ghostcat", "GHOSTCAT", "ghostcat", "ghostcat", SPRITE_GHOSTCAT, 180, 0, 16, 0xdde8ffff),
    CAT_PROFILE("kotan_the_destroyer", "KOTAN THE DESTROYER", "kotan_the_destroyer", "kotan_the_destroyer", SPRITE_KOTAN_THE_DESTROYER, 220, 900, 20, 0xa85d36ff),
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
