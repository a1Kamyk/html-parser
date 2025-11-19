#include "tokenizer.h"

#include <assert.h>

#include "misc.h"

#define NULL_CHARACTER          0x0000
#define CHARACTER_TABULATION    0x0009
#define LINE_FEED               0x000A
#define FORM_FEED               0x000C
#define SPACE                   0x0020

#define REPLACEMENT_CHARACTER   0xFFFD
#define SOLIDUS                 0x002F
#define LESS_THAN_SIGN          0x003C
#define GREATER_THAN_SIGN       0x003E

// TODO move these to tree construction stage
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

void queue_init(internal_token_queue_t* queue) {
    queue->start = 0;
    queue->end = 0;
    queue->count = 0;
}

int queue_pop(internal_token_queue_t* queue, const token_t* out) {
    if (queue_is_empty(queue)) return 1;

    out = &queue->tokens[queue->end];
    queue->end = (++queue->end) % INTERNAL_QUEUE_SIZE;
    queue->count--;
    return 0;
}

int queue_push(internal_token_queue_t* queue, const token_t* token) {
    if (queue_is_full(queue) || !token) return 1;

    queue->tokens[queue->end] = *token;
    queue->end = (++queue->end) % INTERNAL_QUEUE_SIZE;
    queue->count++;
    return 0;
}

bool queue_is_empty(const internal_token_queue_t* queue) {
    return queue->count == 0;
}

bool queue_is_full(const internal_token_queue_t* queue) {
    return queue->count == INTERNAL_QUEUE_SIZE;
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
    out->data.tag.attr_count = 0;
    out->data.tag.attributes = NULL;

    return 0;
}

int get_new_end_tag_token(token_t* out) {
    out->type = END_TAG;
    if (parser_string_init(&out->data.tag.name) != 0)
        return 1;
    out->data.tag.attr_count = 0;
    out->data.tag.attributes = NULL;

    return 0;
}

int get_new_comment_token(token_t* out) {
    out->type = COMMENT;
    if (parser_string_init(&out->data.comment.data) != 0)
        return 1;

    return 0;
}

void destroy_token(token_t* token) {
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
            for (size_t i = 0; i < token_data->attr_count; i++) {
                attribute_t* attribute = &token_data->attributes[i];
                parser_string_delete(&attribute->name);
                parser_string_delete(&attribute->value);
            }
            free(token_data->attributes);
            return;
        }
        case COMMENT: {
            comment_token* token_data = &token->data.comment;
            parser_string_delete(&token_data->data);
            return;
        }
        case CHARACTER:
        case END_OF_FILE: {
            return;
        }
        default: {
            printf("Unknown token type");
        }
    }
}

token_result_t push_token(tokenizer_t* tokenizer, const token_t* token) {
    if (queue_push(&tokenizer->internal_token_queue, token) != 0 )
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
            // TODO `unexpected-null-character parse error`
            const token_t token = get_character_token(0);
            return push_token(tokenizer, &token);
        }
        case EOF: {
            const token_t token = get_eof_token();
            if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
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
            // TODO `unexpected-null-character` parse error
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
            // TODO `unexpected-null-character` parse error
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
            if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
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
            // TODO `unexpected-question-mark-instead-of-tag-name` parse error
            if (get_new_comment_token(&tokenizer->pending_token) != 0) {
                fprintf(stderr, "Error creating new comment token");
                return TOKEN_ERROR;
            }
            tokenizer->has_pending_token = true;
            tokenizer->data_state = BOGUS_COMMENT_STATE;
            tokenizer->consume_flag = false;
            return TOKEN_RECONSUME;
        }
        case EOF: {
            // TODO `eof-before-tag-name` parse error
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
                    fprintf(stderr, "Error creating new start tag token");
                    return TOKEN_ERROR;
                }
                tokenizer->has_pending_token = true;
                tokenizer->data_state = TAG_NAME_STATE;
                tokenizer->consume_flag = false;
                return TOKEN_RECONSUME;
            }
            // TODO `invalid-first-character-of-tag-name` parse error
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
            // TODO `missing-end-tag-name` parse error
            tokenizer->data_state = DATA_STATE;
            return TOKEN_OK;
        }
        case EOF: {
            // TODO `eof-before-tag-name` parse error
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
                    fprintf(stderr, "Error creating new end tag token");
                    return TOKEN_ERROR;
                }
                tokenizer->has_pending_token = true;
                tokenizer->data_state = TAG_NAME_STATE;
                tokenizer->consume_flag = false;
                return TOKEN_RECONSUME;
            }
            // TODO `invalid-first-character-of-tag-name` parse error
            if (get_new_comment_token(&tokenizer->pending_token) != 0) {
                fprintf(stderr, "Error creating new comment token");
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
            // TODO `unexpected-null-character` parse error
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
            // check if end tag tkoen is appropriate
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

// TODO proper error naming
int token_next(tokenizer_t* tokenizer) {
    typedef token_result_t (*handler_t)(tokenizer_t* tokenizer);

    static handler_t handlers[] = {
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
    };
    token_result_t result = TOKEN_OK;
    do {
        handler_t handler = handlers[tokenizer->data_state];
        if (!handler) {
            printf("Unknown or unhandled tokenizer state");
            result = TOKEN_ERROR;
            break;
        }
        result = handler(tokenizer);
    }
    while (result == TOKEN_RECONSUME);
    return result;
}