#ifndef HTML_PARSER_DRIVER_H
#define HTML_PARSER_DRIVER_H

#include "tokenizer.h"
#include "tree_builder.h"

dom_node_t* parse_document(tokenizer_t* tokenizer, tree_builder_t* builder);
int run_parser(int argc, char **argv);
void parser_init(tokenizer_t* tokenizer, tree_builder_t* tree_builder,
                FILE* stream, token_queue_t* token_stream);

#endif //HTML_PARSER_DRIVER_H