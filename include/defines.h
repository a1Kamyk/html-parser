#ifndef HTML_PARSER_DEFINES_H
#define HTML_PARSER_DEFINES_H

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef enum {
    STDIO_ERROR = 1,
    MEMORY_ERROR,
    PREPROCESSOR_ERROR,
    LEXER_ERROR,
    PARSER_ERROR
} errors;

#endif //HTML_PARSER_DEFINES_H