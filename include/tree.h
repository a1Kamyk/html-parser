#ifndef HTML_PARSER_TREE_H
#define HTML_PARSER_TREE_H

#include "tokenizer.h"

/// Type of stored node
typedef enum {
    DOM_ROOT,
    DOM_ELEMENT,
    DOM_TEXT,
    DOM_COMMENT,
    DOM_DOCTYPE
} dom_node_type_t;

typedef struct {
    char* tag_name;
    // TODO implement attributes
    // attribute_t* attributes;
    size_t attribute_count;
} element_node_t;

typedef struct {
    char* text;
} comment_node_t;

/// Data of stored node
typedef union {
    element_node_t element;
    comment_node_t comment;
} node_data_t;

typedef struct dom_tree_node {
    /// Self data
    dom_node_type_t type;
    node_data_t     data;

    /// Parent and children data
    dom_tree_node_t*    parent;
    dom_tree_node_t**   children;
    size_t              children_amount;
    size_t              children_capacity;
} dom_tree_node_t;

dom_tree_node_t *create_tree_node(token_t* token);
void add_child(dom_tree_node_t *parent, dom_tree_node_t *child);

#endif //HTML_PARSER_TREE_H