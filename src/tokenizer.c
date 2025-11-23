#include "tokenizer.h"

#include <assert.h>

#include "misc.h"

void queue_init(token_queue_t* queue) {
    queue->start = 0;
    queue->end = 0;
    queue->count = 0;
}

int queue_pop(token_queue_t* queue, token_t* out) {
    if (!queue || !out || queue_is_empty(queue))
        return 1;

    // caller takes ownership of the token and must delete it later
    *out = queue->tokens[queue->end];
    queue->end = (--queue->end) % TOKEN_QUEUE_SIZE;
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

bool is_void_element(const token_t token) {
    static const char* void_tags[] = {
        "area",
        "base",
        "br",
        "col",
        "embed",
        "hr",
        "img",
        "input",
        "link",
        "meta",
        "source",
        "track",
        "wbr"
    };
    static const size_t arr_size = sizeof(void_tags) / sizeof(void_tags[0]);

    return contains(void_tags, arr_size, &token.data.tag.name);
}

bool is_template_element(const token_t token) {
    static const char* template_tag = "template";

    return parser_cstrcmp(template_tag, &token.data.tag.name);
}

bool is_raw_text_element(const token_t token) {
    static const char* raw_text_tags[] = {
        "script",
        "style"
    };
    static const size_t arr_size = sizeof(raw_text_tags) / sizeof(raw_text_tags[0]);

    return contains(raw_text_tags, arr_size, &token.data.tag.name);
}

bool is_escapable_raw_text_element(const token_t token) {
    static const char* escapable_raw_text_tags[] = {
        "textarea",
        "title"
    };
    static const size_t arr_size = sizeof(escapable_raw_text_tags) / sizeof(escapable_raw_text_tags[0]);

    return contains(escapable_raw_text_tags, arr_size, &token.data.tag.name);
}

bool is_foreign_element(token_t token) {
    // TODO make this work
    return false;
}

bool is_normal_element(token_t token) {
    // TODO and this
    return false;
}

void consume_character(tokenizer_t* tokenizer) {
    tokenizer->current_char = fgetc(tokenizer->stream);
    tokenizer->chars_consumed++;
}

void reconsume_character(tokenizer_t* tokenizer) {
    tokenizer->consume_flag = true;
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
        .type = EOF
    };
}

int get_new_start_tag_token(token_t* out) {
    out->type = START_TAG;
    if (parser_string_init(&out->data.tag.name) != 0)
        return 1;
    out->data.tag.attributes = (attribute_list_t){0};

    return 0;
}

int get_new_end_tag_token(token_t* out) {
    out->type = END_TAG;
    if (parser_string_init(&out->data.tag.name) != 0)
        return 1;
    out->data.tag.attributes = (attribute_list_t){0};

    return 0;
}

int get_new_comment_token(token_t* out) {
    out->type = COMMENT;
    if (parser_string_init(&out->data.comment.text) != 0)
        return 1;

    return 0;
}

token_result_t push_token(const tokenizer_t* tokenizer, const token_t* token) {
    if (queue_push(tokenizer->token_queue, token) != 0 )
        return TOKEN_ERROR;
    return TOKEN_OK;
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
            return push_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            if (queue_push(tokenizer->token_queue, &token) != 0 )
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            const token_t token =  get_character_token(c);
            return push_token(tokenizer, &token);
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
            return push_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            return push_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return push_token(tokenizer, &token);
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
            return push_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            return push_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return push_token(tokenizer, &token);
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
            return push_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return push_token(tokenizer, &token);
        }
    }
}

token_result_t handle_plaintext_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case NULL_CHARACTER: {
            const token_t token = get_character_token(REPLACEMENT_CHARACTER);
            return push_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            return push_token(tokenizer, &token);
        }
        default: {
            const token_t token = get_character_token(c);
            return push_token(tokenizer, &token);
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
            if (get_new_comment_token(&tokenizer->pending_token) != 0) {
                fprintf(stderr, "Error creating new comment token\n");
                return TOKEN_ERROR;
            }
            tokenizer->has_pending_token = true;
            tokenizer->data_state = BOGUS_COMMENT_STATE;
            tokenizer->consume_flag = false;
            return TOKEN_RECONSUME;
        }
        case EOF: {
            tokenizer->last_error = EOF_BEFORE_TAG_NAME;
            const token_t less_than_sign_token = get_character_token(LESS_THAN_SIGN);
            const token_t eof_token = get_eof_token();
            if (push_token(tokenizer, &less_than_sign_token) != TOKEN_OK ||
                push_token(tokenizer, &eof_token) != TOKEN_OK)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            if (is_ascii_alpha(c)) {
                if (get_new_start_tag_token(&tokenizer->pending_token) != 0) {
                    fprintf(stderr, "Error creating new start tag token\n");
                    return TOKEN_ERROR;
                }
                tokenizer->has_pending_token = true;
                tokenizer->data_state = TAG_NAME_STATE;
                tokenizer->consume_flag = false;
                return TOKEN_RECONSUME;
            }
            tokenizer->last_error = INVALID_FIRST_CHARACTER_OF_TAG_NAME;
            const token_t token = get_character_token(LESS_THAN_SIGN);
            if (push_token(tokenizer, &token) == TOKEN_ERROR)
                return TOKEN_ERROR;
            tokenizer->data_state = DATA_STATE;
            tokenizer->consume_flag = false;
            return TOKEN_RECONSUME;
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
            if (push_token(tokenizer, &less_than_sign_token) != TOKEN_OK ||
                push_token(tokenizer, &solidus_token) != TOKEN_OK ||
                push_token(tokenizer, &eof_token) != TOKEN_OK)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        default: {
            if (is_ascii_alpha(c)) {
                if (get_new_end_tag_token(&tokenizer->pending_token) != 0) {
                    fprintf(stderr, "Error creating new end tag token\n");
                    return TOKEN_ERROR;
                }
                tokenizer->has_pending_token = true;
                tokenizer->data_state = TAG_NAME_STATE;
                tokenizer->consume_flag = false;
                return TOKEN_RECONSUME;
            }
            tokenizer->last_error = INVALID_FIRST_CHARACTER_OF_TAG_NAME;
            if (get_new_comment_token(&tokenizer->pending_token) != 0) {
                fprintf(stderr, "Error creating new comment token\n");
                return TOKEN_ERROR;
            }
            tokenizer->has_pending_token = true;
            tokenizer->data_state = BOGUS_COMMENT_STATE;
            tokenizer->consume_flag = false;
            return TOKEN_RECONSUME;
        }
    }
}

token_result_t handle_tag_name_state(tokenizer_t* tokenizer) {
    assert( tokenizer->has_pending_token == true &&
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
            if (push_token(tokenizer, &tokenizer->pending_token) != TOKEN_OK)
                return TOKEN_ERROR;
            tokenizer->has_pending_token = false;
            return TOKEN_OK;
        }
        case NULL_CHARACTER: {
            tokenizer->last_error = UNEXPECTED_NULL_CHARACTER;
            if (parser_string_append_char(&tokenizer->pending_token.data.tag.name, REPLACEMENT_CHARACTER) != 0)
                return TOKEN_ERROR;
            return TOKEN_OK;
        }
        case EOF: {
            const token_t token = get_eof_token();
            return push_token(tokenizer, &token);
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
    if (push_token(tokenizer, &token) != TOKEN_OK)
        return TOKEN_ERROR;
    tokenizer->data_state = RCDATA_STATE;
    tokenizer->consume_flag = false;
    return TOKEN_RECONSUME;
}

token_result_t handle_rcdata_end_tag_open_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    if (is_ascii_alpha(c)) {
        if (get_new_end_tag_token(&tokenizer->pending_token) != 0)
            return TOKEN_ERROR;
        tokenizer->has_pending_token = true;
        tokenizer->data_state = RCDATA_END_TAG_NAME_STATE;
        tokenizer->consume_flag = false;
        return TOKEN_RECONSUME;
    }
    const token_t token = get_character_token(LESS_THAN_SIGN);
    if (push_token(tokenizer, &token) != TOKEN_OK)
        return TOKEN_ERROR;
    tokenizer->data_state = RCDATA_STATE;
    tokenizer->consume_flag = false;
    return TOKEN_RECONSUME;
}

token_result_t handle_rcdata_end_tag_name_state(tokenizer_t* tokenizer) {
    handle_consume_flag(tokenizer);
    const int c = tokenizer->current_char;
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case SPACE: {
            // TODO check if end tag token is appropriate
            tokenizer->data_state = BEFORE_ATTRIBUTE_NAME_STATE;
            return TOKEN_OK;
            goto anything_else;
        }
        case SOLIDUS: {
            // TODO check if end tag token is appropriate
            tokenizer->data_state = SELF_CLOSING_START_TAG_STATE;
            goto anything_else;
        }
        case '>': {
            // check if end tag token is appropriate
            tokenizer->data_state = DATA_STATE;
            if (push_token(tokenizer, &tokenizer->pending_token) != TOKEN_OK)
                return TOKEN_ERROR;
            tokenizer->has_pending_token = false;
            return TOKEN_OK;
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
                if (push_token(tokenizer, &less_than_sign_token) != TOKEN_OK ||
                    push_token(tokenizer, &solidus_token) != TOKEN_OK)
                    return TOKEN_ERROR;
                /// Emit a character token for each character in temporary buffer
                for (size_t i = 0; i < tokenizer->temporary_buffer.length; i++) {
                    const token_t token = get_character_token(tokenizer->temporary_buffer.data[i]);
                    if (push_token(tokenizer, &token) != TOKEN_OK)
                        return TOKEN_ERROR;
                }
                tokenizer->data_state = RCDATA_STATE;
                tokenizer->consume_flag = false;
                return TOKEN_RECONSUME;
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
            tokenizer->data_state = AFTER_ATTRIBUTE_NAME_STATE;
            tokenizer->consume_flag = false;
            return TOKEN_RECONSUME;
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
            tokenizer->data_state = ATTRIBUTE_NAME_STATE;
            tokenizer->consume_flag = false;
            return TOKEN_RECONSUME;
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
            tokenizer->data_state = AFTER_ATTRIBUTE_NAME_STATE;
            tokenizer->consume_flag = false;
            return TOKEN_RECONSUME;
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
            const attribute_list_t* attributes = &tokenizer->pending_token.data.tag.attributes;
            assert(attributes->items != NULL);

            if (parser_string_append_char(&attributes->items[attributes->count].name, c) != 0)
                return TOKEN_ERROR;
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
        [BEFORE_ATTRIBUTE_VALUE_STATE] = handle_before_attribute_name_state,
        [ATTRIBUTE_NAME_STATE] = handle_attribute_name_state,
        [AFTER_ATTRIBUTE_NAME_STATE] = NULL,
    };
    token_result_t result = TOKEN_OK;
    do {
        assert(tokenizer->data_state < DATA_STATE_COUNT);
        const handler_t handler = handlers[tokenizer->data_state];
        if (!handler) {
            printf("Unknown or unhandled tokenizer state");
            result = TOKEN_ERROR;
            break;
        }
        result = handler(tokenizer);
        if (tokenizer->last_error != NO_ERROR) {
            fprintf(stderr, "Encountered parse error at character %c (%zu): %s",
                tokenizer->current_char,
                tokenizer->chars_consumed,
                parse_error_to_string(tokenizer->last_error));
            tokenizer->last_error = NO_ERROR;
        }
    }
    while (result == TOKEN_RECONSUME);
    return result;
}