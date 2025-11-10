#include <stdio.h>

#include "../include/lexer.h"

int main(const int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <file.html>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE* file = fopen(filename, "r");

    if (!file) {
        printf("Error opening file\n");
        return 1;
    }

    printf("File opened successfully\n");

    get_token(file);

    fclose(file);

    return 0;
}