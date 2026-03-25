#include "driver.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "preprocessor.h"
#include "tokenizer.h"
#include "tree_builder.h"

#define TEMPORARY_BUFFER_SIZE 32

dom_node_t* parse_document(parser_t* parser) {
    dom_node_t* root = create_root_node();
    parser->builder.root_node = root;

    while (true) {
        while (queue_is_empty(&parser->token_queue)) {
            if(token_next(&parser->tokenizer) != 0) {
                goto fail;
            }
        }

        while (!queue_is_empty(&parser->token_queue)) {
            if (parser->builder.current_token.type == END_OF_FILE)
                goto done;
            if (tree_node_next(&parser->builder) != 0) {
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
    const size_t temp_filepath_len = strlen(temp_dir) + strlen("/") + strlen(filepath) + 1;
    char* temp_filepath = calloc(temp_filepath_len, sizeof(char));
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

    // Start parser in default state
    parser_t parser;
    parser_init(&parser, stream);

    dom_node_t* dom_root = parse_document(&parser);

    printf("%p\n", dom_root);

    return 0;
}

void parser_init(parser_t* parser, FILE* stream) {
    if (!parser || !stream)
        return;
    *parser = (parser_t){0};
    queue_init(&parser->token_queue);
    stack_init(&parser->open_elem_stack);

    tokenizer_t* tokenizer = &parser->tokenizer;
    tree_builder_t* builder = &parser->builder;

    // Tokenizer initialization
    tokenizer->data_state = DATA_STATE;
    tokenizer->return_state = DATA_STATE;
    tokenizer->consume_flag = true;
    tokenizer->current_char = '\0';

    tokenizer->pending_token = (token_t){ .type = NONE };
    tokenizer->has_pending_token = false;
    tokenizer->temporary_buffer = (string_t){};
    tokenizer->last_start_tag_name = (string_t){};
    tokenizer->last_error = NO_ERROR;

    tokenizer->token_queue = &parser->token_queue;
    tokenizer->stream = stream;
    tokenizer->chars_consumed = 0;
    tokenizer->peek_count = 0;

    parser_string_init_sized(&tokenizer->temporary_buffer, TEMPORARY_BUFFER_SIZE);

    // Tree builder initialization
    builder->insertion_state = INITIAL;
    builder->temporary_state = INITIAL;
    builder->temporary_state_flag = false;
    builder->consume_flag = true;
    builder->current_token = (token_t){ .type = NONE };
    builder->parser_cannot_change_mode = false;

    builder->pending_node = (dom_node_t){ .type = DOM_NONE };
    builder->has_pending_node = false;

    builder->parser = parser;
    builder->open_elem_stack = &parser->open_elem_stack;
    builder->token_stream = &parser->token_queue;
    builder->root_node = NULL;
    builder->head_element = NULL;
}

void parser_change_tokenizer_state(parser_t* parser, const data_state_t state) {
    parser->tokenizer.data_state = state;
}
