cmake_minimum_required(VERSION 3.5..4.0)
project(html_parser C)

set(CMAKE_C_STANDARD 99)

add_executable(html_parser
        src/main.c
        src/tree_builder.c
        src/tokenizer.c
        src/preprocessor.c
        src/driver.c
        src/misc.c
)

target_include_directories(html_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)