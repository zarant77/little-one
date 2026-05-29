cmake_minimum_required(VERSION 3.22.1)

project(littleone C)

add_library(littleone SHARED
    ../../../../src/entity.c
    ../../../../src/entity_config.c
    ../../../../src/game.c
    ../../../../src/input.c
    ../../../../src/main.c
    ../../../../src/platform_android.c
    ../../../../src/renderer.c
)

target_include_directories(littleone PRIVATE
    ../../../../src
)

target_link_libraries(littleone android log)
