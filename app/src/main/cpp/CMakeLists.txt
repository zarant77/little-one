cmake_minimum_required(VERSION 3.22.1)

project(littleone C)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(LITTLE_ONE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../..)
set(LITTLE_ONE_FONT_OUTPUTS
    ${LITTLE_ONE_ROOT}/src/fonts/fonts_packed.c
    ${LITTLE_ONE_ROOT}/src/fonts/fonts_packed.h
)
set(LITTLE_ONE_SPRITE_OUTPUTS
    ${LITTLE_ONE_ROOT}/src/sprites/definitions/sprites_packed.c
    ${LITTLE_ONE_ROOT}/src/sprites/definitions/sprites_packed.h
)

file(GLOB_RECURSE LITTLE_ONE_FONT_ASSETS
    ${LITTLE_ONE_ROOT}/src/assets/fonts/*.font.json
)
file(GLOB_RECURSE LITTLE_ONE_SPRITE_ASSETS
    ${LITTLE_ONE_ROOT}/src/assets/sprites/*.json
)

add_custom_command(
    OUTPUT ${LITTLE_ONE_FONT_OUTPUTS}
    COMMAND ${Python3_EXECUTABLE} ${LITTLE_ONE_ROOT}/tools/pack_fonts.py
    DEPENDS
        ${LITTLE_ONE_ROOT}/tools/pack_fonts.py
        ${LITTLE_ONE_FONT_ASSETS}
    WORKING_DIRECTORY ${LITTLE_ONE_ROOT}
    COMMENT "Packing vector fonts"
)

add_custom_target(pack_fonts DEPENDS ${LITTLE_ONE_FONT_OUTPUTS})

add_custom_command(
    OUTPUT ${LITTLE_ONE_SPRITE_OUTPUTS}
    COMMAND ${Python3_EXECUTABLE} ${LITTLE_ONE_ROOT}/tools/pack_sprites.py
    DEPENDS
        ${LITTLE_ONE_ROOT}/tools/pack_sprites.py
        ${LITTLE_ONE_SPRITE_ASSETS}
    WORKING_DIRECTORY ${LITTLE_ONE_ROOT}
    COMMENT "Packing vector sprites"
)

add_custom_target(pack_sprites DEPENDS ${LITTLE_ONE_SPRITE_OUTPUTS})

add_library(littleone SHARED
    ../../../../src/audio/audio.c
    ../../../../src/audio/generated_music.c
    ../../../../src/audio/generated_sound.c
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
    ../../../../src/font/font_renderer.c
    ../../../../src/fonts/fonts_packed.c
    ../../../../src/game/game.c
    ../../../../src/input/input.c
    ../../../../src/main.c
    ../../../../src/platform/platform_android.c
    ../../../../src/renderer/renderer.c
    ../../../../src/sprites/animations/animation_evaluate.c
    ../../../../src/sprites/animations/animations.c
    ../../../../src/sprites/definitions/sprites_packed.c
    ../../../../src/sprites/generated_sprite.c
    ../../../../src/ui/hud.c
)

add_dependencies(littleone pack_fonts pack_sprites)

target_include_directories(littleone PRIVATE
    ../../../../src
)

target_link_libraries(littleone android log OpenSLES)
