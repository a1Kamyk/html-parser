#ifndef HTML_PARSER_LEXER_H
#define HTML_PARSER_LEXER_H

#include <stdio.h>
#include <stdbool.h>

#include "misc.h"

#define INTERNAL_QUEUE_SIZE  8

typedef struct dom_tree_node dom_tree_node_t;

typedef enum {
    TOKEN_OK,
    TOKEN_ERROR,
    TOKEN_PARSE_ERROR,
    TOKEN_RECONSUME
} token_result_t;

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
    AFTER_AFTER_FRAMESET
} insertion_state_t;

/// tokenizer state machine data states
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
    NONE = 0,
    DOCTYPE,
    START_TAG,
    END_TAG,
    COMMENT,
    CHARACTER,
    END_OF_FILE
} token_type_t;

/// Attribute type
typedef struct {
    string_t    name;
    string_t    value;
} attribute_t;

/// DOCTYPE token type
typedef struct {
    string_t    name;
    string_t    public_id;
    string_t    system_id;
    bool        force_quirks;
} doctype_token;

/// Start and End tags token type
typedef struct {
    string_t    name;
    bool        self_closing_flag;
    attribute_t*  attributes;
    size_t      attr_count;
} tag_token;

/// Comment token type
typedef struct {
    string_t    data;
} comment_token;

/// Character token type
typedef struct {
    int        character;
} character_token;

/// Data contained by the token
typedef union {
    doctype_token   doctype;
    tag_token       tag;
    comment_token   comment;
    character_token character;
} token_data_t;

/// Token type
typedef struct {
    token_type_t type;
    token_data_t data;
} token_t;

typedef struct {
    token_t tokens[INTERNAL_QUEUE_SIZE];
    size_t start;
    size_t end;
    size_t count;
} internal_token_queue_t;

typedef struct {
    /// Tokenizer state
    insertion_state_t   insertion_state;
    data_state_t        data_state;
    data_state_t        return_state;
    bool                consume_flag;
    int                 current_char;

    /// Temporary storage fields
    token_t             pending_token;
    bool                has_pending_token;
    string_t            temporary_buffer;

    /// Helper and miscellaneous fields
    internal_token_queue_t internal_token_queue;
    FILE* stream;
} tokenizer_t;

typedef struct open_elem_stack {
    struct open_elem_stack *next;
} open_elem_stack_t;

void queue_init(internal_token_queue_t* queue);
int queue_pop(internal_token_queue_t* queue, const token_t* out);
int queue_push(internal_token_queue_t* queue, const token_t* token);
bool queue_is_empty(const internal_token_queue_t* queue);
bool queue_is_full(const internal_token_queue_t* queue);

bool is_void_element(token_t token);
bool is_template_element(token_t token);
bool is_raw_text_element(token_t token);
bool is_escapable_raw_text_element(token_t token);
bool is_foreign_element(token_t token);
bool is_normal_element(token_t token);

void consume_character(tokenizer_t* tokenizer);
void reconsume_character(tokenizer_t* tokenizer);
token_t get_character_token(int c);
token_t get_eof_token();
int get_new_start_tag_token(token_t* out);
int get_new_end_tag_token(token_t* out);
int get_new_comment_token(token_t* out);

/// ONLY called in the tree building stage!
void destroy_token(token_t* token);

token_result_t push_token(tokenizer_t* tokenizer, const token_t* token);
token_result_t pop_token(tokenizer_t* tokenizer, token_t* out);

/// State handling functions
token_result_t handle_data_state(tokenizer_t* tokenizer);
token_result_t handle_rcdata_state(tokenizer_t* tokenizer);
token_result_t handle_rawtext_state(tokenizer_t* tokenizer);
token_result_t handle_script_data_state(tokenizer_t* tokenizer);
token_result_t handle_plaintext_state(tokenizer_t* tokenizer);
token_result_t handle_tag_open_state(tokenizer_t* tokenizer);
token_result_t handle_end_tag_open_state(tokenizer_t* tokenizer);
token_result_t handle_tag_name_state(tokenizer_t* tokenizer);
token_result_t handle_rcdata_less_than_sign_state(tokenizer_t* tokenizer);
token_result_t handle_rcdata_end_tag_open_state(tokenizer_t* tokenizer);
token_result_t handle_rcdata_end_tag_name_state(tokenizer_t* tokenizer);

int token_next(tokenizer_t* tokenizer);

#endif //HTML_PARSER_LEXER_H