#include "driver.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "preprocessor.h"
#include "tokenizer.h"
#include "tree_builder.h"

#define TEMPORARY_BUFFER_SIZE 32

dom_node_t* parse_document(tokenizer_t* tokenizer, tree_builder_t* builder) {
    dom_node_t* root = create_root_node();
    builder->root_node = root;
    builder->current_node = root;

    while (true) {
        while (queue_is_empty(tokenizer->token_queue)) {
            if(token_next(tokenizer) != 0) {
                goto fail;
            }
        }

        while (!queue_is_empty(tokenizer->token_queue)) {
            if (builder->current_token.type == END_OF_FILE)
                goto done;
            if (tree_node_next(builder) != 0) {
                goto fail;
            }
        }
    }

    done:
    return root;

    fail:
    delete_tree_node(root);
    free(root);
    return NULL;
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

    // Initialize the token queue
    token_queue_t token_queue = {};
    queue_init(&token_queue);

    // Start parser in default state
    tokenizer_t tokenizer;
    tree_builder_t tree_builder;
    parser_init(&tokenizer, &tree_builder, stream, &token_queue);

    dom_node_t* dom_root = parse_document(&tokenizer, &tree_builder);

    return 0;
}

void parser_init(tokenizer_t* tokenizer, tree_builder_t* tree_builder,
                FILE* stream, token_queue_t* token_stream) {
    // Tokenizer initialization
    tokenizer->data_state = DATA_STATE;
    tokenizer->return_state = DATA_STATE;
    tokenizer->consume_flag = true;
    tokenizer->current_char = '\0';

    tokenizer->pending_token = (token_t){ .type = NONE };
    tokenizer->has_pending_token = false;
    tokenizer->temporary_buffer = (string_t){};
    tokenizer->last_error = NO_ERROR;

    tokenizer->token_queue = token_stream;
    tokenizer->stream = stream;
    tokenizer->chars_consumed = 0;

    parser_string_init_sized(&tokenizer->temporary_buffer, TEMPORARY_BUFFER_SIZE);

    // Tree builder initialization
    tree_builder->insertion_state = INITIAL;
    tree_builder->consume_flag = true;
    tree_builder->current_token = (token_t){ .type = NONE };
    tree_builder->parser_cannot_change_mode = false;

    tree_builder->pending_node = (dom_node_t){ .type = DOM_NONE };
    tree_builder->has_pending_node = false;

    tree_builder->open_elem_stack = (open_elem_stack_t){0};
    tree_builder->token_stream = token_stream;
    tree_builder->root_node = NULL;
    tree_builder->current_node = NULL;

    stack_init(&tree_builder->open_elem_stack);
}