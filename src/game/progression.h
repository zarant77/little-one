#ifndef LITTLE_ONE_PROGRESSION_H
#define LITTLE_ONE_PROGRESSION_H

typedef struct {
    int best_score;
    int total_score;
    int total_runs;
    int selected_cat_index;
} ProgressionState;

void progression_init(ProgressionState* progress);
void progression_apply_score(ProgressionState* progress, int score);
void progression_apply_run(ProgressionState* progress, int score);
int progression_load_from_path(const char* path, ProgressionState* progress);
int progression_save_to_path(const char* path, const ProgressionState* progress);

#endif
