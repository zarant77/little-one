#include "progression.h"

#include <stdio.h>

#define PROGRESSION_SAVE_VERSION 1

void progression_init(ProgressionState* progress)
{
    if (progress == 0)
    {
        return;
    }

    progress->best_score = 0;
    progress->total_score = 0;
    progress->total_runs = 0;
    progress->selected_cat_index = 0;
}

void progression_apply_run(ProgressionState* progress, int score)
{
    if (progress == 0)
    {
        return;
    }

    if (score < 0)
    {
        score = 0;
    }

    if (score > progress->best_score)
    {
        progress->best_score = score;
    }

    progress->total_score += score;
    progress->total_runs += 1;
}

int progression_load_from_path(const char* path, ProgressionState* progress)
{
    FILE* file;
    int version;
    ProgressionState loaded;

    if (path == 0 || progress == 0)
    {
        return 0;
    }

    file = fopen(path, "r");
    if (file == 0)
    {
        return 0;
    }

    progression_init(&loaded);
    if (fscanf(
            file,
            "little_one_progress %d\nbest %d\ntotal %d\nruns %d\ncat %d\n",
            &version,
            &loaded.best_score,
            &loaded.total_score,
            &loaded.total_runs,
            &loaded.selected_cat_index) != 5)
    {
        fclose(file);
        return 0;
    }

    fclose(file);
    if (version != PROGRESSION_SAVE_VERSION)
    {
        return 0;
    }

    if (loaded.best_score < 0)
    {
        loaded.best_score = 0;
    }
    if (loaded.total_score < 0)
    {
        loaded.total_score = 0;
    }
    if (loaded.total_runs < 0)
    {
        loaded.total_runs = 0;
    }

    *progress = loaded;
    return 1;
}

int progression_save_to_path(const char* path, const ProgressionState* progress)
{
    FILE* file;
    int result;

    if (path == 0 || progress == 0)
    {
        return 0;
    }

    file = fopen(path, "w");
    if (file == 0)
    {
        return 0;
    }

    result = fprintf(
            file,
            "little_one_progress %d\nbest %d\ntotal %d\nruns %d\ncat %d\n",
            PROGRESSION_SAVE_VERSION,
            progress->best_score,
            progress->total_score,
            progress->total_runs,
            progress->selected_cat_index);

    fclose(file);
    return result > 0;
}
