cmake_minimum_required(VERSION 3.22.1)

project(littleone C)

add_library(littleone SHARED
    ../../../../src/config/enemy_config.c
    ../../../../src/config/obstacle_config.c
    ../../../../src/config/player_config.c
    ../../../../src/entity/entity.c
    ../../../../src/game/game.c
    ../../../../src/input/input.c
    ../../../../src/main.c
    ../../../../src/platform/platform_android.c
    ../../../../src/renderer/renderer.c
    ../../../../src/sprites/definitions/boar.sprite.c
    ../../../../src/sprites/definitions/ork.sprite.c
    ../../../../src/sprites/definitions/player.sprite.c
    ../../../../src/sprites/definitions/rat.sprite.c
    ../../../../src/sprites/definitions/rock.sprite.c
    ../../../../src/sprites/definitions/stump.sprite.c
    ../../../../src/sprites/generated_sprite.c
    ../../../../src/sprites/sprite_registry.c
)

target_include_directories(littleone PRIVATE
    ../../../../src
)

target_link_libraries(littleone android log)
