#ifndef HTML_PARSER_TREE_H
#define HTML_PARSER_TREE_H

#include <stdint.h>

#include "tokenizer.h"

typedef struct dom_tree_node {
    struct dom_tree_node **children;
    size_t children_amount;
    size_t children_capacity;

    token_t* token;
} dom_tree_node_t;

dom_tree_node_t *create_tree_node(void* data);
void add_child(dom_tree_node_t *node, dom_tree_node_t *child);

#endif //HTML_PARSER_TREE_H