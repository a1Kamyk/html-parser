cmake_minimum_required(VERSION 4.0)
project(html_parser C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(html_parser
        src/main.c
        src/tree.cW
        src/lexer.c
)
