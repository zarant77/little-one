cmake_minimum_required(VERSION 3.22.1)

project(littleone C)

add_library(littleone SHARED
    ../../../../src/audio/audio.c
    ../../../../src/audio/music_generator.c
    ../../../../src/audio/music_registry.c
    ../../../../src/audio/sound_generator.c
    ../../../../src/audio/sound_registry.c
    ../../../../src/config/background_config.c
    ../../../../src/config/enemy_config.c
    ../../../../src/config/obstacle_config.c
    ../../../../src/config/player_config.c
    ../../../../src/entity/entity.c
    ../../../../src/entity/entity_animation.c
    ../../../../src/feedback/game_feedback.c
    ../../../../src/feedback/screen_shake.c
    ../../../../src/game/game.c
    ../../../../src/input/input.c
    ../../../../src/main.c
    ../../../../src/platform/platform_android.c
    ../../../../src/renderer/renderer.c
    ../../../../src/sprites/animations/animation_evaluate.c
    ../../../../src/sprites/animations/animations.c
    ../../../../src/sprites/definitions/sprites_packed.c
    ../../../../src/sprites/generated_sprite.c
)

target_include_directories(littleone PRIVATE
    ../../../../src
)

target_link_libraries(littleone android log OpenSLES)
