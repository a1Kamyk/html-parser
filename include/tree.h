#ifndef HTML_PARSER_TREE_H
#define HTML_PARSER_TREE_H

#include <stdint.h>

typedef struct tree_node {
    struct tree_node **children;
    size_t children_amount;
    size_t children_capacity;

    void *data;
} tree_node_t;

tree_node_t *create_tree_node(void* data);
void add_child(tree_node_t *node, tree_node_t *child);

#endif //HTML_PARSER_TREE_H