#ifndef HTML_PARSER_DRIVER_H
#define HTML_PARSER_DRIVER_H

#include "tokenizer.h"
#include "tree_builder.h"

typedef struct parser {
    tokenizer_t         tokenizer;
    tree_builder_t      builder;

    open_elem_stack_t   open_elem_stack;
    token_queue_t       token_queue;
} parser_t;

int run_parser(int argc, char **argv);
dom_node_t* parse_document(parser_t* parser);
void parser_init(parser_t* parser, FILE* stream);
void parser_change_tokenizer_state(parser_t* parser, data_state_t state);

#endif //HTML_PARSER_DRIVER_H