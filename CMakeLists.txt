cmake_minimum_required(VERSION 4.0)
project(html_parser C)

set(CMAKE_C_STANDARD 11)

add_executable(html_parser
        src/main.c
        src/tree.c
        src/lexer.c
        src/preprocessor.c
)

target_include_directories(html_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)