#include "input.h"

#include "../game/game_settings.h"

#define INPUT_NO_POINTER -1

static int movement_pointer_id = INPUT_NO_POINTER;
static int action_pointer_id = INPUT_NO_POINTER;
static float movement_start_x = 0.0f;

static int input_is_movement_zone(float x, float screen_width) {
    return x < screen_width * LITTLE_ONE_MOVEMENT_TOUCH_ZONE_RATIO;
}

static void input_clear_movement(InputState* input) {
    movement_pointer_id = INPUT_NO_POINTER;

    if (input == 0) {
        return;
    }

    input->left = 0;
    input->right = 0;
}

static void input_update_movement(InputState* input, float x) {
    float delta_x;

    if (input == 0) {
        return;
    }

    input->left = 0;
    input->right = 0;

    delta_x = x - movement_start_x;
    if (delta_x < -LITTLE_ONE_MOVEMENT_DEADZONE_PX) {
        input->left = 1;
    } else if (delta_x > LITTLE_ONE_MOVEMENT_DEADZONE_PX) {
        input->right = 1;
    }
}

void input_init(InputState* input) {
    movement_pointer_id = INPUT_NO_POINTER;
    action_pointer_id = INPUT_NO_POINTER;
    movement_start_x = 0.0f;

    if (input == 0) {
        return;
    }

    input->left = 0;
    input->right = 0;
    input->actionPressed = 0;
    input->actionHeld = 0;
}

static void input_pointer_down(InputState* input, int pointer_id, float x, float y, float screen_width) {
    (void)y;

    if (input == 0) {
        return;
    }

    if (input_is_movement_zone(x, screen_width)) {
        if (movement_pointer_id == INPUT_NO_POINTER) {
            movement_pointer_id = pointer_id;
            movement_start_x = x;
            input->left = 0;
            input->right = 0;
        }

        return;
    }

    if (action_pointer_id == INPUT_NO_POINTER) {
        action_pointer_id = pointer_id;
        input->actionPressed = 1;
        input->actionHeld = 1;
    }
}

static void input_pointer_move(InputState* input, int pointer_id, float x, float y, float screen_width) {
    (void)y;
    (void)screen_width;

    if (pointer_id != movement_pointer_id) {
        return;
    }

    input_update_movement(input, x);
}

static void input_pointer_up(InputState* input, int pointer_id) {
    if (pointer_id == movement_pointer_id) {
        input_clear_movement(input);
    }

    if (pointer_id == action_pointer_id) {
        action_pointer_id = INPUT_NO_POINTER;

        if (input != 0) {
            input->actionHeld = 0;
        }
    }
}

static void input_cancel(InputState* input) {
    input_clear_movement(input);
    action_pointer_id = INPUT_NO_POINTER;

    if (input == 0) {
        return;
    }

    input->actionPressed = 0;
    input->actionHeld = 0;
}

void input_handle_touch(
        InputState* input,
        int action_type,
        int pointer_id,
        float x,
        float y,
        float screen_width
) {
    if (action_type == INPUT_TOUCH_DOWN) {
        input_pointer_down(input, pointer_id, x, y, screen_width);
        return;
    }

    if (action_type == INPUT_TOUCH_MOVE) {
        input_pointer_move(input, pointer_id, x, y, screen_width);
        return;
    }

    if (action_type == INPUT_TOUCH_UP) {
        input_pointer_up(input, pointer_id);
        return;
    }

    if (action_type == INPUT_TOUCH_CANCEL) {
        input_cancel(input);
    }
}

void input_end_frame(InputState* input) {
    if (input == 0) {
        return;
    }

    input->actionPressed = 0;
}
