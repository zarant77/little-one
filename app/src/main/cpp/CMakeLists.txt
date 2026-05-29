cmake_minimum_required(VERSION 3.22.1)

project(littleone C)

add_library(littleone SHARED
    ../../../../src/entity.c
    ../../../../src/enemy_config.c
    ../../../../src/game.c
    ../../../../src/input.c
    ../../../../src/main.c
    ../../../../src/obstacle_config.c
    ../../../../src/platform_android.c
    ../../../../src/player_config.c
    ../../../../src/renderer.c
)

target_include_directories(littleone PRIVATE
    ../../../../src
)

target_link_libraries(littleone android log)
