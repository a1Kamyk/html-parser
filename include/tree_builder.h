#ifndef HTML_PARSER_TREE_H
#define HTML_PARSER_TREE_H

#include "tokenizer.h"
#include "misc.h"

#define OPEN_ELEM_STACK_SIZE 32

typedef enum {
    NODE_OK,
    NODE_ERROR,
    NODE_REPROCESS
} node_result_t;

/// tokenizer state machine insertion states
typedef enum {
    INITIAL = 0,
    BEFORE_HTML,
    BEFORE_HEAD,
    IN_HEAD,
    IN_HEAD_NOSCRIPT,
    AFTER_HEAD,
    IN_BODY,
    TEXT,
    IN_TABLE,
    IN_TABLE_TEXT,
    IN_CAPTION,
    IN_COLUMN_GROUP,
    IN_TABLE_BODY,
    IN_ROW,
    IN_CELL,
    IN_TEMPLATE,
    AFTER_BODY,
    IN_FRAMESET,
    AFTER_FRAMESET,
    AFTER_AFTER_BODY,
    AFTER_AFTER_FRAMESET,
    INSERTION_STATE_COUNT
} insertion_state_t;

/// Type of stored node
typedef enum {
    DOM_NONE = 0,
    DOM_ROOT,
    DOM_ELEMENT,
    DOM_TEXT,
    DOM_COMMENT,
    DOM_DOCTYPE,
    DOM_TYPES_COUNT
} dom_node_type_t;

typedef struct {
    string_t name;
    string_t public_id;
    string_t system_id;
} doctype_node_t;

typedef struct {
    string_t tag_name;
    attribute_list_t attributes;
} element_node_t;

typedef struct {
    string_t text;
} comment_node_t;

/// Data of stored node
typedef union {
    doctype_node_t doctype;
    element_node_t element;
    comment_node_t comment;
} node_data_t;

typedef struct dom_tree_node {
    /// Self data
    dom_node_type_t type;
    node_data_t     data;

    /// Parent and children data
    dom_node_t*         parent;
    dom_node_t**        children;
    size_t              children_amount;
    size_t              children_capacity;
} dom_node_t;

typedef struct open_elem_stack {
    dom_node_t* elements[OPEN_ELEM_STACK_SIZE];
    size_t stack_top;
} open_elem_stack_t;

typedef struct {
    /// Parser state
    insertion_state_t   insertion_state;
    insertion_state_t   temporary_state;
    bool                temporary_state_flag;
    bool                consume_flag;
    token_t             current_token;
    bool                parser_cannot_change_mode;

    /// Temporary storage fields
    dom_node_t          pending_node;
    bool                has_pending_node;

    /// Helper and miscellaneous fields
    open_elem_stack_t*  open_elem_stack;
    token_queue_t*      token_stream;
    dom_node_t*         root_node;
    dom_node_t*         head_element;
} tree_builder_t;

void stack_init(open_elem_stack_t* stack);
int stack_pop(open_elem_stack_t* stack, dom_node_t** out);
int stack_push(open_elem_stack_t* stack, dom_node_t* node);
dom_node_t* stack_top(const open_elem_stack_t* stack);
bool stack_is_empty(const open_elem_stack_t* stack);
bool stack_is_full(const open_elem_stack_t* stack);

dom_node_t* create_root_node();
dom_node_t* create_tree_node(token_t* token);
dom_node_t* add_child(dom_node_t *parent, dom_node_t *child);
dom_node_t* add_child_move(dom_node_t* parent, dom_node_t* ref);
void delete_tree_node(dom_node_t* node);
void delete_doctype_node_data(doctype_node_t* node);
void delete_element_node_data(element_node_t* node);
void delete_comment_node_data(comment_node_t* node);
void delete_token(token_t* token);

void consume_token(tree_builder_t* builder);
void reprocess_token(tree_builder_t* builder);
int get_new_comment_node(dom_node_t* out);
int get_new_doctype_node(dom_node_t* out);
int get_new_element_node(dom_node_t* out);
int get_element_for_token(dom_node_t* out, token_t* token);
dom_node_t* insert_element_for_token(tree_builder_t* builder,
                             token_t* token,
                             bool only_add_to_element_stack,
                             dom_node_t* insert_location);
dom_node_t* current_insertion_point(const tree_builder_t* builder);

/// State handlers
node_result_t handle_initial_state(tree_builder_t* builder);
node_result_t handle_before_html_state(tree_builder_t* builder);
node_result_t handle_before_head_state(tree_builder_t* builder);
node_result_t handle_in_head_state(tree_builder_t* builder);
node_result_t handle_in_head_noscript_state(tree_builder_t* builder);
node_result_t handle_after_head_state(tree_builder_t* builder);
node_result_t handle_in_body_state(tree_builder_t* builder);
node_result_t handle_text_state(tree_builder_t* builder);
node_result_t handle_in_table_state(tree_builder_t* builder);
node_result_t handle_in_table_text_state(tree_builder_t* builder);
node_result_t handle_in_caption_state(tree_builder_t* builder);
node_result_t handle_in_column_group_state(tree_builder_t* builder);
node_result_t handle_in_table_body_state(tree_builder_t* builder);
node_result_t handle_in_row_state(tree_builder_t* builder);
node_result_t handle_in_cell_state(tree_builder_t* builder);
node_result_t handle_in_template_state(tree_builder_t* builder);
node_result_t handle_after_body_state(tree_builder_t* builder);
node_result_t handle_in_frameset_state(tree_builder_t* builder);
node_result_t handle_after_frameset_state(tree_builder_t* builder);
node_result_t handle_after_after_body_state(tree_builder_t* builder);
node_result_t handle_after_after_frameset_state(tree_builder_t* builder);

int tree_node_next(tree_builder_t* builder);

#endif //HTML_PARSER_TREE_H