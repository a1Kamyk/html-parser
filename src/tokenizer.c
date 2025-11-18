#include "tokenizer.h"

#include <string.h>

#include "misc.h"

#define REPLACEMENT_CHARACTER   0xFFFD
#define LESS_THAN_SIGN          0x003C
#define SOLIDUS                 0x002F

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

int queue_pop(internal_token_queue_t* queue, const token_t* token) {
    if (queue_is_empty(queue) || !token) return 1;

    token = &queue->tokens[queue->end];
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

    return contains(void_tags, arr_size, token.data.tag.name);
}

bool is_template_element(const token_t token) {
    static const char* template_tag = "template";

    return strcmp(template_tag, token.data.tag.name);
}

bool is_raw_text_element(const token_t token) {
    static const char* raw_text_tags[] = {
        "script",
        "style"
    };
    static const size_t arr_size = sizeof(raw_text_tags) / sizeof(raw_text_tags[0]);

    return contains(raw_text_tags, arr_size, token.data.tag.name);
}

bool is_escapable_raw_text_element(const token_t token) {
    static const char* escapable_raw_text_tags[] = {
        "textarea",
        "title"
    };
    static const size_t arr_size = sizeof(escapable_raw_text_tags) / sizeof(escapable_raw_text_tags[0]);

    return contains(escapable_raw_text_tags, arr_size, token.data.tag.name);
}

bool is_foreign_element(token_t token) {
    // TODO make this work
    return false;
}
bool is_normal_element(token_t token) {
    // TODO and this
    return false;
}

bool is_ascii_upper_alpha(const int c) {
    return (0x0041 <= c && c <= 0x005A);
}

bool is_ascii_lower_alpha(const int c) {
    return (0x0061 <= c && c <= 0x007A);
}

inline bool is_ascii_alpha(const int c) {
    return (is_ascii_upper_alpha(c) && is_ascii_lower_alpha(c));
}

int consume_character(FILE* stream) {
    return fgetc(stream);
}

int reconsume_character(const int character) {
    return character;
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
    out->data.tag.name = calloc(1, sizeof(char));
    if (!out->data.tag.name) return 1;
    out->data.tag.attr_count = 0;
    out->data.tag.attributes = NULL;

    return 0;
}

int get_new_end_tag_token(token_t* out) {
    out->type = END_TAG;
    out->data.tag.name = calloc(1, sizeof(char));
    if (!out->data.tag.name) return 1;
    out->data.tag.attr_count = 0;
    out->data.tag.attributes = NULL;

    return 0;
}

int get_new_comment_token(token_t* out) {
    out->type = COMMENT;
    out->data.comment.data = calloc(1, sizeof(char));
    if (!out->data.comment.data) return 1;

    return 0;
}

void destroy_token(token_t* token) {
    if (!token) return;
    switch (token->type) {
        case DOCTYPE: {
            const doctype_token* token_data = &token->data.doctype;
            free(token_data->name);
            free(token_data->public_id);
            free(token_data->system_id);
            return;
        }
        case START_TAG:
        case END_TAG: {
            const tag_token* token_data = &token->data.tag;
            free(token_data->name);
            for (size_t i = 0; i < token_data->attr_count; i++) {
                const attribute_t* attribute = &token_data->attributes[i];
                free(attribute->name);
                free(attribute->value);
            }
            free(token_data->attributes);
            return;
        }
        case COMMENT: {
            const comment_token* token_data = &token->data.comment;
            free(token_data->data);
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

// TODO proper error naming
int token_next(FILE* stream, tokenizer_t* tokenizer) {
    int c = {};
    goto consume;

    reconsume:
    tokenizer->consume_flag = false;
    consume:
    switch (tokenizer->data_state) {
        case DATA_STATE: {
            if (tokenizer->consume_flag)
                c = consume_character(stream);
            else
                tokenizer->consume_flag = true;
            switch (c) {
                case '&':
                    tokenizer->return_state = DATA_STATE;
                    tokenizer->data_state = CHARACTER_REFERENCE_STATE;
                    return 0;
                case '<':
                    tokenizer->data_state = TAG_OPEN_STATE;
                    return 0;;
                case 0x00: {
                    // TODO `unexpected-null-character parse error`
                    const token_t token = get_character_token(0);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;;
                }
                case EOF: {
                    const token_t token = get_eof_token();
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                default: {
                    const token_t token =  get_character_token(c);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
            }
        }
        case RCDATA_STATE: {
            if (tokenizer->consume_flag)
                c = consume_character(stream);
            else
                tokenizer->consume_flag = true;
            switch (c) {
                case '&':
                    tokenizer->return_state = RCDATA_STATE;
                    tokenizer->data_state = CHARACTER_REFERENCE_STATE;
                    return 0;
                case '<':
                    tokenizer->data_state = RCDATA_LESS_THAN_SIGN_STATE;
                    return 0;
                case 0x00: {
                    // TODO `unexpected-null-character` parse error
                    const token_t token =  get_character_token(REPLACEMENT_CHARACTER);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                case EOF: {
                    const token_t token = get_eof_token();
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                default: {
                    const token_t token = get_character_token(c);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
            }
        }
            case RAWTEXT_STATE: {
            if (tokenizer->consume_flag)
                c = consume_character(stream);
            else
                tokenizer->consume_flag = true;
            switch (c) {
                case '<':
                    tokenizer->data_state = RAWTEXT_LESS_THAN_SIGN_STATE;
                    return 0;
                case 0x00: {
                    // TODO `unexpected-null-character` parse error
                    const token_t token = get_character_token(REPLACEMENT_CHARACTER);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                case EOF: {
                    const token_t token = get_eof_token();
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                default: {
                    const token_t token = get_character_token(c);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
            }
        }
        case SCRIPT_DATA_STATE: {
            if (tokenizer->consume_flag)
                c = consume_character(stream);
            else
                tokenizer->consume_flag = true;
            switch (c) {
                case '<':
                    tokenizer->data_state = SCRIPT_DATA_LESS_THAN_SIGN_STATE;
                    return 0;
                case 0x00: {
                    const token_t token = get_character_token(REPLACEMENT_CHARACTER);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                case EOF: {
                    const token_t token = get_eof_token();
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                default: {
                    const token_t token = get_character_token(c);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
            }
        }
        case PLAINTEXT_STATE: {
            if (tokenizer->consume_flag)
                c = consume_character(stream);
            else
                tokenizer->consume_flag = true;
            switch (c) {
                case 0x00: {
                    const token_t token = get_character_token(REPLACEMENT_CHARACTER);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                case EOF: {
                    const token_t token = get_eof_token();
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
                default: {
                    const token_t token = get_character_token(c);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    return 0;
                }
            }
        }
        case TAG_OPEN_STATE: {
            if (tokenizer->consume_flag)
                c = consume_character(stream);
            else
                tokenizer->consume_flag = true;
            switch (c) {
                case '!':
                    tokenizer->data_state = MARKUP_DECLARATION_OPEN_STATE;
                    return 0;
                case '/':
                    tokenizer->data_state = END_TAG_OPEN_STATE;
                    return 0;
                case '?': {
                    // TODO `unexpected-question-mark-instead-of-tag-name` parse error
                    token_t token;
                    if (get_new_comment_token(&token) != 0) {
                        fprintf(stderr, "Error creating new comment token");
                        return 1;
                    }
                    tokenizer->data_state = BOGUS_COMMENT_STATE;
                    goto reconsume;
                }
                case EOF: {
                    // TODO `eof-before-tag-name` parse error
                    const token_t less_than_sign_token = get_character_token(LESS_THAN_SIGN);
                    const token_t eof_token = get_eof_token();
                    if (queue_push(&tokenizer->internal_token_queue, &less_than_sign_token) != 0 ||
                        queue_push(&tokenizer->internal_token_queue, &eof_token) != 0)
                        return 1;
                    return 0;
                }
                default: {
                    if (is_ascii_alpha(c)) {
                        token_t token;
                        if (get_new_start_tag_token(&token) != 0) {
                            fprintf(stderr, "Error creating new start tag token");
                            return 1;
                        }
                        tokenizer->data_state = TAG_NAME_STATE;
                        goto reconsume;
                    }
                    // TODO `invalid-first-character-of-tag-name` parse error
                    const token_t token = get_character_token(LESS_THAN_SIGN);
                    if (queue_push(&tokenizer->internal_token_queue, &token) != 0 )
                        return 1;
                    tokenizer->data_state = DATA_STATE;
                    goto reconsume;
                }
            }
        }
        case END_TAG_OPEN_STATE: {
            if (tokenizer->consume_flag)
                c = consume_character(stream);
            else
                tokenizer->consume_flag = true;
            switch (c) {
                case '>': {
                    // TODO `missing-end-tag-name` parse error
                    tokenizer->data_state = DATA_STATE;
                    return 0;
                }
                case EOF: {
                    // TODO `eof-before-tag-name` parse error
                    const token_t less_than_sign_token = get_character_token(LESS_THAN_SIGN);
                    const token_t solidus_token = get_character_token(SOLIDUS);
                    if (queue_push(&tokenizer->internal_token_queue, &solidus_token) != 0 ||
                        queue_push(&tokenizer->internal_token_queue, &less_than_sign_token) != 0)
                        return 1;
                    return 0;
                }
                default: {
                    if (is_ascii_alpha(c)) {
                        token_t token;
                        if (get_new_end_tag_token(&token) != 0) {
                            fprintf(stderr, "Error creating new end tag token");
                            return 1;
                        }
                        tokenizer->data_state = TAG_NAME_STATE;
                        goto reconsume;
                    }
                    // TODO `invalid-first-character-of-tag-name` parse error
                    token_t token;
                    if (get_new_comment_token(&token) != 0) {
                        fprintf(stderr, "Error creating new comment token");
                        return 1;
                    }
                    tokenizer->data_state = BOGUS_COMMENT_STATE;
                    goto reconsume;
                }
            }
        }
        default: {
            printf("Unknown or unhandled tokenizer state");
            return 0;
        }
    }
}

dom_tree_node_t* tokenize(FILE* stream, tokenizer_t* tokenizer) {
    dom_tree_node_t* root = NULL;

    token_next(stream, tokenizer);
    return NULL;
}