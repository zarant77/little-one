#ifndef LITTLE_ONE_CAT_PROFILE_H
#define LITTLE_ONE_CAT_PROFILE_H

#include "../config/player_config.h"
#include "progression.h"

typedef struct {
    const char* id;
    const char* name;
    const char* sprite_id;
    const char* music_id;
    int required_best_score;
    int required_total_score;
    int required_runs;
    PlayerConfig config;
} CatProfile;

int cat_profile_count(void);
const CatProfile* cat_profile_get(int index);
const CatProfile* cat_profile_default(void);
int cat_profile_is_unlocked(const CatProfile* profile, const ProgressionState* progress);
int cat_profile_first_new_unlock(
        const ProgressionState* before,
        const ProgressionState* after
);
int cat_profile_clamp_index(int index);

#endif
