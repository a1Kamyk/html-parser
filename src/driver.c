#include "driver.h"

#include <sys/stat.h>
#include <stdio.h>

#include "preprocessor.h"
#include "tokenizer.h"

void run_parser(const int argc, char **argv) {
    // Check for command line arguments
    if (argc < 2) {
        printf("Usage: %s <file.html>\n", argv[0]);
        return;
    }

    // Check for temp folder
    const char* dir = "temp";
    struct stat s = {0};
    if (stat(dir, &s) == -1) {
        if (mkdir(dir) == -1) {
            printf("Count not create directory: %s\n", dir);
            return;
        }
    }

    // Normalize newlines
    const char *filename = argv[1];
    if (normalize_newlines(filename) != 0) {
        return;
    }

    // Start tokenizer in default state
    tokenizer_t tokenizer = {
        .insertion_state = INITIAL,
        .data_state = DATA_STATE,
        .return_state = DATA_STATE,
        .consume_flag = true,

        .internal_token_queue = {}
    };
    queue_init(&tokenizer.internal_token_queue);
}