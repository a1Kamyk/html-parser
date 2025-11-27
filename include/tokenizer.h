#ifndef HTML_PARSER_LEXER_H
#define HTML_PARSER_LEXER_H

#include <stdio.h>
#include <stdbool.h>

#include "misc.h"

#define TOKEN_QUEUE_SIZE  8
#define PEEK_BUFFER_LEN   16

typedef struct dom_tree_node dom_node_t;
struct parser;

typedef enum {
    TOKEN_OK,
    TOKEN_ERROR,
    TOKEN_PARSE_ERROR,
    TOKEN_RECONSUME
} token_result_t;

/// Parse errors
typedef enum {
    NO_ERROR = 0,
    ABRUPT_CLOSING_OF_EMPTY_COMMENT,
    ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER,
    ABRUPT_DOCTYPE_SYSTEM_IDENTIFIER,
    ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE,
    CDATA_IN_HTML_CONTENT,
    CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE,
    CONTROL_CHARACTER_IN_INPUT_STREAM,
    CONTROL_CHARACTER_REFERENCE,
    DUPLICATE_ATTRIBUTE,
    END_TAG_WITH_ATTRIBUTES,
    END_TAG_WITH_TRAILING_SOLIDUS,
    EOF_BEFORE_TAG_NAME,
    EOF_IN_CDATA,
    EOF_IN_COMMENT,
    EOF_IN_DOCTYPE,
    EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT,
    EOF_IN_TAG,
    INCORRECTLY_CLOSED_COMMENT,
    INCORRECTLY_OPENED_COMMENT,
    INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME,
    INVALID_FIRST_CHARACTER_OF_TAG_NAME,
    MISSING_ATTRIBUTE_VALUE,
    MISSING_DOCTYPE_NAME,
    MISSING_DOCTYPE_PUBLIC_IDENTIFIER,
    MISSING_DOCTYPE_SYSTEM_IDENTIFIER,
    MISSING_END_TAG_NAME,
    MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER,
    MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER,
    MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE,
    MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD,
    MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD,
    MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME,
    MISSING_WHITESPACE_BETWEEN_ATTRIBUTES,
    MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS,
    NESTED_COMMENT,
    NONCHARACTER_CHARACTER_REFERENCE,
    NONCHARACTER_IN_INPUT_STREAM,
    NON_VOID_HTML_ELEMENT_START_TAG_WITH_TRAILING_SOLIDUS,
    NULL_CHARACTER_REFERENCE,
    SURROGATE_CHARACTER_REFERENCE,
    SURROGATE_IN_INPUT_STREAM,
    UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_IDENTIFIER,
    UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME,
    UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE,
    UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME,
    UNEXPECTED_NULL_CHARACTER,
    UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME,
    UNEXPECTED_SOLIDUS_IN_TAG,
    UNKNOWN_NAMED_CHARACTER_REFERENCE,
    PARSE_ERROR_COUNT
} parse_error_t;

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
    ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE,
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
    DATA_STATE_COUNT
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
    attribute_list_t attributes;
} tag_token;

/// Comment token type
typedef struct {
    string_t    text;
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
    token_t tokens[TOKEN_QUEUE_SIZE];
    size_t start;
    size_t end;
    size_t count;
} token_queue_t;

typedef struct {
    /// Tokenizer state
    data_state_t        data_state;
    data_state_t        return_state;
    bool                consume_flag;
    int                 current_char;

    /// Temporary storage fields
    token_t             pending_token;
    bool                has_pending_token;
    string_t            temporary_buffer;
    parse_error_t       last_error;

    /// Helper and miscellaneous fields
    token_queue_t*      token_queue;
    FILE*               stream;
    size_t              chars_consumed;
    int                 peek_buffer[PEEK_BUFFER_LEN];
    size_t              peek_count;
    struct parser*      parser;
} tokenizer_t;

void queue_init(token_queue_t* queue);
int queue_pop(token_queue_t* queue, token_t* out);
int queue_push(token_queue_t* queue, const token_t* token);
bool queue_is_empty(const token_queue_t* queue);
bool queue_is_full(const token_queue_t* queue);

bool is_void_element(token_t token);
bool is_template_element(token_t token);
bool is_raw_text_element(token_t token);
bool is_escapable_raw_text_element(token_t token);
bool is_foreign_element(token_t token);
bool is_normal_element(token_t token);

bool is_peek_buf_empty(const tokenizer_t* tokenizer);
bool try_match_peek_buf(tokenizer_t* tokenizer, const char* str);
void consume_peek_buf(tokenizer_t* tokenizer);
void consume_character(tokenizer_t* tokenizer);
void reconsume_character(tokenizer_t* tokenizer);
token_t get_character_token(int c);
token_t get_eof_token();
token_t get_new_start_tag_token();
token_t get_new_end_tag_token();
token_t get_new_comment_token();
token_t get_new_doctype_token();

token_result_t emit_token(const tokenizer_t* tokenizer, const token_t* token);
token_result_t pop_token(tokenizer_t* tokenizer, token_t* out);

/// State handlers
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

token_result_t handle_before_attribute_name_state(tokenizer_t* tokenizer);
token_result_t handle_attribute_name_state(tokenizer_t* tokenizer);
// after attribute name state
token_result_t handle_before_attribute_value_state(tokenizer_t* tokenizer);
token_result_t handle_attribute_value_double_quoted_state(tokenizer_t* tokenizer);
token_result_t handle_attribute_value_single_quote_state(tokenizer_t* tokenizer);
token_result_t handle_attribute_value_unquote_state(tokenizer_t* tokenizer);
token_result_t handle_after_attribute_value_quoted_state(tokenizer_t* tokenizer);

token_result_t handle_markup_declaration_open_state(tokenizer_t* tokenizer);

token_result_t handle_doctype_state(tokenizer_t* tokenizer);
token_result_t handle_before_doctype_name_state(tokenizer_t* tokenizer);
token_result_t handle_doctype_name_state(tokenizer_t* tokenizer);
token_result_t handle_after_doctype_name_state(tokenizer_t* tokenizer);

int token_next(tokenizer_t* tokenizer);

#endif //HTML_PARSER_LEXER_H