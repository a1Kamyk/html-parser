#ifndef HTML_PARSER_LEXER_H
#define HTML_PARSER_LEXER_H

#include <stdio.h>
#include <stdbool.h>

#include "tree.h"

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
    AFTER_AFTER_FRAMESET
} insertion_state_t;

/// tokenizer state machine states
typedef enum {
    DATA_STATE = 0,
    RCDATA_STATE,
    RAWTEXT_STATE,
    SCRIPT_DATA_STATE,
    PLAINTEXT_STATE,
    TAG_OPEN_STATE,
    END_TAG_OPEN_STATE,
    TAG_NAME_STATE,
    RCDATA_LESS_THAN_SIGN_STATE,
    RCDATA_END_TAG_OPEN_STATE,
    RCDATA_END_TAG_NAME_STATE,
    RAWTEXT_LESS_THAN_SIGN_STATE,
    RAWTEXT_END_TAG_OPEN_STATE,
    RAWTEXT_END_TAG_NAME_STATE,
    SCRIPT_DATA_LESS_THAN_SIGN_STATE,
    SCRIPT_DATA_END_TAG_OPEN_STATE,
    SCRIPT_DATA_END_TAG_NAME_STATE,
    SCRIPT_DATA_ESCAPE_START_STATE,
    SCRIPT_DATA_ESCAPE_START_DASH_STATE,
    SCRIPT_DATA_ESCAPED_STATE,
    SCRIPT_DATA_ESCAPED_DASH_STATE,
    SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN_STATE,
    SCRIPT_DATA_ESCAPED_END_TAG_OPEN_STATE,
    SCRIPT_DATA_ESCAPED_END_TAG_NAME_STATE,
    SCRIPT_DATA_DOUBLE_ESCAPE_START_STATE,
    SCRIPT_DATA_DOUBLE_ESCAPED_STATE,
    SCRIPT_DATA_DOUBLE_ESCAPED_DASH_STATE,
    SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH_STATE,
    SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN_STATE,
    SCRIPT_DATA_DOUBLE_ESCAPE_END_STATE,
    BEFORE_ATTRIBUTE_NAME_STATE,
    ATTRIBUTE_NAME_STATE,
    AFTER_ATTRIBUTE_NAME_STATE,
    BEFORE_ATTRIBUTE_VALUE_STATE,
    ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE,
    ATTRIBUTE_VALUE_SINGLE_QUOTE_STATE,
    ATTRIBUTE_VALUE_UNQUOTE_STATE,
    AFTER_ATTRIBUTE_VALUE_QUOTED_STATE,
    SELF_CLOSING_START_TAG_STATE,
    BOGUS_COMMENT_STATE,
    MARKUP_DECLARATION_OPEN_STATE,
    COMMENT_START_STATE,
    COMMENT_START_DASH_STATE,
    COMMENT_STATE,
    COMMENT_LESS_THAN_SIGN_STATE,
    COMMENT_LESS_THAN_SIGN_BANG_STATE,
    COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE,
    COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE,
    COMMENT_END_DASH_STATE,
    COMMENT_END_STATE,
    COMMENT_END_BANG_STATE,
    DOCTYPE_STATE,
    BEFORE_DOCTYPE_NAME_STATE,
    DOCTYPE_NAME_STATE,
    AFTER_DOCTYPE_NAME_STATE,
    AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE,
    BEFORE_DOCTYPE_PUBLIC_IDENTIFIER_STATE,
    DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED_STATE,
    DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED_STATE,
    AFTER_DOCTYPE_PUBLIC_IDENTIFIER_STATE,
    BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS_STATE,
    AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE,
    BEFORE_DOCTYPE_SYSTEM_IDENTIFIER_STATE,
    DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED_STATE,
    DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED_STATE,
    AFTER_DOCTYPE_SYSTEM_IDENTIFIER_STATE,
    BOGUS_DOCTYPE_STATE,
    CDATA_SECTION_STATE,
    CDATA_SECTION_BRACKET_STATE,
    CDATA_SECTION_END_STATE,
    CHARACTER_REFERENCE_STATE,
    NAMED_CHARACTER_REFERENCE_STATE,
    AMBIGUOUS_AMPERSAND_STATE,
    NUMERIC_CHARACTER_REFERENCE_STATE,
    HEXADECIMAL_CHARACTER_REFERENCE_START_STATE,
    DECIMAL_CHARACTER_REFERENCE_START_STATE,
    HEXADECIMAL_CHARACTER_REFERENCE_STATE,
    DECIMAL_CHARACTER_REFERENCE_STATE,
    NUMERIC_CHARACTER_REFERENCE_END_STATE,
} data_state_t;

/// Token types
typedef enum {
    DOCTYPE = 0,
    START_TAG,
    END_TAG,
    COMMENT,
    CHARACTER,
    END_OF_FILE
} token_type_t;

/// Attribute type
typedef struct {
    char*       name;
    char*       value;
} attribute;

/// DOCTYPE token type
typedef struct {
    char*       name;
    char*       public_id;
    char*       system_id;
    bool        force_quirks;
} doctype_token;

/// Start and End tags token type
typedef struct {
    char*       name;
    bool        self_closing_flag;
    attribute*  attributes;
    size_t      attr_count;
} tag_token;

/// Comment and Character token type
typedef struct {
    char*       data;
} data_token;

/// Data contained by the token
typedef union {
    doctype_token   doctype;
    tag_token       tag;
    data_token      data;
} token_data_t;

/// Token type
typedef struct {
    token_type_t type;
    token_data_t data;
} token_t;

typedef struct {
    insertion_state_t   insertion_state;
    data_state_t        data_state;
} tokenizer_state_t;

typedef struct open_elem_stack {
    struct open_elem_stack *next;
} open_elem_stack_t;

bool is_void_element(token_t token);
bool is_template_element(token_t token);
bool is_raw_text_element(token_t token);
bool is_escapable_raw_text_element(token_t token);
bool is_foreign_element(token_t token);
bool is_normal_element(token_t token);

int consume_character(FILE* stream, tokenizer_state_t state);
dom_tree_node_t* tokenize(FILE* stream, tokenizer_state_t state);

#endif //HTML_PARSER_LEXER_H