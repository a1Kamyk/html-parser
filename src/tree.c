#include "tree.h"

#include <stdio.h>
#include <stdlib.h>

static const size_t DYN_ARR_EXPANSION_COEFFICIENT = 2;

dom_tree_node_t *create_tree_node(void* data) {
    dom_tree_node_t *node = calloc(1, sizeof(dom_tree_node_t));
    if (!node) {
        printf("Malloc failed: %i", errno);
        return NULL;
    }

    node->data = data;
    data = NULL;

    return node;
}


void add_child(dom_tree_node_t *node, dom_tree_node_t *child) {
    if (node->children_amount + 1 <= node->children_capacity) {
        node->children[node->children_amount++] = child;
        return;
    }

    dom_tree_node_t **new_arr = realloc(node->children,
                            node->children_capacity * sizeof(dom_tree_node_t *) * DYN_ARR_EXPANSION_COEFFICIENT
                            );
    if (!new_arr) {
        printf("Failed adding child node");
        return;
    }

    node->children = new_arr;
    node->children_capacity *= DYN_ARR_EXPANSION_COEFFICIENT;
    node->children[node->children_amount++] = child;
}