#ifndef HTML_PARSER_LEXER_H
#define HTML_PARSER_LEXER_H

#include <stdio.h>

enum {
    TAG_OPEN,
    TAG_CLOSE,
};

int get_token(FILE* file);

#endif //HTML_PARSER_LEXER_H