cmake_minimum_required(VERSION 3.22.1)

project(littleone C)

add_library(littleone SHARED
    ../../../../src/main.c
)

target_include_directories(littleone PRIVATE
    ../../../../src
)

target_link_libraries(littleone android log)
