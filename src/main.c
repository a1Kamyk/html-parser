#include <stdio.h>
#include <sys/stat.h>

#include "preprocessor.h"

int main(const int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <file.html>\n", argv[0]);
        return 1;
    }

    // Check for temp folder
    const char* dir = "temp";
    struct stat s = {0};
    if (stat(dir, &s) == -1) {
        if (mkdir(dir) == -1) {
            printf("Count not create directory: %s\n", dir);
            return 1;
        }
    }

    const char *filename = argv[1];

    if (normalize_newlines(filename) != 0) {
        return 1;
    }

    return 0;
}