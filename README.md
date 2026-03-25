# HTML Parser in C

- Implemented according to the [whatwg spec](https://html.spec.whatwg.org/multipage/parsing.html)
- Written in standard C99

## Building
```aiignore
 mkdir build && cd build
 cmake ..
 cmake --build .
```

## Requirements 
- Any C compiler
- CMake 3.5 or newer

## Goals
- Generate a DOM tree from HTML source code
- Move from a standalone binary to static / dynamic library