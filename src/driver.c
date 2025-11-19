#include "driver.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "preprocessor.h"
#include "tokenizer.h"
#include "tree.h"

dom_tree_node_t* parse_document(tokenizer_t* tokenizer) {
    dom_tree_node_t* root = create_root_node();

    if (token_next(tokenizer) != 0) {
        // TODO cleanup
        return NULL;
    }

    return root;
}

int run_parser(const int argc, char **argv) {
    // Check for command line arguments
    if (argc < 2) {
        printf("Usage: %s <file.html>\n", argv[0]);
        return 1;
    }

    // Check for temp folder
    const char* temp_dir = "temp";
    struct stat s = {0};
    if (stat(temp_dir, &s) == -1) {
        if (mkdir(temp_dir) == -1) {
            printf("Count not create directory: %s\n", temp_dir);
            return 1;
        }
    }

    // Create temporary filepath
    const char *filepath= argv[1];
    char* temp_filepath = malloc(strlen(filepath) + strlen("/") + strlen(temp_dir) + 1);
    if (!temp_filepath) return 1;
    strcpy(temp_filepath, temp_dir);
    strcat(temp_filepath, "/");
    strcat(temp_filepath, filepath);
    // Remove old temporary file if exists
    const int res = remove(temp_filepath);
    if (res != 0 && errno != ENOENT) {
        free(temp_filepath);
        return 1;
    }

    // Normalize newlines
    if (normalize_newlines(filepath, temp_filepath) != 0) {
        free(temp_filepath);
        return 1;
    }

    FILE* stream = fopen(temp_filepath, "r");
    if (!stream) {
        free(temp_filepath);
        return 1;
    }

    // Start tokenizer in default state
    tokenizer_t tokenizer = {
        .insertion_state = INITIAL,
        .data_state = DATA_STATE,
        .return_state = DATA_STATE,
        .consume_flag = true,
        .current_char = '\0',

        .pending_token = { .type = NONE },
        .has_pending_token = false,

        .internal_token_queue = {},
        .stream = stream
    };
    queue_init(&tokenizer.internal_token_queue);

    dom_tree_node_t* dom_root = parse_document(&tokenizer);

    return 0;
}