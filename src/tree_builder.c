#include "tree_builder.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"

static const size_t INITIAL_CHILD_CAPACITY = 4;
static const size_t DYN_ARR_EXPANSION_COEFFICIENT = 2;

static const char* special_tags[] = {
    "address",
    "applet",
    "area",
    "article",
    "aside",
    "base",
    "basefont",
    "bgsound",
    "blockquote",
    "body",
    "br",
    "button",
    "caption",
    "center",
    "col",
    "colgroup",
    "dd",
    "details",
    "dir",
    "div",
    "dl",
    "dt",
    "embed",
    "fieldset",
    "figcaption",
    "figure",
    "footer",
    "form",
    "frame",
    "frameset",
    "h1",
    "h2",
    "h3",
    "h4",
    "h5",
    "h6",
    "head",
    "header",
    "hgroup",
    "hr",
    "html",
    "iframe",
    "img",
    "input",
    "keygen",
    "li",
    "link",
    "listing",
    "main",
    "marquee",
    "menu",
    "meta",
    "nav",
    "noembed",
    "noframes",
    "noscript",
    "object",
    "ol",
    "p",
    "param",
    "plaintext",
    "pre",
    "script",
    "search",
    "section",
    "select",
    "source",
    "style",
    "summary",
    "table",
    "tbody",
    "td",
    "template",
    "textarea",
    "tfoot",
    "th",
    "thead",
    "title",
    "tr",
    "track",
    "ul",
    "wbr",
    "xmp"
};

static const char* formatting_tags[] = {
    "a",
    "b",
    "big",
    "code",
    "em",
    "font",
    "i",
    "nobr",
    "s",
    "small",
    "strike",
    "strong",
    "tt",
    "u"
};

static const char* node_type_to_string(const dom_node_type_t type) {
    static const char* types[DOM_TYPES_COUNT] = {
        "dom-none",
        "dom-root",
        "dom-element",
        "dom-text",
        "dom-comment",
        "dom-doctype"
    };
    if (type >= DOM_TYPES_COUNT)
        return "Unknown node type";
    return types[type];
}

static const char* insertion_state_to_string(const insertion_state_t state) {
    static const char* states[INSERTION_STATE_COUNT] = {
        "initial",
        "before-html",
        "before-head",
        "in-head",
        "in-head-noscript",
        "after-head",
        "in-body",
        "text",
        "in-table",
        "in-table-text",
        "in-caption",
        "in-column-group",
        "in-table-body",
        "in-row",
        "in-cell",
        "in-template",
        "after-body",
        "in-frameset",
        "after-frameset",
        "after-after-body",
        "after-after-frameset",
    };
    if (state >= INSERTION_STATE_COUNT)
        return "Unknown insertion state";
    return states[state];
}

int init_children_array(dom_node_t* node) {
    node->children = calloc(INITIAL_CHILD_CAPACITY, sizeof(dom_node_t*));
    if (!node->children) {
        fprintf(stderr, "Out of memory allocating new child array");
        return 1;
    }

    node->children_capacity = INITIAL_CHILD_CAPACITY;
    node->children_amount = 0;

    return 0;
}

void stack_init(open_elem_stack_t* stack) {
    *stack = (open_elem_stack_t){0};
}

int stack_pop(open_elem_stack_t* stack, dom_node_t** out) {
    if (!stack || !out || stack_is_empty(stack))
        return 1;

    *out = stack->elements[stack->stack_top--];
    return 0;
}

int stack_push(open_elem_stack_t* stack, dom_node_t* node) {
    if (!stack || !node || stack_is_full(stack))
        return 1;

    stack->elements[stack->stack_top++] = node;
    return 0;
}

dom_node_t* stack_top(const open_elem_stack_t* stack) {
    if (stack_is_empty(stack))
        return NULL;
    return stack->elements[stack->stack_top];
}

bool stack_is_empty(const open_elem_stack_t* stack) {
    return stack->stack_top == 0;
}

bool stack_is_full(const open_elem_stack_t* stack) {
    return stack->stack_top == OPEN_ELEM_STACK_SIZE - 1;
}

dom_node_t* create_root_node() {
    dom_node_t* root = calloc(1, sizeof(dom_node_t));
    if (!root) {
        fprintf(stderr, "Out of memory allocating root node");
        return NULL;
    }

    const int res = init_children_array(root);
    if (res != 0) {
        free(root);
        return NULL;
    }

    root->type = DOM_ROOT;
    root->parent = NULL;

    return root;
}

dom_node_t* add_child(dom_node_t *parent, dom_node_t *child) {
    if (!parent)
        return NULL;
    if (parent->children_amount + 1 > parent->children_capacity) {
        if (parent->children_capacity == 0)
            parent->children_capacity = 4;
        else
            parent->children_capacity *= DYN_ARR_EXPANSION_COEFFICIENT;

        dom_node_t **new_arr = realloc(
            parent->children,
    parent->children_capacity * sizeof(dom_node_t*)
        );

        if (!new_arr) {
            fprintf(stderr, "Out of memory allocating child array\n");
            return NULL;
        }
        parent->children = new_arr;
    }
    parent->children[parent->children_amount++] = child;
    child->parent = parent;
    return child;
}

dom_node_t* add_child_move(dom_node_t* parent, dom_node_t* ref) {
    if (!ref || !parent)
        return NULL;
    dom_node_t* new_node = calloc(1, sizeof(dom_node_t));
    if (!new_node)
        return NULL;
    *new_node = *ref;
    if (add_child(parent, new_node) == NULL) {
        *new_node = (dom_node_t){0};
        free(new_node);
        return NULL;
    }
    *ref = (dom_node_t){};
    return new_node;
}

void delete_tree_node(dom_node_t* node) {
    if (!node)
        return;
    for (size_t i = 0; i < node->children_amount; i++) {
        delete_tree_node(node->children[i]);
    }
    free(node->children);

    switch (node->type) {
        case DOM_ELEMENT:
            delete_element_node_data(&node->data.element);
            break;
        case DOM_COMMENT:
            delete_comment_node_data(&node->data.comment);
            break;
        case DOM_DOCTYPE:
            delete_doctype_node_data(&node->data.doctype);
            break;
        case DOM_ROOT:
        case DOM_TEXT:
            break;
        default: {
            printf("Unknown node type %s\n", node_type_to_string(node->type));
            break;
        }
    }
}

void delete_doctype_node_data(doctype_node_t* node) {
    if (!node)
        return;
    parser_string_delete(&node->name);
    parser_string_delete(&node->public_id);
    parser_string_delete(&node->system_id);
}

void delete_element_node_data(element_node_t* node) {
    if (!node)
        return;
    parser_string_delete(&node->tag_name);
    attribute_list_delete(&node->attributes);
}

void delete_comment_node_data(comment_node_t* node) {
    if (!node)
        return;
    parser_string_delete(&node->text);
}

void delete_token(token_t* token) {
    if (!token) return;
    switch (token->type) {
        case DOCTYPE: {
            doctype_token* token_data = &token->data.doctype;
            parser_string_delete(&token_data->name);
            parser_string_delete(&token_data->public_id);
            parser_string_delete(&token_data->system_id);
            return;
        }
        case START_TAG:
        case END_TAG: {
            tag_token* token_data = &token->data.tag;
            parser_string_delete(&token_data->name);
            attribute_list_delete(&token_data->attributes);
            return;
        }
        case COMMENT: {
            comment_token* token_data = &token->data.comment;
            parser_string_delete(&token_data->text);
            return;
        }
        case NONE:
        case CHARACTER:
        case END_OF_FILE: {
            return;
        }
        default: {
            printf("Unknown token type: %d\n", token->type);
        }
    }
}

void consume_token(tree_builder_t* builder) {
    delete_token(&builder->current_token);
    queue_pop(builder->token_stream, &builder->current_token);
}

void reprocess_token(tree_builder_t* builder) {
    builder->consume_flag = true;
}

static void set_default_params(dom_node_t* node) {
    node->parent = NULL;
    node->children = NULL;
    node->children_amount = 0;
    node->children_capacity = 0;
}

int get_new_comment_node(dom_node_t* out) {
    out->type = DOM_COMMENT;
    set_default_params(out);

    return 0;
}

int get_new_doctype_node(dom_node_t* out) {
    out->type = DOM_DOCTYPE;
    set_default_params(out);

    out->data.doctype.name = (string_t){0};
    out->data.doctype.public_id = (string_t){0};
    out->data.doctype.system_id = (string_t){0};

    return 0;
}

int get_new_element_node(dom_node_t* out) {
    out->type = DOM_ELEMENT;
    set_default_params(out);

    out->data.element.attributes = (attribute_list_t){0};
    out->data.element.tag_name = (string_t){0};

    return 0;
}

int get_element_for_token(dom_node_t* out, token_t* token) {
    // omitting namespace and `is` attribute
    if (!out || !token)
        return 1;
    tag_token* token_data = &token->data.tag;
    out->type = DOM_ELEMENT;
    set_default_params(out);
    parser_move_string(&out->data.element.tag_name,
        &token_data->name);

    attribute_list_move(&out->data.element.attributes,
        &token_data->attributes);

    return 0;
}

dom_node_t* insert_element_for_token(tree_builder_t* builder,
                             token_t* token,
                             bool only_add_to_element_stack,
                             dom_node_t* insert_location) {
    if (!builder || !token)
        return NULL;
    if (!insert_location)
        insert_location = current_insertion_point(builder);
    dom_node_t element;
    get_element_for_token(&element, token);
    dom_node_t* node = add_child_move(insert_location, &element);
    if (!node)
        goto fail;
    if (stack_push(builder->open_elem_stack, &element) != 0)
        goto fail;
    return node;

    fail:
    delete_tree_node(&element);
    return NULL;
}

dom_node_t* current_insertion_point(const tree_builder_t* builder) {
    dom_node_t* insert = stack_top(builder->open_elem_stack);
    if (!insert)
        return builder->root_node;
    return insert;
}

static void handle_consume_flag(tree_builder_t* builder) {
    if (builder->consume_flag)
        consume_token(builder);
    else
        reprocess_token(builder);
}

node_result_t handle_initial_state(tree_builder_t* builder) {
    handle_consume_flag(builder);
    token_t* current_token = &builder->current_token;
    switch (current_token->type) {
        case COMMENT: {
            get_new_comment_node(&builder->pending_node);
            parser_move_string(&builder->pending_node.data.comment.text,
                &current_token->data.comment.text);
            if (add_child_move(builder->root_node, &builder->pending_node) == NULL) {
                delete_tree_node(&builder->pending_node);
                return NODE_ERROR;
            }
            return NODE_OK;
        }
        case DOCTYPE: {
            doctype_token* token_data = &current_token->data.doctype;
            // omitted a condition from the spec
            if (parser_cstrcmp("html", &token_data->name) != 0 ||
                token_data->public_id.data != NULL) {
                return NODE_ERROR;
            }

            // using { NULL, 0, 0 } to represent empty string
            get_new_doctype_node(&builder->pending_node);
            parser_move_string(&builder->pending_node.data.doctype.name,
                &token_data->name);
            parser_move_string(&builder->pending_node.data.doctype.public_id,
                &token_data->public_id);
            parser_move_string(&builder->pending_node.data.doctype.system_id,
                &token_data->system_id);

            // omitted setting document quirks mode / limited quirks mode
            if (add_child_move(builder->root_node, &builder->pending_node) == NULL) {
                delete_tree_node(&builder->pending_node);
                return NODE_ERROR;
            }
            builder->insertion_state = BEFORE_HTML;
            return NODE_OK;
        }
        default: {
            if (current_token->type == CHARACTER &&
                is_html_whitespace(current_token->data.character.character)) {
                return NODE_OK;
            }
            // omitted srcdoc case
            return NODE_ERROR;
        }
    }
}

node_result_t handle_before_html_state(tree_builder_t* builder) {
    handle_consume_flag(builder);
    token_t* current_token = &builder->current_token;
    switch (current_token->type) {
        case DOCTYPE: {
            return NODE_OK;
        }
        case COMMENT: {
            get_new_comment_node(&builder->pending_node);
            parser_move_string(&builder->pending_node.data.comment.text,
                &current_token->data.comment.text);
            if (add_child_move(builder->root_node, &builder->pending_node) == NULL) {
                delete_tree_node(&builder->pending_node);
                return NODE_ERROR;
            }
            return NODE_OK;
        }
        case CHARACTER: {
            if (is_html_whitespace(current_token->data.character.character))
                    return NODE_OK;
            // anything else case
            break;
        }
        case START_TAG: {
            if (parser_cstrcmp("html", &current_token->data.tag.name) == 0) {
                get_new_element_node(&builder->pending_node);
                get_element_for_token(&builder->pending_node, current_token);

                dom_node_t* node = add_child_move(builder->root_node, &builder->pending_node);
                if (!node)
                    goto fail;
                if (stack_push(builder->open_elem_stack, node) != 0)
                    goto fail;
                builder->insertion_state = BEFORE_HEAD;
                return NODE_OK;
            }
            // anything else case
            break;
        }
        case END_TAG: {
            const string_t* name = &current_token->data.tag.name;
            if (parser_cstrcmp("head", name) == 0 ||
                parser_cstrcmp("body", name) == 0 ||
                parser_cstrcmp("html", name) == 0 ||
                parser_cstrcmp("br", name) == 0) {
                // anything else case
                break;
            }
            // parse error
            return NODE_ERROR;
        }
        default: {
            // anything else case
            break;
        }
    }

    // anything else
    get_new_element_node(&builder->pending_node);
    element_node_t* node_data = &builder->pending_node.data.element;
    if (parser_string_init_cstr(&node_data->tag_name, "html") != 0)
        goto fail;
    node_data->attributes = (attribute_list_t){0};

    dom_node_t* node = add_child_move(builder->root_node, &builder->pending_node);
    if (!node)
        goto fail;
    if (stack_push(builder->open_elem_stack, node) != 0)
        goto fail;

    builder->insertion_state = BEFORE_HEAD;
    return NODE_OK;

    fail: {
        delete_tree_node(&builder->pending_node);
        return NODE_ERROR;
    }
}

node_result_t handle_before_head_state(tree_builder_t* builder) {
    handle_consume_flag(builder);
    token_t* current_token = &builder->current_token;
    switch (current_token->type) {
        case CHARACTER: {
            if (is_html_whitespace(current_token->data.character.character))
                return NODE_OK;
            break;
        }
        case COMMENT: {
            get_new_comment_node(&builder->pending_node);
            parser_move_string(&builder->pending_node.data.comment.text,
                &current_token->data.comment.text);
            dom_node_t* node = add_child_move(current_insertion_point(builder), &builder->pending_node);
            if (!node)
                goto fail;
            return NODE_OK;
        }
        case DOCTYPE: {
            // parse error
            return NODE_ERROR;
        }
        case START_TAG: {
            if (parser_cstrcmp("html", &current_token->data.tag.name) == 0) {
                builder->temporary_state = IN_BODY;
                builder->temporary_state_flag = true;
                builder->consume_flag = false;
                return NODE_REPROCESS;
            }
            if (parser_cstrcmp("head", &current_token->data.tag.name) == 0) {
                dom_node_t* insert = current_insertion_point(builder);
                dom_node_t* node = insert_element_for_token(builder, current_token,
                false, insert);
                if (!node)
                    return NODE_ERROR;
                builder->head_element = node;
                builder->insertion_state = IN_HEAD;
                return NODE_OK;
            }
        }
        case END_TAG: {
            if (parser_cstrcmp("head", &current_token->data.tag.name) == 0 ||
                parser_cstrcmp("body", &current_token->data.tag.name) == 0 ||
                parser_cstrcmp("html", &current_token->data.tag.name) == 0 ||
                parser_cstrcmp("br", &current_token->data.tag.name) == 0) {
                // anything else case
                break;
            }
            // parse error
            return NODE_ERROR;
        }
        default: {
            // anything else case
            break;
        }
    }

    // anything else
    token_t head_token = get_new_start_tag_token();
    dom_node_t* insert = current_insertion_point(builder);
    if (parser_string_init_cstr(&head_token.data.tag.name, "head") != 0)
        return NODE_ERROR;
    dom_node_t* node = insert_element_for_token(builder, &head_token,
        false, insert);
    if (!node)
        return NODE_ERROR;
    builder->head_element = node;
    builder->insertion_state = IN_HEAD;
    builder->consume_flag = false;
    return NODE_REPROCESS;

    fail:
    delete_tree_node(&builder->pending_node);
    return NODE_ERROR;
}

int tree_node_next(tree_builder_t* builder) {
    typedef node_result_t (*handler_t)(tree_builder_t* builder);
    const static handler_t handlers[] = {
        [INITIAL] = handle_initial_state,
        [BEFORE_HTML] = handle_before_html_state,
        [BEFORE_HEAD] = handle_before_head_state,
        [IN_HEAD] = NULL,
        [IN_HEAD_NOSCRIPT] = NULL,
        [AFTER_HEAD] = NULL,
        [IN_BODY] = NULL,
        [TEXT] = NULL,
        [IN_TABLE] = NULL,
        [IN_TABLE_TEXT] = NULL,
        [IN_CAPTION] = NULL,
        [IN_COLUMN_GROUP] = NULL,
        [IN_TABLE_BODY] = NULL,
        [IN_ROW] = NULL,
        [IN_CELL] = NULL,
        [IN_TEMPLATE] = NULL,
        [AFTER_BODY] = NULL,
        [IN_FRAMESET] = NULL,
        [AFTER_FRAMESET] = NULL,
        [AFTER_AFTER_BODY] = NULL,
        [AFTER_AFTER_FRAMESET] = NULL
    };

    node_result_t result = NODE_OK;
    do {
        assert(builder->insertion_state < INSERTION_STATE_COUNT);
        handler_t handler = NULL;
        if (builder->temporary_state_flag) {
            handler = handlers[builder->temporary_state];
            builder->temporary_state_flag = false;
        }
        else
            handler = handlers[builder->insertion_state];
        if (!handler) {
            printf("Unknown or unhandled tree builder state: %s\n",
                insertion_state_to_string(builder->insertion_state));
            result = NODE_ERROR;
            break;
        }
        result = handler(builder);
        // TODO match error handling with tokenizer loop
    }
    while (result == NODE_REPROCESS);
    return result;
}
