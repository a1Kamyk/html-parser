#include "lexer.h"



int get_token(FILE* file) {
    const int c = fgetc(file);
    printf("%c", c);

    return 0;
}