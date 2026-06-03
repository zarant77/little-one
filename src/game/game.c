#include "game.h"

#include <android/log.h>

#include "../audio/audio.h"
#include "../config.h"
#include "../config/enemy_config.h"
#include "../config/obstacle_config.h"
#include "../config/player_config.h"
#include "../feedback/game_feedback.h"
#include "game_settings.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)

static const float GRAVITY = 3000.0f;
static const float WORLD_SCROLL_WRAP = 10000.0f;
static const float SPAWN_MIN_SECONDS = 1.5f;
static const float SPAWN_SECONDS_RANGE = 1.0f;
static const float SPAWN_RIGHT_PADDING = 16.0f;
static const int PLAYER_HIT_INVULNERABLE_MS = 900;
static unsigned int game_random_state = 12345;

static float game_player_width(void) {
    return (float)player_config_get()->visual.width;
}

static float game_player_height(void) {
    return (float)player_config_get()->visual.height;
}

static float game_ground_y(const GameState* game) {
    return (float)game->screenHeight - LITTLE_ONE_GROUND_BOTTOM_MARGIN_PX;
}

static float game_random_01(void) {
    game_random_state = game_random_state * 1103515245 + 12345;

    return (float)((game_random_state / 65536) % 32768) / 32767.0f;
}

static float game_next_spawn_time(void) {
    return SPAWN_MIN_SECONDS + game_random_01() * SPAWN_SECONDS_RANGE;
}

static int game_random_index(int count) {
    int index;

    if (count <= 0) {
        return -1;
    }

    index = (int)(game_random_01() * (float)count);
    if (index >= count) {
        index = count - 1;
    }

    return index;
}

static EntityAnimSlot game_player_animation_slot(const GameState* game) {
    if (game->gameOver) {
        return ENTITY_ANIM_DEATH;
    }

    if (game->playerInvulnerableMs > 0) {
        return ENTITY_ANIM_DAMAGE;
    }

    if (game->playerSmashing) {
        return ENTITY_ANIM_SMASH;
    }

    if (!game->playerGrounded) {
        return game->playerVelocityY < 0.0f ? ENTITY_ANIM_JUMP : ENTITY_ANIM_FALL;
    }

    if (game->playerVelocityX != 0.0f) {
        return ENTITY_ANIM_WALK;
    }

    return ENTITY_ANIM_IDLE;
}

static void game_update_player_animation(GameState* game, int32_t elapsed_ms) {
    entity_animation_set(
            &game->playerAnimation,
            entity_animation_player_config(),
            game_player_animation_slot(game)
    );
    entity_animation_update(&game->playerAnimation, elapsed_ms);
}

static void game_clamp_player_x(GameState* game) {
    float max_x = (float)game->screenWidth - game_player_width();

    if (max_x < 0.0f) {
        max_x = 0.0f;
    }

    if (game->playerX < 0.0f) {
        game->playerX = 0.0f;
    }

    if (game->playerX > max_x) {
        game->playerX = max_x;
    }
}

static void game_clamp_player_to_ground(GameState* game) {
    float ground_y = game_ground_y(game);
    float player_bottom = game->playerY + game_player_height();
    int was_grounded = game->playerGrounded;
    int was_smashing = game->playerSmashing;

    if (game->playerY < 0.0f) {
        game->playerY = 0.0f;
        if (game->playerVelocityY < 0.0f) {
            game->playerVelocityY = 0.0f;
        }
    }

    if (player_bottom >= ground_y) {
        game->playerY = ground_y - game_player_height();
        if (game->playerY < 0.0f) {
            game->playerY = 0.0f;
        }
        game->playerVelocityY = 0.0f;
        game->playerGrounded = 1;
        game->playerSmashing = 0;
        game->playerCanSmash = 0;
        if (!was_grounded) {
            if (was_smashing) {
                audio_play_sound("smash");
                game_feedback_smash_land(&game->screenShake);
            }
            #if LITTLE_ONE_DEBUG_SMASH
            LOGI("Landing");
            #endif
        }
    } else {
        game->playerGrounded = 0;
    }
}

static void game_clear_entities(GameState* game) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        entity_clear(game->entities + entity_index);
    }
}

static void game_shift_entities_y(GameState* game, float y_delta) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        Entity* entity = game->entities + entity_index;

        if (entity->active) {
            entity->y += y_delta;
        }
    }
}

static Entity* game_find_free_entity(GameState* game) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        if (!game->entities[entity_index].active) {
            return game->entities + entity_index;
        }
    }

    return 0;
}

static void game_spawn_entity(GameState* game) {
    Entity* entity = game_find_free_entity(game);
    float x = (float)game->screenWidth + SPAWN_RIGHT_PADDING;
    float y;
    float y_offset;

    if (entity == 0 || game->screenWidth <= 0 || game->screenHeight <= 0) {
        return;
    }

    if (game_random_01() < 0.5f) {
        int enemy_count = 0;
        int enemy_index;
        const EnemyConfig* enemy_config;

        enemy_config_get_all(&enemy_count);
        enemy_index = game_random_index(enemy_count);
        enemy_config = enemy_config_get(enemy_index);
        if (enemy_config == 0) {
            return;
        }

        y_offset = enemy_config->yMin + game_random_01() * (enemy_config->yMax - enemy_config->yMin);
        y = game_ground_y(game) + y_offset - (float)enemy_config->visual.height;
        entity_spawn_enemy(entity, enemy_config, x, y);
    } else {
        int obstacle_count = 0;
        int obstacle_index;
        const ObstacleConfig* obstacle_config;

        obstacle_config_get_all(&obstacle_count);
        obstacle_index = game_random_index(obstacle_count);
        obstacle_config = obstacle_config_get(obstacle_index);
        if (obstacle_config == 0) {
            return;
        }

        y = game_ground_y(game) - (float)obstacle_config->visual.height;
        entity_spawn_obstacle(entity, obstacle_config, x, y);
    }
}

static void game_update_spawn_timer(GameState* game, float dt) {
    game->spawnTimer -= dt;
    if (game->spawnTimer > 0.0f) {
        return;
    }

    game_spawn_entity(game);
    game->spawnTimer = game_next_spawn_time();
}

static void game_update_entities(GameState* game, float dt) {
    int active_count = 0;

    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        Entity* entity = game->entities + entity_index;

        if (!entity->active) {
            continue;
        }

        entity_update(entity, game->worldSpeed, dt);
        if (entity->x + (float)entity_get_width(entity) < 0.0f) {
            entity_clear(entity);
        } else {
            active_count += 1;
        }
    }

    game->activeEntityCount = active_count;
}

static int game_player_overlaps_entity_hurt_zone(const GameState* game, const Entity* entity) {
    return hurt_zones_overlap(
            (int32_t)game->playerX,
            (int32_t)game->playerY,
            player_config_get()->visual.width,
            player_config_get()->visual.height,
            &player_config_get()->hurt_zone,
            (int32_t)entity->x,
            (int32_t)entity->y,
            entity_get_width(entity),
            entity_get_height(entity),
            entity_get_hurt_zone(entity)
    );
}

static int game_player_boundary_overlaps_enemy_hurt_zone(const GameState* game, const Entity* enemy) {
    if (enemy == 0 || enemy->type != ENTITY_ENEMY) {
        return 0;
    }

    return rect_overlaps_hurt_zone(
            (int32_t)game->playerX,
            (int32_t)game->playerY,
            &player_config_get()->boundary,
            (int32_t)enemy->x,
            (int32_t)enemy->y,
            entity_get_width(enemy),
            entity_get_height(enemy),
            entity_get_hurt_zone(enemy)
    );
}

static const char* game_enemy_death_sound_id(const Entity* entity) {
    const char* id;

    if (entity == 0 || entity->enemyConfig == 0 || entity->enemyConfig->id == 0) {
        return "death";
    }

    id = entity->enemyConfig->id;
    if (id[0] == 'e' && id[1] == 'n' && id[2] == 'e' && id[3] == 'm' && id[4] == 'y' && id[5] == '.') {
        id += 6;

        if (id[0] == 'o' && id[1] == 'r' && id[2] == 'k' && id[3] == 0) {
            return "ork_death";
        }
        if (id[0] == 'b' && id[1] == 'o' && id[2] == 'a' && id[3] == 'r' && id[4] == 0) {
            return "boar_death";
        }
        if (id[0] == 'r' && id[1] == 'a' && id[2] == 't' && id[3] == 0) {
            return "rat_death";
        }
    }

    return "death";
}

static void game_kill_enemy_by_smash(GameState* game, Entity* entity) {
    if (entity->enemyConfig != 0) {
        game->score += entity->enemyConfig->scoreValue;
    }
    audio_play_sound(game_enemy_death_sound_id(entity));
    entity_clear(entity);
    #if LITTLE_ONE_DEBUG_SMASH
    LOGI("Enemy killed by smash");
    #endif
}

static void game_handle_player_death(GameState* game) {
    game->gameOver = 1;
    entity_animation_set(
            &game->playerAnimation,
            entity_animation_player_config(),
            ENTITY_ANIM_DEATH
    );
    audio_play_sound("death");
    game_feedback_player_death(&game->screenShake);
    if (game->score > game->bestScore) {
        game->bestScore = game->score;
    }
}

static void game_damage_player(GameState* game) {
    if (game->playerInvulnerableMs > 0 || game->playerHp <= 0) {
        return;
    }

    game->playerHp -= 1;
    if (game->playerHp <= 0) {
        game_handle_player_death(game);
        return;
    }

    game->playerInvulnerableMs = PLAYER_HIT_INVULNERABLE_MS;
    entity_animation_set(
            &game->playerAnimation,
            entity_animation_player_config(),
            ENTITY_ANIM_DAMAGE
    );
    audio_play_sound("damage");
}

static void game_handle_collisions(GameState* game, int player_was_smashing) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        Entity* entity = game->entities + entity_index;

        if (!entity->active) {
            continue;
        }

        if (entity->type == ENTITY_ENEMY
                && player_was_smashing
                && game_player_boundary_overlaps_enemy_hurt_zone(game, entity)) {
            game_kill_enemy_by_smash(game, entity);
            continue;
        }

        if (!game_player_overlaps_entity_hurt_zone(game, entity)) {
            continue;
        }

        game_damage_player(game);
        return;
    }
}

void game_init(GameState* game) {
    int best_score;

    if (game == 0) {
        return;
    }

    best_score = game->bestScore;

    game->playerX = 0.0f;
    game->playerY = 0.0f;
    game->playerVelocityX = 0.0f;
    game->playerVelocityY = 0.0f;
    game->playerGrounded = 0;
    game->playerSmashing = 0;
    game->playerCanSmash = 0;
    game->playerHp = player_config_get()->hp;
    game->playerInvulnerableMs = 0;
    game->playerAnimation.slot = ENTITY_ANIM_IDLE;
    game->playerAnimation.clip = 0;
    game->playerAnimation.time_ms = 0;
    entity_animation_set(
            &game->playerAnimation,
            entity_animation_player_config(),
            ENTITY_ANIM_IDLE
    );
    game->screenWidth = 0;
    game->screenHeight = 0;
    game->worldScrollX = 0.0f;
    game->worldSpeed = LITTLE_ONE_WORLD_SCROLL_SPEED;
    game_clear_entities(game);
    game->spawnTimer = game_next_spawn_time();
    game->gameOver = 0;
    game->score = 0;
    game->bestScore = best_score;
    game->fps = 0;
    game->averageFrameMs = 0;
    game->activeEntityCount = 0;
    screen_shake_start(&game->screenShake, 0, 0, 1u);
}

const EntityVisualConfig* game_player_visual_config(void) {
    return &player_config_get()->visual;
}

const HurtZone* game_player_hurt_zone_config(void) {
    return &player_config_get()->hurt_zone;
}

void game_set_screen_size(GameState* game, float width, float height) {
    int old_screen_width;
    int old_screen_height;
    float old_ground_y;
    float new_ground_y;
    float player_ground_offset;
    int was_grounded;

    if (game == 0) {
        return;
    }

    old_screen_width = game->screenWidth;
    old_screen_height = game->screenHeight;
    old_ground_y = game_ground_y(game);
    player_ground_offset = 0.0f;
    was_grounded = game->playerGrounded;

    if (old_screen_width > 0 && old_screen_height > 0) {
        player_ground_offset = old_ground_y - (game->playerY + game_player_height());
        if (was_grounded || player_ground_offset < 0.0f) {
            player_ground_offset = 0.0f;
        }
    }

    game->screenWidth = (int)width;
    game->screenHeight = (int)height;
    new_ground_y = game_ground_y(game);

    if (old_screen_width <= 0 || old_screen_height <= 0) {
        game->playerX = 0.0f;
        game->playerY = new_ground_y - game_player_height();
        game->playerVelocityX = 0.0f;
        game->playerVelocityY = 0.0f;
        game->playerGrounded = 1;
        game->playerSmashing = 0;
        game->playerCanSmash = 0;
    } else if (old_screen_width != game->screenWidth || old_screen_height != game->screenHeight) {
        game->playerY = new_ground_y - game_player_height() - player_ground_offset;
        game_shift_entities_y(game, new_ground_y - old_ground_y);
        if (was_grounded) {
            game->playerVelocityY = 0.0f;
            game->playerGrounded = 1;
            game->playerSmashing = 0;
            game->playerCanSmash = 0;
        } else if (game->playerY + game_player_height() >= new_ground_y) {
            game->playerVelocityY = 0.0f;
        }
    }

    game_clamp_player_x(game);
    game_clamp_player_to_ground(game);
}

void game_update(GameState* game, const InputState* input, float dt) {
    int player_was_smashing;
    int32_t elapsed_ms;

    if (game == 0) {
        return;
    }

    elapsed_ms = (int32_t)(dt * 1000.0f);
    if (elapsed_ms < 0) {
        elapsed_ms = 0;
    }
    screen_shake_update(&game->screenShake, elapsed_ms);
    if (game->playerInvulnerableMs > 0) {
        game->playerInvulnerableMs -= elapsed_ms;
        if (game->playerInvulnerableMs < 0) {
            game->playerInvulnerableMs = 0;
        }
    }

    if (game->gameOver) {
        game_update_player_animation(game, elapsed_ms);
        if (input != 0 && input->actionPressed) {
            int screen_width = game->screenWidth;
            int screen_height = game->screenHeight;

            game_init(game);
            game_set_screen_size(game, (float)screen_width, (float)screen_height);
        }

        return;
    }

    game->worldScrollX += game->worldSpeed * dt;
    while (game->worldScrollX >= WORLD_SCROLL_WRAP) {
        game->worldScrollX -= WORLD_SCROLL_WRAP;
    }

    /* Screen-space Y grows downward, matching the framebuffer. Negative Y velocity jumps upward. */
    game->playerVelocityX = 0.0f;

    if (input != 0 && input->left) {
        game->playerVelocityX = -player_config_get()->moveSpeed;
    }

    if (input != 0 && input->right) {
        game->playerVelocityX = player_config_get()->moveSpeed;
    }

    if (input != 0 && input->actionPressed) {
        if (game->playerGrounded) {
            game->playerVelocityY = player_config_get()->jumpVelocity;
            game->playerGrounded = 0;
            game->playerSmashing = 0;
            game->playerCanSmash = 1;
            audio_play_sound("jump");
            #if LITTLE_ONE_DEBUG_SMASH
            LOGI("Jump start");
            #endif
        } else if (game->playerCanSmash) {
            game->playerVelocityY = player_config_get()->smashVelocity;
            game->playerSmashing = 1;
            game->playerCanSmash = 0;
            audio_play_sound("smash");
            #if LITTLE_ONE_DEBUG_SMASH
            LOGI("Smash start");
            #endif
        }
    }

    game->playerVelocityY += GRAVITY * dt;

    game->playerX += game->playerVelocityX * dt;
    game->playerY += game->playerVelocityY * dt;

    player_was_smashing = game->playerSmashing;

    game_clamp_player_x(game);
    game_clamp_player_to_ground(game);

    game_update_spawn_timer(game, dt);
    game_update_entities(game, dt);
    game_handle_collisions(game, player_was_smashing);
    game_update_player_animation(game, elapsed_ms);
}
