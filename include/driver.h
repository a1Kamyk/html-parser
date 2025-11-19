#ifndef HTML_PARSER_DRIVER_H
#define HTML_PARSER_DRIVER_H

#include "tokenizer.h"

dom_tree_node_t* parse_document(tokenizer_t* tokenizer);
int run_parser(int argc, char **argv);

#endif //HTML_PARSER_DRIVER_H