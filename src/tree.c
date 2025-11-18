#include "tree.h"

#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"

static const size_t INITIAL_CHILD_CAPACITY = 4;
static const size_t DYN_ARR_EXPANSION_COEFFICIENT = 2;

dom_tree_node_t *create_tree_node(token_t* token) {
    dom_tree_node_t *node = calloc(1, sizeof(dom_tree_node_t));
    if (!node) {
        fprintf(stderr, "Out of memory allocating new tree node");
        return NULL;
    }

    // TODO only create a children array if node not a text or comment type
    node->children = calloc(INITIAL_CHILD_CAPACITY, sizeof(dom_tree_node_t*));
    if (!node->children) {
        fprintf(stderr, "Out of memory allocating new child array");
        return NULL;
    }
    node->children_capacity = INITIAL_CHILD_CAPACITY;
    node->children_amount = 0;

    // TODO fix this basically
    switch (token->type) {
        case DOCTYPE:
        case START_TAG:
        case END_TAG:
        case COMMENT:
        case CHARACTER:
        case END_OF_FILE:
        default:
            break;
    }

    return node;
}


void add_child(dom_tree_node_t *parent, dom_tree_node_t *child) {
    if (parent->children_amount + 1 > parent->children_capacity) {
        if (parent->children_capacity == 0)
            parent->children_capacity = 4;
        else
            parent->children_capacity *= DYN_ARR_EXPANSION_COEFFICIENT;

        dom_tree_node_t **new_arr = realloc(
            parent->children,
    parent->children_capacity * sizeof(dom_tree_node_t*)
        );

        if (!new_arr) {
            fprintf(stderr, "Out of memory allocating child array\n");
            return;
        }
        parent->children = new_arr;
    }
    parent->children[parent->children_amount++] = child;
    child->parent = parent;
}