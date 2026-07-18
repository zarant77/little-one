#include "game.h"

#include <android/log.h>
#include <stdio.h>

#include "../audio/audio.h"
#include "../config.h"
#include "../config/threat_config.h"
#include "../config/foreground_decoration_config.h"
#include "../config/player_config.h"
#include "../feedback/game_feedback.h"
#include "../sprites/animations/animation_evaluate.h"
#include "game_effects.h"
#include "game_settings.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LITTLE_ONE_LOG_TAG, __VA_ARGS__)

static const float GRAVITY = 3000.0f;
static const double WORLD_SCROLL_WRAP = 80539200.0;
static const float SPAWN_MIN_SECONDS = 1.5f;
static const float SPAWN_SECONDS_RANGE = 1.0f;
static const float SPAWN_RIGHT_PADDING = 16.0f;
static const int PLAYER_HIT_INVULNERABLE_MS = 900;
static const int GAME_OVER_MIN_VISIBLE_MS = 1000;
static const int SMASH_HITSTOP_MS = 70;
static const int FLOATING_TEXT_LIFETIME_MS = 1300;
static const float FLOATING_TEXT_RISE_SPEED = -125.0f;
static const uint32_t FLOATING_TEXT_SCORE_COLOR = 0x64ff7dff;
static const uint32_t FLOATING_TEXT_DAMAGE_COLOR = 0xff4f5cff;
static unsigned int game_random_state = 12345;

static void game_set_music(const char* music_id) {
    audio_play_music(music_id);

    #if LITTLE_ONE_DEBUG_GAME_STATE
    LOGI("Game music requested: %s", music_id);
    #endif
}

static const PlayerConfig* game_active_player_config(const GameState* game) {
    (void)game;
    return player_config_get();
}

static float game_player_width(const GameState* game) {
    return (float)game_active_player_config(game)->visual.width;
}

static float game_player_height(const GameState* game) {
    return (float)game_active_player_config(game)->visual.height;
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

    return ENTITY_ANIM_WALK;
}

static void game_update_player_animation(GameState* game, int32_t elapsed_ms) {
    EntityAnimSlot slot;
    int32_t animation_elapsed_ms = elapsed_ms;

    if (game->gameOver) {
        entity_animation_update(&game->playerAnimation, elapsed_ms);
        return;
    }

    slot = game_player_animation_slot(game);

    if (slot == ENTITY_ANIM_WALK) {
        if (game->playerVelocityX < 0.0f) {
            animation_elapsed_ms = 0;
        } else if (game->playerVelocityX > 0.0f) {
            animation_elapsed_ms *= 2;
        }
    }

    entity_animation_set(
            &game->playerAnimation,
            entity_animation_player_config(),
            slot
    );
    entity_animation_update(&game->playerAnimation, animation_elapsed_ms);
}

static void game_clamp_player_x(GameState* game) {
    float max_x = (float)game->screenWidth - game_player_width(game);

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
    float player_bottom = game->playerY + game_player_height(game);
    int was_grounded = game->playerGrounded;
    int was_smashing = game->playerSmashing;

    if (game->playerY < 0.0f) {
        game->playerY = 0.0f;
        if (game->playerVelocityY < 0.0f) {
            game->playerVelocityY = 0.0f;
        }
    }

    if (player_bottom >= ground_y) {
        game->playerY = ground_y - game_player_height(game);
        if (game->playerY < 0.0f) {
            game->playerY = 0.0f;
        }
        game->playerVelocityY = 0.0f;
        game->playerGrounded = 1;
        game->playerSmashing = 0;
        game->playerCanSmash = 0;
        if (!was_grounded) {
            if (was_smashing) {
                int impact_x = (int)game->playerX + (int)game_player_width(game) / 2;
                int impact_y = (int)ground_y;
                uint32_t effect_seed = game_random_state
                        ^ (uint32_t)impact_x
                        ^ ((uint32_t)impact_y << 16)
                        ^ (uint32_t)game->runTimeMs;

                audio_play_sound("smash");
                game_feedback_smash_land(&game->screenShake);
                game_effects_spawn_smash_impact(impact_x, impact_y, effect_seed);
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

static void game_clear_foreground_decorations(GameState* game) {
    for (int decoration_index = 0;
            decoration_index < FOREGROUND_MAX_INSTANCES;
            ++decoration_index) {
        ForegroundDecoration* decoration = game->foregroundDecorations + decoration_index;

        decoration->active = 0;
        decoration->x = 0.0f;
        decoration->y = 0.0f;
        decoration->scale = 1.0f;
        decoration->alpha = 1.0f;
        decoration->sprite = 0;
        decoration->width = 0;
        decoration->height = 0;
    }
}

static void game_clear_floating_texts(GameState* game)
{
    for (int text_index = 0; text_index < MAX_FLOATING_TEXTS; ++text_index)
    {
        FloatingText* text = game->floatingTexts + text_index;

        text->active = 0;
        text->x = 0.0f;
        text->y = 0.0f;
        text->velocity_y = 0.0f;
        text->age_ms = 0;
        text->lifetime_ms = 0;
        text->color = 0xffffffff;
        text->text[0] = 0;
    }
}

static void game_spawn_floating_text(
        GameState* game,
        float x,
        float y,
        uint32_t color,
        const char* text_value)
{
    FloatingText* target = 0;

    if (game == 0 || text_value == 0)
    {
        return;
    }

    for (int text_index = 0; text_index < MAX_FLOATING_TEXTS; ++text_index)
    {
        FloatingText* text = game->floatingTexts + text_index;

        if (!text->active)
        {
            target = text;
            break;
        }

        if (target == 0 || text->age_ms > target->age_ms)
        {
            target = text;
        }
    }

    if (target == 0)
    {
        return;
    }

    target->active = 1;
    target->x = x;
    target->y = y;
    target->velocity_y = FLOATING_TEXT_RISE_SPEED;
    target->age_ms = 0;
    target->lifetime_ms = FLOATING_TEXT_LIFETIME_MS;
    target->color = color;
    snprintf(target->text, sizeof(target->text), "%s", text_value);
}

static void game_update_floating_texts(GameState* game, int elapsed_ms)
{
    float dt = (float)elapsed_ms / 1000.0f;

    if (game == 0)
    {
        return;
    }

    for (int text_index = 0; text_index < MAX_FLOATING_TEXTS; ++text_index)
    {
        FloatingText* text = game->floatingTexts + text_index;

        if (!text->active)
        {
            continue;
        }

        text->age_ms += elapsed_ms;
        if (text->age_ms >= text->lifetime_ms)
        {
            text->active = 0;
            continue;
        }

        text->y += text->velocity_y * dt;
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

static void game_shift_foreground_decorations_y(GameState* game, float y_delta) {
    for (int decoration_index = 0;
            decoration_index < FOREGROUND_MAX_INSTANCES;
            ++decoration_index) {
        ForegroundDecoration* decoration = game->foregroundDecorations + decoration_index;

        if (decoration->active) {
            decoration->y += y_delta;
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

    {
        int threat_count = 0;
        const ThreatConfig* config;
        threat_config_get_all(&threat_count);
        config = threat_config_get(game_random_index(threat_count));
        if (config == 0) return;
        y_offset = config->spawnYmin
                + game_random_01() * (config->spawnYmax - config->spawnYmin);
        y = game_ground_y(game) + y_offset - (float)config->visual.height;
        entity_spawn(entity, config, x, y);
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
        if (!entity->active) {
            continue;
        }

        if (entity->dead) {
            active_count += 1;
            continue;
        }

        if (entity->x + (float)entity_get_width(entity) < 0.0f) {
            entity_clear(entity);
        } else {
            active_count += 1;
        }
    }

    game->activeEntityCount = active_count;
}

static void game_update_dead_entities(GameState* game, float dt) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        Entity* entity = game->entities + entity_index;

        if (entity->active && entity->dead) {
            entity_update(entity, 0.0f, dt);
        }
    }
}

static ForegroundDecoration* game_find_free_foreground_decoration(GameState* game) {
    for (int decoration_index = 0;
            decoration_index < FOREGROUND_MAX_INSTANCES;
            ++decoration_index) {
        if (!game->foregroundDecorations[decoration_index].active) {
            return game->foregroundDecorations + decoration_index;
        }
    }

    return 0;
}

static float game_foreground_config_gap(const ForegroundDecorationConfig* config) {
    float min_gap = FOREGROUND_DEFAULT_MIN_GAP;
    float max_gap = FOREGROUND_DEFAULT_MAX_GAP;

    if (config != 0) {
        if (config->min_gap >= 0.0f) {
            min_gap = config->min_gap;
        }
        if (config->max_gap >= min_gap) {
            max_gap = config->max_gap;
        } else {
            max_gap = min_gap;
        }
    }

    return min_gap + game_random_01() * (max_gap - min_gap);
}

static float game_foreground_config_weight(const ForegroundDecorationConfig* config) {
    if (config == 0 || config->spawn_weight < 0.0f) {
        return 0.0f;
    }

    return config->spawn_weight > 0.0f ? config->spawn_weight : 1.0f;
}

static const GeneratedSprite* game_foreground_config_sprite(
        const ForegroundDecorationConfig* config
) {
    const GeneratedSprite* sprite;

    if (config == 0
            || config->sprite_id == 0
            || config->width <= 0
            || config->height <= 0
            || config->scale_min <= 0.0f
            || config->scale_max < config->scale_min
            || game_foreground_config_weight(config) <= 0.0f) {
        return 0;
    }

    sprite = generated_sprite_get_by_id(config->sprite_id);
    if (sprite == 0 || sprite->pixels == 0) {
        return 0;
    }

    return sprite;
}

static const ForegroundDecorationConfig* game_pick_foreground_config(
        const GeneratedSprite** out_sprite
) {
    const ForegroundDecorationConfig* configs;
    const ForegroundDecorationConfig* last_valid_config = 0;
    const GeneratedSprite* last_valid_sprite = 0;
    float total_weight = 0.0f;
    float selected_weight;
    int config_count = 0;

    if (out_sprite != 0) {
        *out_sprite = 0;
    }

    configs = foreground_decoration_config_get_all(&config_count);
    for (int config_index = 0; config_index < config_count; ++config_index) {
        const ForegroundDecorationConfig* config = configs + config_index;

        if (game_foreground_config_sprite(config) == 0) {
            continue;
        }

        total_weight += game_foreground_config_weight(config);
    }

    if (total_weight <= 0.0f) {
        return 0;
    }

    selected_weight = game_random_01() * total_weight;
    for (int config_index = 0; config_index < config_count; ++config_index) {
        const ForegroundDecorationConfig* config = configs + config_index;
        const GeneratedSprite* sprite = game_foreground_config_sprite(config);

        if (sprite == 0) {
            continue;
        }

        last_valid_config = config;
        last_valid_sprite = sprite;
        selected_weight -= game_foreground_config_weight(config);
        if (selected_weight <= 0.0f) {
            if (out_sprite != 0) {
                *out_sprite = sprite;
            }
            return config;
        }
    }

    if (out_sprite != 0) {
        *out_sprite = last_valid_sprite;
    }
    return last_valid_config;
}

static const ForegroundDecorationConfig* game_spawn_foreground_decoration(GameState* game) {
    ForegroundDecoration* decoration = game_find_free_foreground_decoration(game);
    const ForegroundDecorationConfig* config;
    const GeneratedSprite* sprite;
    float scale;
    float y_min;
    float y_max;
    float y_offset;

    if (decoration == 0 || game->screenWidth <= 0 || game->screenHeight <= 0) {
        return 0;
    }

    config = game_pick_foreground_config(&sprite);
    if (config == 0 || sprite == 0) {
        return 0;
    }

    scale = config->scale_min + game_random_01() * (config->scale_max - config->scale_min);
    if (scale <= 0.0f) {
        return 0;
    }

    y_min = config->spawn_y_min;
    y_max = config->spawn_y_max >= y_min ? config->spawn_y_max : y_min;
    y_offset = y_min + game_random_01() * (y_max - y_min);

    decoration->active = 1;
    decoration->x = (float)game->screenWidth + FOREGROUND_SPAWN_RIGHT_PADDING;
    decoration->scale = scale;
    decoration->alpha = 1.0f;
    decoration->sprite = sprite;
    decoration->width = (int)((float)config->width * scale);
    decoration->height = (int)((float)config->height * scale);
    if (decoration->width < 1) {
        decoration->width = 1;
    }
    if (decoration->height < 1) {
        decoration->height = 1;
    }
    decoration->y = game_ground_y(game)
            + y_offset
            + config->draw_offset_y
            - (float)decoration->height;

    return config;
}

#if FOREGROUND_FADE_ENABLED
static float game_foreground_decoration_alpha(
        const GameState* game,
        const ForegroundDecoration* decoration
) {
    float min_alpha = FOREGROUND_FADE_MIN_ALPHA;
    float player_left = game->playerX;
    float player_top = game->playerY;
    float player_right = player_left + game_player_width(game);
    float player_bottom = player_top + game_player_height(game);
    float decoration_left = decoration->x;
    float decoration_top = decoration->y;
    float decoration_right = decoration_left + (float)decoration->width;
    float decoration_bottom = decoration_top + (float)decoration->height;
    float distance_x = 0.0f;
    float distance_y = 0.0f;
    float distance;
    float t;

    if (min_alpha < 0.0f) {
        min_alpha = 0.0f;
    } else if (min_alpha > 1.0f) {
        min_alpha = 1.0f;
    }

    if (decoration_right < player_left) {
        distance_x = player_left - decoration_right;
    } else if (player_right < decoration_left) {
        distance_x = decoration_left - player_right;
    }

    if (decoration_bottom < player_top) {
        distance_y = player_top - decoration_bottom;
    } else if (player_bottom < decoration_top) {
        distance_y = decoration_top - player_bottom;
    }

    distance = distance_x > distance_y ? distance_x : distance_y;
    if (distance <= 0.0f) {
        return min_alpha;
    }

    #if FOREGROUND_FADE_USE_SMOOTH_DISTANCE
    if (FOREGROUND_FADE_DISTANCE <= 0.0f) {
        return 1.0f;
    }

    if (distance >= FOREGROUND_FADE_DISTANCE) {
        return 1.0f;
    }

    t = distance / FOREGROUND_FADE_DISTANCE;
    return min_alpha + (1.0f - min_alpha) * t;
    #else
    (void)t;
    return 1.0f;
    #endif
}
#endif

static void game_update_foreground_decorations(GameState* game, float dt) {
    float scroll_distance = game->worldSpeed * FOREGROUND_SCROLL_MULTIPLIER * dt;

    for (int decoration_index = 0;
            decoration_index < FOREGROUND_MAX_INSTANCES;
            ++decoration_index) {
        ForegroundDecoration* decoration = game->foregroundDecorations + decoration_index;

        if (!decoration->active) {
            continue;
        }

        decoration->x -= scroll_distance;
        if (decoration->x + (float)decoration->width < 0.0f) {
            decoration->active = 0;
            decoration->sprite = 0;
            decoration->alpha = 1.0f;
            continue;
        }

        #if FOREGROUND_FADE_ENABLED
        decoration->alpha = game_foreground_decoration_alpha(game, decoration);
        #else
        decoration->alpha = 1.0f;
        #endif
    }

    game->foregroundSpawnGap -= scroll_distance;
    if (game->foregroundSpawnGap <= 0.0f) {
        const ForegroundDecorationConfig* spawned_config =
                game_spawn_foreground_decoration(game);

        game->foregroundSpawnGap = game_foreground_config_gap(spawned_config);
    }
}

static int game_player_overlaps_entity_hurt_zone(const GameState* game, const Entity* entity) {
    return hurt_zones_overlap(
            (int32_t)game->playerX,
            (int32_t)game->playerY,
            game_active_player_config(game)->visual.width,
            game_active_player_config(game)->visual.height,
            &game_active_player_config(game)->hurt_zone,
            (int32_t)entity->x,
            (int32_t)entity->y,
            entity_get_width(entity),
            entity_get_height(entity),
            entity_get_hurt_zone(entity)
    );
}

static int game_player_boundary_overlaps_enemy_hurt_zone(const GameState* game, const Entity* enemy) {
    if (enemy == 0 || enemy->config == 0
            || enemy->config->type == THREAT_STATIC_OBSTACLE) {
        return 0;
    }

    return rect_overlaps_hurt_zone(
            (int32_t)game->playerX,
            (int32_t)game->playerY,
            &game_active_player_config(game)->boundary,
            (int32_t)enemy->x,
            (int32_t)enemy->y,
            entity_get_width(enemy),
            entity_get_height(enemy),
            entity_get_hurt_zone(enemy)
    );
}

static const char* game_enemy_death_sound_id(const Entity* entity) {
    const char* id;

    if (entity == 0 || entity->config == 0 || entity->config->id == 0) {
        return "death";
    }

    id = entity->config->id;
    if (id[0] == 't' && id[1] == 'h' && id[2] == 'r' && id[3] == 'e' && id[4] == 'a' && id[5] == 't' && id[6] == '.') {
        id += 7;

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

static void game_award_score(GameState* game, int score)
{
    if (game == 0 || score <= 0)
    {
        return;
    }

    game->score += score;
    progression_apply_score(&game->progress, score);
    game->progressDirty = 1;
}

static void game_kill_enemy_by_smash(GameState* game, Entity* entity) {
    if (entity->config != 0) {
        char score_text[12];
        int score_value = entity->config->scoreValue;

        snprintf(score_text, sizeof(score_text), "+%d", score_value);
        game_spawn_floating_text(
                game,
                entity->x + (float)entity_get_width(entity) * 0.5f,
                entity->y + (float)entity_get_height(entity) * 0.25f,
                FLOATING_TEXT_SCORE_COLOR,
                score_text
        );
        game_award_score(game, entity->config->scoreValue);
    }
    audio_play_sound(game_enemy_death_sound_id(entity));
    entity_kill(entity);
    if (game->hitstopMs < SMASH_HITSTOP_MS) {
        game->hitstopMs = SMASH_HITSTOP_MS;
    }
    #if LITTLE_ONE_DEBUG_SMASH
    LOGI("Enemy killed by smash");
    #endif
}

static void game_enter_game_over(GameState* game) {
    if (game->gameOver) {
        return;
    }

    game->gameOver = 1;
    game->gameOverElapsedMs = 0;
    game->gameOverInputArmed = 0;
    game->playerVelocityX = 0.0f;
    game->playerVelocityY = 0.0f;
    game->playerSmashing = 0;
    game->playerCanSmash = 0;
    entity_animation_set(
            &game->playerAnimation,
            entity_animation_player_config(),
            ENTITY_ANIM_DEATH
    );
    if (game_active_player_config(game)->visual.deathAnimationId != 0) {
        const AnimationClip* death_clip =
                animation_find_clip(game_active_player_config(game)->visual.deathAnimationId);

        if (death_clip != 0) {
            game->playerAnimation.clip = death_clip;
        }
    }
    audio_play_sound("death");
    game_feedback_player_death(&game->screenShake);
    game->newRecord = game->score > game->progress.best_score;
    progression_apply_run(&game->progress, game->score);
    game->bestScore = game->progress.best_score;
    game->progressDirty = 1;
    game_set_music("game_over");

    #if LITTLE_ONE_DEBUG_GAME_STATE
    LOGI("Game state switched to game over");
    #endif
}

static int game_damage_player(GameState* game) {
    if (game->playerInvulnerableMs > 0 || game->playerHp <= 0) {
        return 0;
    }

    game->playerHp -= 1;
    game_spawn_floating_text(
            game,
            game->playerX + (float)game_active_player_config(game)->visual.width * 0.5f,
            game->playerY - 20.0f,
            FLOATING_TEXT_DAMAGE_COLOR,
            "-1HP"
    );
    if (game->playerHp <= 0) {
        game_enter_game_over(game);
        return 1;
    }

    game->playerInvulnerableMs = PLAYER_HIT_INVULNERABLE_MS;
    entity_animation_set(
            &game->playerAnimation,
            entity_animation_player_config(),
            ENTITY_ANIM_DAMAGE
    );
    audio_play_sound("damage");
    return 1;
}

static void game_handle_collisions(GameState* game, int player_was_smashing) {
    for (int entity_index = 0; entity_index < MAX_ENTITIES; ++entity_index) {
        Entity* entity = game->entities + entity_index;

        if (!entity->active || entity->dead) {
            continue;
        }

        if (entity->config != 0
                && entity->config->type != THREAT_STATIC_OBSTACLE
                && player_was_smashing
                && game_player_boundary_overlaps_enemy_hurt_zone(game, entity)) {
            game_kill_enemy_by_smash(game, entity);
            if (game->uiState != GAME_UI_PLAYING) {
                return;
            }
            continue;
        }

        if (!game_player_overlaps_entity_hurt_zone(game, entity)) {
            continue;
        }

        if (game_damage_player(game)) {
            entity_attack(entity);
            return;
        }
    }
}

void game_init(GameState* game) {
    int best_score;
    GameSettings settings;
    int settings_initialized;
    int settings_dirty;
    ProgressionState progress;
    int progress_initialized;
    int progress_dirty;

    if (game == 0) {
        return;
    }

    best_score = game->bestScore;
    settings = game->settings;
    settings_initialized = game->settingsInitialized;
    settings_dirty = game->settingsDirty;
    progress = game->progress;
    progress_initialized = game->progressInitialized;
    progress_dirty = game->progressDirty;
    if (!progress_initialized) {
        progression_init(&progress);
        progress_initialized = 1;
    }
    if (best_score > progress.best_score) {
        progress.best_score = best_score;
        progress_dirty = 1;
    }

    game->playerX = 0.0f;
    game->playerY = 0.0f;
    game->playerVelocityX = 0.0f;
    game->playerVelocityY = 0.0f;
    game->playerGrounded = 0;
    game->playerSmashing = 0;
    game->playerCanSmash = 0;
    game->playerHp = player_config_get()->hp;
    game->playerInvulnerableMs = 0;
    game->hitstopMs = 0;
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
    game->worldScrollX = 0.0;
    game->worldSpeed = LITTLE_ONE_WORLD_SCROLL_SPEED;
    game_clear_entities(game);
    game->spawnTimer = game_next_spawn_time();
    game_clear_foreground_decorations(game);
    game_clear_floating_texts(game);
    game->foregroundSpawnGap = FOREGROUND_INITIAL_SPAWN_GAP_MIN
            + game_random_01()
            * (FOREGROUND_INITIAL_SPAWN_GAP_MAX - FOREGROUND_INITIAL_SPAWN_GAP_MIN);
    game->gameOver = 0;
    game->gameOverElapsedMs = 0;
    game->gameOverInputArmed = 0;
    game->score = 0;
    game->bestScore = progress.best_score;
    game->newRecord = 0;
    game->runTimeMs = 0;
    game->fps = 0;
    game->averageFrameMs = 0;
    game->activeEntityCount = 0;
    game->exitRequested = 0;
    game->runStarted = 0;
    screen_shake_start(&game->screenShake, 0, 0, 1u);
    game_effects_init();
    game->uiState = GAME_UI_MENU;
    if (!settings_initialized) {
        game_settings_init(&settings);
        settings_initialized = 1;
    }
    game->settings = settings;
    game->settingsInitialized = settings_initialized;
    game->settingsDirty = settings_dirty;
    game->progress = progress;
    game->progressInitialized = progress_initialized;
    game->progressDirty = progress_dirty;
    audio_set_music_volume(game->settings.music_volume);
    audio_set_sfx_volume(game->settings.sfx_volume);
    game_set_music("game_loop");
}

void game_restart_run(GameState* game) {
    int screen_width;
    int screen_height;

    if (game == 0) {
        return;
    }

    screen_width = game->screenWidth;
    screen_height = game->screenHeight;
    game_init(game);
    game_set_screen_size(game, (float)screen_width, (float)screen_height);
    game->runStarted = 1;
    game->uiState = GAME_UI_PLAYING;
    game_set_music("game_loop");

    #if LITTLE_ONE_DEBUG_GAME_STATE
    LOGI("Game run restarted");
    #endif
}

int game_try_restart_after_game_over(GameState* game) {
    if (game == 0
            || !game->gameOver
            || game->gameOverElapsedMs < GAME_OVER_MIN_VISIBLE_MS
            || !game->gameOverInputArmed) {
        return 0;
    }

    game_restart_run(game);
    return 1;
}

int game_start_run(GameState* game) {
    if (game == 0) {
        return 0;
    }

    game_restart_run(game);
    return 1;
}

void game_show_menu(GameState* game) {
    if (game == 0) {
        return;
    }

    if (game->gameOver) {
        game->runStarted = 0;
    }

    game->uiState = GAME_UI_MENU;
    game->gameOver = 0;
    game->gameOverElapsedMs = 0;
    game->gameOverInputArmed = 0;
    game_set_music("game_loop");
}

const PlayerConfig* game_player_config(const GameState* game) {
    return game_active_player_config(game);
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
        player_ground_offset = old_ground_y - (game->playerY + game_player_height(game));
        if (was_grounded || player_ground_offset < 0.0f) {
            player_ground_offset = 0.0f;
        }
    }

    game->screenWidth = (int)width;
    game->screenHeight = (int)height;
    new_ground_y = game_ground_y(game);

    if (old_screen_width <= 0 || old_screen_height <= 0) {
        game->playerX = 0.0f;
        game->playerY = new_ground_y - game_player_height(game);
        game->playerVelocityX = 0.0f;
        game->playerVelocityY = 0.0f;
        game->playerGrounded = 1;
        game->playerSmashing = 0;
        game->playerCanSmash = 0;
    } else if (old_screen_width != game->screenWidth || old_screen_height != game->screenHeight) {
        game->playerY = new_ground_y - game_player_height(game) - player_ground_offset;
        game_shift_entities_y(game, new_ground_y - old_ground_y);
        game_shift_foreground_decorations_y(game, new_ground_y - old_ground_y);
        if (was_grounded) {
            game->playerVelocityY = 0.0f;
            game->playerGrounded = 1;
            game->playerSmashing = 0;
            game->playerCanSmash = 0;
        } else if (game->playerY + game_player_height(game) >= new_ground_y) {
            game->playerVelocityY = 0.0f;
        }
    }

    if (!game->gameOver) {
        game_clamp_player_x(game);
        game_clamp_player_to_ground(game);
    }
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

    if (game->uiState != GAME_UI_PLAYING) {
        return;
    }

    screen_shake_update(&game->screenShake, elapsed_ms);
    game_effects_update(elapsed_ms);
    game_update_floating_texts(game, elapsed_ms);
    if (game->playerInvulnerableMs > 0) {
        game->playerInvulnerableMs -= elapsed_ms;
        if (game->playerInvulnerableMs < 0) {
            game->playerInvulnerableMs = 0;
        }
    }

    if (game->gameOver) {
        game->gameOverElapsedMs += elapsed_ms;
        game_update_player_animation(game, elapsed_ms);
        game_update_dead_entities(game, dt);

        if (input == 0 || !input->actionHeld) {
            game->gameOverInputArmed = 1;
        }

        if (game->gameOverElapsedMs >= GAME_OVER_MIN_VISIBLE_MS
                && game->gameOverInputArmed
                && input != 0
                && input->actionPressed) {
            game_try_restart_after_game_over(game);
        }

        return;
    }

    if (game->hitstopMs > 0) {
        game->hitstopMs -= elapsed_ms;
        if (game->hitstopMs < 0) {
            game->hitstopMs = 0;
        }
        game->playerVelocityX = 0.0f;
        return;
    }

    game->runTimeMs += elapsed_ms;

    game->worldScrollX += game->worldSpeed * dt;
    while (game->worldScrollX >= WORLD_SCROLL_WRAP) {
        game->worldScrollX -= WORLD_SCROLL_WRAP;
    }

    /* Screen-space Y grows downward, matching the framebuffer. Negative Y velocity jumps upward. */
    game->playerVelocityX = 0.0f;

    if (input != 0 && input->left) {
        game->playerVelocityX = -game_active_player_config(game)->moveSpeed;
    }

    if (input != 0 && input->right) {
        game->playerVelocityX = game_active_player_config(game)->moveSpeed;
    }

    if (input != 0 && input->actionPressed) {
        if (game->playerGrounded) {
            game->playerVelocityY = game_active_player_config(game)->jumpVelocity;
            game->playerGrounded = 0;
            game->playerSmashing = 0;
            game->playerCanSmash = 1;
            audio_play_sound("jump");
            #if LITTLE_ONE_DEBUG_SMASH
            LOGI("Jump start");
            #endif
        } else if (game->playerCanSmash) {
            game->playerVelocityY = game_active_player_config(game)->smashVelocity;
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
    game_update_foreground_decorations(game, dt);
}
