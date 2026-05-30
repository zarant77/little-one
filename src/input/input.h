#ifndef LITTLE_ONE_INPUT_H
#define LITTLE_ONE_INPUT_H

typedef struct {
    int left;
    int right;
    int actionPressed;
    int actionHeld;
} InputState;

#define INPUT_TOUCH_DOWN 1
#define INPUT_TOUCH_MOVE 2
#define INPUT_TOUCH_UP 3
#define INPUT_TOUCH_CANCEL 4

void input_init(InputState* input);
void input_handle_touch(
        InputState* input,
        int action_type,
        int pointer_id,
        float x,
        float y,
        float screen_width
);
void input_end_frame(InputState* input);

#endif
