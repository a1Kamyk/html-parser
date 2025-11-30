#include "tokenizer.h"

#include <assert.h>
#include <string.h>

#include "misc.h"

static const char* parse_error_to_string(parse_error_t error_code) {
    static const char* parse_errors[PARSE_ERROR_COUNT] = {
        "no-error",
        "abrupt-closing-of-empty-comment",
        "abrupt-doctype-public-identifier",
        "abrupt-doctype-system-identifier",
        "absence-of-digits-in-numeric-character-reference",
        "cdata-in-html-content",
        "character-reference-outside-unicode-range",
        "control-character-in-input-stream",
        "control-character-reference",
        "duplicate-attribute",
        "end-tag-with-attributes",
        "end-tag-with-trailing-solidus",
        "eof-before-tag-name",
        "eof-in-cdata",
        "eof-in-comment",
        "eof-in-doctype",
        "eof-in-script-html-comment-like-text",
        "eof-in-tag",
        "incorrectly-closed-comment",
        "incorrectly-opened-comment",
        "invalid-character-sequence-after-doctype-name",
        "invalid-first-character-of-tag-name",
        "missing-attribute-value",
        "missing-doctype-name",
        "missing-doctype-public-identifier",
        "missing-doctype-system-identifier",
        "missing-end-tag-name",
        "missing-quote-before-doctype-public-identifier",
        "missing-quote-before-doctype-system-identifier",
        "missing-semicolon-after-character-reference",
        "missing-whitespace-after-doctype-public-keyword",
        "missing-whitespace-after-doctype-system-keyword",
        "missing-whitespace-before-doctype-name",
        "missing-whitespace-between-attributes",
        "missing-whitespace-between-doctype-public-and-system-identifiers",
        "nested-comment",
        "noncharacter-character-reference",
        "noncharacter-in-input-stream",
        "non-void-html-element-start-tag-with-trailing-solidus",
        "null-character-reference",
        "surrogate-character-reference",
        "surrogate-in-input-stream",
        "unexpected-character-after-doctype-system-identifier",
        "unexpected-character-in-attribute-name",
        "unexpected-character-in-unquoted-attribute-value",
        "unexpected-equals-sign-before-attribute-name",
        "unexpected-null-character",
        "unexpected-question-mark-instead-of-tag-name",
        "unexpected-solidus-in-tag",
        "unknown-named-character-reference"
    };
    if (error_code >= PARSE_ERROR_COUNT)
        return "unknown parse error";
    return parse_errors[error_code];
}

static const char* data_state_to_string(const data_state_t state) {
    static const char* states[DATA_STATE_COUNT] = {
        "data-state",
        "rcdata-state",
        "rawtext-state",
        "script-data-state",
        "plaintext-state",
        "tag-open-state",
        "end-tag-open-state",
        "tag-name-state",
        "rcdata-less-than-sign-state",
        "rcdata-end-tag-open-state",
        "rcdata-end-tag-name-state",
        "rawtext-less-than-sign-state",
        "rawtext-end-tag-open-state",
        "rawtext-end-tag-name-state",
        "script-data-less-than-sign-state",
        "script-data-end-tag-open-state",
        "script-data-end-tag-name-state",
        "script-data-escape-start-state",
        "script-data-escape-start-dash-state",
        "script-data-escaped-state",
        "script-data-escaped-dash-state",
        "script-data-escaped-less-than-sign-state",
        "script-data-escaped-end-tag-open-state",
        "script-data-escaped-end-tag-name-state",
        "script-data-double-escape-start-state",
        "script-data-double-escaped-state",
        "script-data-double-escaped-dash-state",
        "script-data-double-escaped-dash-dash-state",
        "script-data-double-escaped-less-than-sign-state",
        "script-data-double-escape-end-state",
        "before-attribute-name-state",
        "attribute-name-state",
        "after-attribute-name-state",
        "before-attribute-value-state",
        "attribute-value-double-quoted-state",
        "attribute-value-single-quote-state",
        "attribute-value-unquote-state",
        "after-attribute-value-quoted-state",
        "self-closing-start-tag-state",
        "bogus-comment-state",
        "markup-declaration-open-state",
        "comment-start-state",
        "comment-start-dash-state",
        "comment-state",
        "comment-less-than-sign-state",
        "comment-less-than-sign-bang-state",
        "comment-less-than-sign-bang-dash-state",
        "comment-less-than-sign-bang-dash-dash-state",
        "comment-end-dash-state",
        "comment-end-state",
        "comment-end-bang-state",
        "doctype-state",
        "before-doctype-name-state",
        "doctype-name-state",
        "after-doctype-name-state",
        "after-doctype-public-keyword-state",
        "before-doctype-public-identifier-state",
        "doctype-public-identifier-double-quoted-state",
        "doctype-public-identifier-single-quoted-state",
        "after-doctype-public-identifier-state",
        "between-doctype-public-and-system-identifiers-state",
        "after-doctype-system-keyword-state",
        "before-doctype-system-identifier-state",
        "doctype-system-identifier-double-quoted-state",
        "doctype-system-identifier-single-quoted-state",
        "after-doctype-system-identifier-state",
        "bogus-doctype-state",
        "cdata-section-state",
        "cdata-section-bracket-state",
        "cdata-section-end-state",
        "character-reference-state",
        "named-character-reference-state",
        "ambiguous-ampersand-state",
        "numeric-character-reference-state",
        "hexadecimal-character-reference-start-state",
        "decimal-character-reference-start-state",
        "hexadecimal-character-reference-state",
        "decimal-character-reference-state",
        "numeric-character-reference-end-state"
    };
    if (state >= DATA_STATE_COUNT)
        return "unknown data state";
    return states[state];
}

void queue_init(token_queue_t* queue) {
    queue->start = 0;
    queue->end = 0;
    queue->count = 0;
}

int queue_pop(token_queue_t* queue, token_t* out) {
    if (!queue || !out || queue_is_empty(queue))
        return 1;

    // caller takes ownership of the token and must delete it later
    *out = queue->tokens[queue->start];
    queue->start = (++queue->start) % TOKEN_QUEUE_SIZE;
    queue->count--;
    return 0;
}

int queue_push(token_queue_t* queue, const token_t* token) {
    if (queue_is_full(queue) || !token) return 1;

    queue->tokens[queue->end] = *token;
    queue->end = (++queue->end) % TOKEN_QUEUE_SIZE;
    queue->count++;
    return 0;
}

bool queue_is_empty(const token_queue_t* queue) {
    return queue->count == 0;
}

bool queue_is_full(const token_queue_t* queue) {
    return queue->count == TOKEN_QUEUE_SIZE;
}

bool is_peek_buf_empty(const tokenizer_t* tokenizer) {
    return tokenizer->peek_count == 0;
}

bool try_match_peek_buf(tokenizer_t* tokenizer, const char* str) {
    const size_t len = strlen(str);
    if (len + tokenizer->peek_count > PEEK_BUFFER_LEN)
        return false;
    for (size_t i = tokenizer->peek_count; i < len; i++) {
        const int c = fgetc(tokenizer->stream);
        if (c == EOF)
            return false;
        tokenizer->peek_buffer[i] = c;
        tokenizer->peek_count++;
    }

    for (size_t i = 0; i < len; i++) {
        if (tokenizer->peek_buffer[i] != str[i])
            return false;
    }
    return true;
}

void consume_peek_buf(tokenizer_t* tokenizer) {
    tokenizer->current_char = tokenizer->peek_buffer[tokenizer->peek_count - 1];
    tokenizer->chars_consumed += tokenizer->peek_count;
    tokenizer->peek_count = 0;
}

void consume_character(tokenizer_t* tokenizer) {
    if (is_peek_buf_empty(tokenizer))
        tokenizer->current_char = fgetc(tokenizer->stream);
    else {
        tokenizer->current_char = tokenizer->peek_buffer[tokenizer->peek_count - 1];
        tokenizer->peek_count--;
    }

    tokenizer->chars_consumed++;
}

void reconsume_character(tokenizer_t* tokenizer) {
    tokenizer->consume_flag = true;
}

token_result_t reconsume_in(tokenizer_t* tokenizer, data_state_t state) {
    tokenizer->data_state = state;
    tokenizer->consume_flag = false;
    return TOKEN_RECONSUME;
}

token_t get_character_token(const int c) {
    return (token_t){
        .type = CHARACTER,
        .data = {
            .character = c
        }
    };
}

token_t get_eof_token() {
    return (token_t){
        .type = END_OF_FILE
    };
}

token_t get_new_start_tag_token() {
    return (token_t){
        .type = START_TAG,
        .data.tag = {
            .name = (string_t){0},
            .self_closing_flag = false,
            .attributes = (attribute_list_t){0}
        }
    };
}

token_t get_new_end_tag_token() {
    return (token_t){
        .type = END_TAG,
        .data.tag = {
            .name = (string_t){0},
            .self_closing_flag = false,
            .attributes = (attribute_list_t){0}
        }
    };
}

token_t get_new_comment_token() {
    return (token_t){
        .type = COMMENT,
        .data.comment = {
            .text = (string_t){0},
        }
    };
}

token_t get_new_doctype_token() {
    return (token_t){
        .type = DOCTYPE,
        .data.doctype = {
            .name = (string_t){0},
            .public_id = (string_t){0},
            .system_id = (string_t){0},
            .force_quirks = false
        }
    };
}

token_result_t emit_token(const tokenizer_t* tokenizer, const token_t* token) {
    if (queue_push(tokenizer->token_queue, token) != 0 )
        return TOKEN_ERROR;
    return TOKEN_OK;
}

token_result_t emit_pending_token(tokenizer_t* tokenizer) {
    assert(tokenizer->has_pending_token);
    tokenizer->has_pending_token = false;
    return emit_token(tokenizer, &tokenizer->pending_token);
}

static void handle_consume_flag(tokenizer_t* tokenizer) {
    if (tokenizer->consume_flag)
        consume_character(tokenizer);
    else
        reconsume_character(tokenizer);
}

token_result_t handle_data_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case '&':
            tokenizer->return_state = DATA_STATE;
            tokenizer->data_state = CHARACTER_REFERENCE_STATE;
            return TOKEN_OK;
        case '<':
            tokenizer->data_state = TAG_OPEN_STATE;
            return TOKEN_OK;
        case NULL_CHARACTER: {
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            const token_t token = get_character_token(0);
            return emit_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            if (queue_push(tokenizer->token_queue, &token) != 0 )
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            const token_t token =  get_character_token(c);
            return emit_token(tokenizer, &token);
        }
    }
}

token_result_t handle_rcdata_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case '&':
            tokenizer->return_state = RCDATA_STATE;
            tokenizer->data_state = CHARACTER_REFERENCE_STATE;
            return TOKEN_OK;
        case '<':
            tokenizer->data_state = RCDATA_LESS_THAN_SIGN_STATE;
            return TOKEN_OK;
        case NULL_CHARACTER: {
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            const token_t token =  get_character_token(REPLACEMENT_CHARACTER);
            return emit_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            return emit_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return emit_token(tokenizer, &token);
        }
    }
}

token_result_t handle_rawtext_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case '<':
            tokenizer->data_state = RAWTEXT_LESS_THAN_SIGN_STATE;
            return TOKEN_OK;
        case NULL_CHARACTER: {
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            const token_t token = get_character_token(REPLACEMENT_CHARACTER);
            return emit_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            return emit_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return emit_token(tokenizer, &token);
        }
    }
}

token_result_t handle_script_data_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case '<':
            tokenizer->data_state = SCRIPT_DATA_LESS_THAN_SIGN_STATE;
            return TOKEN_OK;
        case NULL_CHARACTER: {
            const token_t token = get_character_token(REPLACEMENT_CHARACTER);
            if (queue_push(tokenizer->token_queue, &token) != 0 )
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        case EOF: {
            const token_t token = get_eof_token();
            return emit_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return emit_token(tokenizer, &token);
        }
    }
}

token_result_t handle_plaintext_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case NULL_CHARACTER: {
            const token_t token = get_character_token(REPLACEMENT_CHARACTER);
            return emit_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            return emit_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return emit_token(tokenizer, &token);
        }
    }
}

token_result_t handle_tag_open_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case '!':
            tokenizer->data_state = MARKUP_DECLARATION_OPEN_STATE;
            return TOKEN_OK;
        case '/':
            tokenizer->data_state = END_TAG_OPEN_STATE;
            return TOKEN_OK;
        case '?': {
            tokenizer->last_error = UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME;
            tokenizer->pending_token = get_new_comment_token();
            tokenizer->has_pending_token = true;
            return reconsume_in(tokenizer, BOGUS_COMMENT_STATE);
        }
        case EOF: {
            tokenizer->last_error = EOF_BEFORE_TAG_NAME;
            const token_t less_than_sign_token = get_character_token(LESS_THAN_SIGN);
            const token_t eof_token = get_eof_token();
            if (emit_token(tokenizer, &less_than_sign_token) != TOKEN_OK ||
                emit_token(tokenizer, &eof_token) != TOKEN_OK)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            if (is_ascii_alpha(c)) {
                tokenizer->pending_token = get_new_start_tag_token();
                tokenizer->has_pending_token = true;
                return reconsume_in(tokenizer, TAG_NAME_STATE);
            }
            tokenizer->last_error = INVALID_FIRST_CHARACTER_OF_TAG_NAME;
            const token_t token = get_character_token(LESS_THAN_SIGN);
            if (emit_token(tokenizer, &token) == TOKEN_ERROR)
                return TOKEN_ERROR;
            return reconsume_in(tokenizer, DATA_STATE);
        }
    }
}

token_result_t handle_end_tag_open_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case '>': {
            tokenizer->last_error = MISSING_END_TAG_NAME;
            tokenizer->data_state = DATA_STATE;
            return TOKEN_OK;
        }
        case EOF: {
            tokenizer->last_error = EOF_BEFORE_TAG_NAME;
            const token_t less_than_sign_token = get_character_token(LESS_THAN_SIGN);
            const token_t solidus_token = get_character_token(SOLIDUS);
            const token_t eof_token = get_eof_token();
            if (emit_token(tokenizer, &less_than_sign_token) != TOKEN_OK ||
                emit_token(tokenizer, &solidus_token) != TOKEN_OK ||
                emit_token(tokenizer, &eof_token) != TOKEN_OK)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            if (is_ascii_alpha(c)) {
                tokenizer->pending_token = get_new_end_tag_token();
                tokenizer->has_pending_token = true;
                return reconsume_in(tokenizer, TAG_NAME_STATE);
            }
            tokenizer->last_error = INVALID_FIRST_CHARACTER_OF_TAG_NAME;
            tokenizer->pending_token = get_new_comment_token();
            tokenizer->has_pending_token = true;
            return reconsume_in(tokenizer, BOGUS_COMMENT_STATE);
        }
    }
}

token_result_t handle_tag_name_state(tokenizer_t* tokenizer) {
    assert(tokenizer->has_pending_token == true &&
            (tokenizer->pending_token.type == START_TAG ||
            tokenizer->pending_token.type == END_TAG));

    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            tokenizer->data_state = BEFORE_ATTRIBUTE_NAME_STATE;
            return TOKEN_OK;
        }
        case SOLIDUS: {
            tokenizer->data_state = SELF_CLOSING_START_TAG_STATE;
            return TOKEN_OK;
        }
        case '>': {
            tokenizer->data_state = DATA_STATE;
            return emit_pending_token(tokenizer);
        }
        case NULL_CHARACTER: {
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            if (parser_string_append_char(&tokenizer->pending_token.data.tag.name, REPLACEMENT_CHARACTER) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        case EOF: {
            const token_t token = get_eof_token();
            return emit_token(tokenizer, &token);
        }
        default: {
            if (parser_string_append_char(&tokenizer->pending_token.data.tag.name, c | 0x0020) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
    }
}

token_result_t handle_rcdata_less_than_sign_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    if (c == SOLIDUS) {
        parser_string_clear(&tokenizer->temporary_buffer);
        tokenizer->data_state = RCDATA_END_TAG_OPEN_STATE;
        return TOKEN_OK;
    }
    /// Default case
    const token_t token = get_character_token(LESS_THAN_SIGN);
    if (emit_token(tokenizer, &token) != TOKEN_OK)
        return TOKEN_ERROR;
    return reconsume_in(tokenizer, RCDATA_STATE);
}

token_result_t handle_rcdata_end_tag_open_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    if (is_ascii_alpha(c)) {
        tokenizer->pending_token = get_new_end_tag_token();
        tokenizer->has_pending_token = true;
        return reconsume_in(tokenizer, RCDATA_END_TAG_NAME_STATE);
    }
    const token_t token = get_character_token(LESS_THAN_SIGN);
    if (emit_token(tokenizer, &token) != TOKEN_OK)
        return TOKEN_ERROR;
    return reconsume_in(tokenizer, RCDATA_STATE);
}

token_result_t handle_rcdata_end_tag_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == END_TAG);
            if (parser_strcmp(&tokenizer->last_start_tag_name,
                &tokenizer->pending_token.data.tag.name) == 0) {
                tokenizer->data_state = BEFORE_ATTRIBUTE_NAME_STATE;
                return TOKEN_OK;
            }
            goto anything_else;
        }
        case SOLIDUS: {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == END_TAG);
            if (parser_strcmp(&tokenizer->last_start_tag_name,
                &tokenizer->pending_token.data.tag.name) == 0) {
                tokenizer->data_state = SELF_CLOSING_START_TAG_STATE;
                return TOKEN_OK;
                }
            goto anything_else;
        }
        case '>': {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == END_TAG);
            if (parser_strcmp(&tokenizer->last_start_tag_name,
                &tokenizer->pending_token.data.tag.name) == 0) {
                tokenizer->data_state = DATA_STATE;
                return emit_pending_token(tokenizer);;
                }
            goto anything_else;
        }
        default: {
            if (is_ascii_alpha(c)) {
                if (parser_string_append_char(&tokenizer->pending_token.data.tag.name, c | 0x0020) != 0 ||
                    parser_string_append_char(&tokenizer->temporary_buffer, c) != 0)
                    return TOKEN_ERROR;
                return TOKEN_OK;
            }
            anything_else: {
                const token_t less_than_sign_token = get_character_token(LESS_THAN_SIGN);
                const token_t solidus_token = get_character_token(SOLIDUS);
                if (emit_token(tokenizer, &less_than_sign_token) != TOKEN_OK ||
                    emit_token(tokenizer, &solidus_token) != TOKEN_OK)
                    return TOKEN_ERROR;
                /// Emit a character token for each character in temporary buffer
                for (size_t i = 0; i < tokenizer->temporary_buffer.length; i++) {
                    const token_t token = get_character_token(tokenizer->temporary_buffer.data[i]);
                    if (emit_token(tokenizer, &token) != TOKEN_OK)
                        return TOKEN_ERROR;
                }
                return reconsume_in(tokenizer, RCDATA_STATE);
            }
        }
    }
}

token_result_t handle_before_attribute_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            return TOKEN_OK;
        }
        case SOLIDUS:
        case GREATER_THAN_SIGN:
        case EOF: {
            return reconsume_in(tokenizer, AFTER_ATTRIBUTE_NAME_STATE);
        }
        case '=': {
            tokenizer->last_error = UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME;
            attribute_list_t* attr_list = &tokenizer->pending_token.data.tag.attributes;

            attribute_t attr = (attribute_t){0};
            if (parser_string_append_char(&attr.name, c) != 0)
                return TOKEN_ERROR;
            if (attribute_list_append(attr_list, &attr) != 0)
                return TOKEN_ERROR;
            tokenizer->data_state = ATTRIBUTE_NAME_STATE;
            return TOKEN_OK;
        }
        default: {
            attribute_list_t* attr_list = &tokenizer->pending_token.data.tag.attributes;

            const attribute_t attr = (attribute_t){0};
            if (attribute_list_append(attr_list, &attr) != 0)
                return TOKEN_ERROR;
            return reconsume_in(tokenizer, ATTRIBUTE_NAME_STATE);
        }
    }
}

token_result_t handle_attribute_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE:
        case SOLIDUS:
        case GREATER_THAN_SIGN:
        case EOF: {
            return reconsume_in(tokenizer, AFTER_ATTRIBUTE_NAME_STATE);
        }
        case '=': {
            tokenizer->data_state = BEFORE_ATTRIBUTE_VALUE_STATE;
            return TOKEN_OK;
        }
        case NULL_CHARACTER: {
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            const attribute_list_t* attributes = &tokenizer->pending_token.data.tag.attributes;
            const int res = parser_string_append_char(&attributes->items[attributes->count].name, REPLACEMENT_CHARACTER);
            if (res != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        case QUOTATION_MARK:
        case APOSTROPHE:
        case LESS_THAN_SIGN: {
            tokenizer->last_error = UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME;
            // FALLTHROUGH
        }
        default: {
            attribute_list_t* attr_list = &tokenizer->pending_token.data.tag.attributes;
            assert(attr_list->items != NULL);

            attribute_t* attr = get_current_attribute(attr_list);
            // if (parser_string_append_char(&attr_list->items[attr_list->count].name, c) != 0)
            if (parser_string_append_char(&attr->name, c) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
    }
}

token_result_t handle_after_attribute_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            return TOKEN_OK;
        }
        case SOLIDUS: {
            tokenizer->data_state = SELF_CLOSING_START_TAG_STATE;
            return TOKEN_OK;
        }
        case EQUALS_SIGN: {
            tokenizer->data_state = BEFORE_ATTRIBUTE_VALUE_STATE;
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == START_TAG);
            tokenizer->data_state = DATA_STATE;
            return emit_pending_token(tokenizer);
        }
        case EOF: {
            tokenizer->last_error = EOF_IN_TAG;
            const token_t eof_token = get_eof_token();
            return emit_token(tokenizer, &eof_token);
        }
        default: {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == START_TAG);
            attribute_list_t* attr_list = &tokenizer->pending_token.data.tag.attributes;

            const attribute_t attr = (attribute_t){0};
            if (attribute_list_append(attr_list, &attr) != 0)
                return TOKEN_ERROR;
            return reconsume_in(tokenizer, ATTRIBUTE_NAME_STATE);
        }
    }
}

token_result_t handle_before_attribute_value_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            return TOKEN_OK;
        }
        case QUOTATION_MARK: {
            tokenizer->data_state = ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE;
            return TOKEN_OK;
        }
        case APOSTROPHE: {
            tokenizer->data_state = ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE;
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            assert(tokenizer->has_pending_token &&
                (tokenizer->pending_token.type == START_TAG ||
                 tokenizer->pending_token.type == END_TAG));
            tokenizer->last_error = MISSING_ATTRIBUTE_VALUE;
            tokenizer->data_state = DATA_STATE;
            return emit_pending_token(tokenizer);
        }
        default: {
            return reconsume_in(tokenizer, ATTRIBUTE_VALUE_UNQUOTED_STATE);
        }
    }
}

token_result_t handle_attribute_value_quoted_state(tokenizer_t* tokenizer, const int quote_type) {
    assert(quote_type == QUOTATION_MARK ||
        quote_type == APOSTROPHE);
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case AMPERSAND: {
            if (quote_type == QUOTATION_MARK)
                tokenizer->return_state = ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE;
            if (quote_type == APOSTROPHE)
                tokenizer->return_state = ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE;
            tokenizer->data_state = CHARACTER_REFERENCE_STATE;
            return TOKEN_OK;
        }
        case NULL_CHARACTER: {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == START_TAG);
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            if (append_to_current_attr(&tokenizer->pending_token, REPLACEMENT_CHARACTER) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        case EOF: {
            tokenizer->last_error = EOF_IN_TAG;
            const token_t token = get_eof_token();
            if (emit_token(tokenizer, &token) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            if (quote_type == QUOTATION_MARK) {
                tokenizer->data_state = AFTER_ATTRIBUTE_VALUE_QUOTED_STATE;
                return TOKEN_OK;
            }
            if (quote_type == APOSTROPHE) {
                tokenizer->data_state = AFTER_ATTRIBUTE_VALUE_QUOTED_STATE;
                return TOKEN_OK;
            }
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == START_TAG);
            if (append_to_current_attr(&tokenizer->pending_token, REPLACEMENT_CHARACTER) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
    }
}

token_result_t handle_attribute_value_double_quoted_state(tokenizer_t* tokenizer) {
    return handle_attribute_value_quoted_state(tokenizer, QUOTATION_MARK);
}

token_result_t handle_attribute_value_single_quoted_state(tokenizer_t* tokenizer) {
    return handle_attribute_value_quoted_state(tokenizer, APOSTROPHE);
}

token_result_t handle_attribute_value_unquoted_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            tokenizer->data_state = BEFORE_ATTRIBUTE_NAME_STATE;
            return TOKEN_OK;
        }
        case AMPERSAND: {
            tokenizer->return_state = ATTRIBUTE_VALUE_UNQUOTED_STATE;
            tokenizer->data_state = CHARACTER_REFERENCE_STATE;
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == START_TAG);
            tokenizer->return_state = DATA_STATE;
            return emit_pending_token(tokenizer);
        }
        case NULL_CHARACTER: {
            assert(tokenizer->has_pending_token &&
                tokenizer->pending_token.type == START_TAG);
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            if (append_to_current_attr(&tokenizer->pending_token, REPLACEMENT_CHARACTER) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        case QUOTATION_MARK:
        case APOSTROPHE:
        case LESS_THAN_SIGN:
        case EQUALS_SIGN:
        case GRAVE_ACCENT: {
            tokenizer->last_error = UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE;
            break;
            // anything else case
        }
        case EOF: {
            tokenizer->last_error = EOF_IN_TAG;
            const token_t token = get_eof_token();
            if (emit_token(tokenizer, &token) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            break;
            // anything else case
        }
    }

    // anything else
    if (append_to_current_attr(&tokenizer->pending_token, c) != 0)
        return TOKEN_ERROR;
    return TOKEN_OK;
}

token_result_t handle_after_attribute_value_quoted_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            tokenizer->data_state = BEFORE_ATTRIBUTE_NAME_STATE;
            return TOKEN_OK;
        }
        case SOLIDUS: {
            tokenizer->data_state = SELF_CLOSING_START_TAG_STATE;
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            tokenizer->data_state = DATA_STATE;
            return emit_pending_token(tokenizer);
        }
        case EOF: {
            tokenizer->last_error = EOF_IN_TAG;
            const token_t token = get_eof_token();
            if (emit_token(tokenizer, &token) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            tokenizer->last_error = MISSING_WHITESPACE_BETWEEN_ATTRIBUTES;
            return reconsume_in(tokenizer, BEFORE_ATTRIBUTE_NAME_STATE);
        }
    }
}

token_result_t handle_markup_declaration_open_state(tokenizer_t* tokenizer) {
    if (!tokenizer->consume_flag)
        tokenizer->peek_buffer[tokenizer->peek_count++] = tokenizer->current_char;

    if (try_match_peek_buf(tokenizer, "--")) {
        consume_peek_buf(tokenizer);
        tokenizer->pending_token = get_new_comment_token();
        tokenizer->has_pending_token = true;
        tokenizer->data_state = COMMENT_START_STATE;
        return TOKEN_OK;
    }

    if (try_match_peek_buf(tokenizer, "DOCTYPE")) {
        consume_peek_buf(tokenizer);
        tokenizer->data_state = DOCTYPE_STATE;
        return TOKEN_OK;
    }

    if (try_match_peek_buf(tokenizer, "[CDATA[")) {
        consume_peek_buf(tokenizer);
        tokenizer->last_error = CDATA_IN_HTML_CONTENT;
        tokenizer->pending_token = get_new_comment_token();
        tokenizer->has_pending_token = true;
        parser_string_init_cstr(&tokenizer->pending_token.data.comment.text, "[CDATA[");
        tokenizer->data_state = BOGUS_COMMENT_STATE;
        return TOKEN_OK;
    }

    tokenizer->last_error = INCORRECTLY_OPENED_COMMENT;
    tokenizer->pending_token = get_new_comment_token();
    tokenizer->has_pending_token = true;
    tokenizer->data_state = BOGUS_COMMENT_STATE;
    return TOKEN_OK;
}

token_result_t handle_doctype_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            tokenizer->data_state = BEFORE_DOCTYPE_NAME_STATE;
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            return reconsume_in(tokenizer, BEFORE_DOCTYPE_NAME_STATE);
        }
        case EOF: {
            tokenizer->last_error = EOF_IN_DOCTYPE;
            token_t doctype_token = get_new_doctype_token();
            doctype_token.data.doctype.force_quirks = true;
            const token_t eof_token = get_eof_token();
            if (emit_token(tokenizer, &doctype_token) != TOKEN_OK ||
                emit_token(tokenizer, &eof_token) != TOKEN_OK)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            tokenizer->last_error = MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME;
            return reconsume_in(tokenizer, BEFORE_DOCTYPE_NAME_STATE);
        }
    }
}

token_result_t handle_before_doctype_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            return TOKEN_OK;
        }
        case NULL_CHARACTER: {
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            token_t doctype_token = get_new_doctype_token();
            if (parser_string_append_char(&doctype_token.data.doctype.name, REPLACEMENT_CHARACTER) != 0)
                // should delete token?
                return TOKEN_ERROR;
            tokenizer->data_state = DOCTYPE_NAME_STATE;
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            tokenizer->last_error = MISSING_DOCTYPE_NAME;
            token_t doctype_token = get_new_doctype_token();
            doctype_token.data.doctype.force_quirks = true;
            if (emit_token(tokenizer, &doctype_token) != TOKEN_OK)
                return TOKEN_ERROR;
            tokenizer->data_state = DATA_STATE;
            return TOKEN_OK;
        }
        default: {
            tokenizer->pending_token = get_new_doctype_token();
            tokenizer->has_pending_token = true;
            doctype_token* token_data = &tokenizer->pending_token.data.doctype;
            // append lowercase version of c
            if (parser_string_append_char(&token_data->name, c | 0x0020) != 0)
                return TOKEN_ERROR;
            tokenizer->data_state = DOCTYPE_NAME_STATE;
            return TOKEN_OK;
        }
    }
}

token_result_t handle_doctype_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            tokenizer->data_state = AFTER_DOCTYPE_NAME_STATE;
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            assert(tokenizer->has_pending_token);
            tokenizer->data_state = DATA_STATE;
            return emit_pending_token(tokenizer);
        }
        case NULL_CHARACTER: {
            assert(tokenizer->has_pending_token && tokenizer->pending_token.type == DOCTYPE);
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            if (parser_string_append_char(&tokenizer->pending_token.data.doctype.name, REPLACEMENT_CHARACTER) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        case EOF: {
            assert(tokenizer->has_pending_token && tokenizer->pending_token.type == DOCTYPE);
            tokenizer->last_error = EOF_IN_DOCTYPE;
            tokenizer->pending_token.data.doctype.force_quirks = true;
            const token_t eof_token = get_eof_token();
            if (emit_pending_token(tokenizer) != TOKEN_OK ||
                emit_token(tokenizer, &eof_token) != TOKEN_OK)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            assert(tokenizer->has_pending_token && tokenizer->pending_token.type == DOCTYPE);
            if (parser_string_append_char(&tokenizer->pending_token.data.doctype.name, c | 0x0020) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
    }
}

token_result_t handle_after_doctype_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            return TOKEN_OK;
        }
        case GREATER_THAN_SIGN: {
            assert(tokenizer->has_pending_token && tokenizer->pending_token.type == DOCTYPE);
            tokenizer->data_state = DATA_STATE;
            return emit_pending_token(tokenizer);
        }
        case EOF: {
            assert(tokenizer->has_pending_token && tokenizer->pending_token.type == DOCTYPE);
            tokenizer->last_error = EOF_IN_DOCTYPE;
            tokenizer->pending_token.data.doctype.force_quirks = true;
            const token_t eof_token = get_eof_token();
            if (emit_pending_token(tokenizer) != TOKEN_OK ||
                emit_token(tokenizer, &eof_token) != TOKEN_OK)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            if (tokenizer->peek_count + 1 > PEEK_BUFFER_LEN)
                return TOKEN_ERROR;
            tokenizer->peek_buffer[tokenizer->peek_count++] = c;
            if (try_match_peek_buf(tokenizer, "PUBLIC")) {
                consume_peek_buf(tokenizer);
                tokenizer->data_state = AFTER_DOCTYPE_PUBLIC_KEYWORD_STATE;
                return TOKEN_OK;
            }
            if (try_match_peek_buf(tokenizer, "SYSTEM")) {
                consume_peek_buf(tokenizer);
                tokenizer->data_state = AFTER_DOCTYPE_SYSTEM_KEYWORD_STATE;
                return TOKEN_OK;
            }
            assert(tokenizer->has_pending_token && tokenizer->pending_token.type == DOCTYPE);
            tokenizer->last_error = INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME;
            tokenizer->pending_token.data.doctype.force_quirks = true;
            tokenizer->consume_flag = false;
            tokenizer->data_state = BOGUS_DOCTYPE_STATE;
            return TOKEN_OK;
        }
    }
}

int token_next(tokenizer_t* tokenizer) {
    typedef token_result_t (*handler_t)(tokenizer_t* tokenizer);

    static handler_t handlers[DATA_STATE_COUNT] = {
        [DATA_STATE] = handle_data_state,
        [RCDATA_STATE] = handle_rcdata_state,
        [RAWTEXT_STATE] = handle_rawtext_state,
        [SCRIPT_DATA_STATE] = handle_script_data_state,
        [PLAINTEXT_STATE] = handle_plaintext_state,
        [TAG_OPEN_STATE] = handle_tag_open_state,
        [END_TAG_OPEN_STATE] = handle_end_tag_open_state,
        [TAG_NAME_STATE] = handle_tag_name_state,
        [RCDATA_LESS_THAN_SIGN_STATE] = handle_rcdata_less_than_sign_state,
        [RCDATA_END_TAG_OPEN_STATE] = handle_rcdata_end_tag_open_state,
        [RCDATA_END_TAG_NAME_STATE] = handle_rcdata_end_tag_name_state,
        [RAWTEXT_LESS_THAN_SIGN_STATE] = NULL,
        [RAWTEXT_END_TAG_OPEN_STATE] = NULL,
        [RAWTEXT_END_TAG_NAME_STATE] = NULL,
        [SCRIPT_DATA_LESS_THAN_SIGN_STATE] = NULL,
        [SCRIPT_DATA_END_TAG_OPEN_STATE] = NULL,
        [SCRIPT_DATA_END_TAG_NAME_STATE] = NULL,
        [SCRIPT_DATA_ESCAPE_START_STATE] = NULL,
        [SCRIPT_DATA_ESCAPE_START_DASH_STATE] = NULL,
        [SCRIPT_DATA_ESCAPED_STATE] = NULL,
        [SCRIPT_DATA_ESCAPED_DASH_STATE] = NULL,
        [SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN_STATE] = NULL,
        [SCRIPT_DATA_ESCAPED_END_TAG_OPEN_STATE] = NULL,
        [SCRIPT_DATA_ESCAPED_END_TAG_NAME_STATE] = NULL,
        [SCRIPT_DATA_DOUBLE_ESCAPE_START_STATE] = NULL,
        [SCRIPT_DATA_DOUBLE_ESCAPED_STATE] = NULL,
        [SCRIPT_DATA_DOUBLE_ESCAPED_DASH_STATE] = NULL,
        [SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH_STATE] = NULL,
        [SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN_STATE] = NULL,
        [SCRIPT_DATA_DOUBLE_ESCAPE_END_STATE] = NULL,
        [BEFORE_ATTRIBUTE_NAME_STATE] = handle_before_attribute_name_state,
        [ATTRIBUTE_NAME_STATE] = handle_attribute_name_state,
        [AFTER_ATTRIBUTE_NAME_STATE] = handle_after_attribute_name_state,
        [BEFORE_ATTRIBUTE_VALUE_STATE] = handle_before_attribute_value_state,
        [ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE] = handle_attribute_value_double_quoted_state,
        [ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE] = handle_attribute_value_single_quoted_state,
        [ATTRIBUTE_VALUE_UNQUOTED_STATE] = handle_attribute_value_unquoted_state,
        [AFTER_ATTRIBUTE_VALUE_QUOTED_STATE] = handle_after_attribute_value_quoted_state,
        [SELF_CLOSING_START_TAG_STATE] = NULL,
        [BOGUS_COMMENT_STATE] = NULL,
        [MARKUP_DECLARATION_OPEN_STATE] = handle_markup_declaration_open_state,
        [COMMENT_START_STATE] = NULL,
        [COMMENT_START_DASH_STATE] = NULL,
        [COMMENT_STATE] = NULL,
        [COMMENT_LESS_THAN_SIGN_STATE] = NULL,
        [COMMENT_LESS_THAN_SIGN_BANG_STATE] = NULL,
        [COMMENT_LESS_THAN_SIGN_BANG_DASH_STATE] = NULL,
        [COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH_STATE] = NULL,
        [COMMENT_END_DASH_STATE] = NULL,
        [COMMENT_END_STATE] = NULL,
        [COMMENT_END_BANG_STATE] = NULL,
        [DOCTYPE_STATE] = handle_doctype_state,
        [BEFORE_DOCTYPE_NAME_STATE] = handle_before_doctype_name_state,
        [DOCTYPE_NAME_STATE] = handle_doctype_name_state,
        [AFTER_DOCTYPE_NAME_STATE] = handle_after_doctype_name_state
    };
    token_result_t result = TOKEN_OK;
    do {
        assert(tokenizer->data_state < DATA_STATE_COUNT);
        const handler_t handler = handlers[tokenizer->data_state];
        if (!handler) {
            printf("Unknown or unhandled tokenizer state: %s\n",
                data_state_to_string(tokenizer->data_state));
            result = TOKEN_ERROR;
            break;
        }
        result = handler(tokenizer);
        if (tokenizer->last_error != NO_ERROR) {
            fprintf(stderr, "Encountered parse error at character %c (%zu): %s\n",
                tokenizer->current_char,
                tokenizer->chars_consumed,
                parse_error_to_string(tokenizer->last_error));
            tokenizer->last_error = NO_ERROR;
        }
    }
    while (result == TOKEN_RECONSUME);
    return result;
}
