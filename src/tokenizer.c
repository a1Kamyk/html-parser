#include "tokenizer.h"

#include <string.h>

#include "misc.h"
#include "tree.h"

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

bool is_void_element(token_t token) {
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

bool is_raw_text_element(token_t token) {
    static const char* raw_text_tags[] = {
        "script",
        "style"
    };
    static const size_t arr_size = sizeof(raw_text_tags) / sizeof(raw_text_tags[0]);

    return contains(raw_text_tags, arr_size, token.data.tag.name);
}

bool is_escapable_raw_text_element(token_t token) {
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

int consume_character(FILE* stream, tokenizer_state_t state) {
    return 0;
}

int reconsume_character(int character, tokenizer_state_t state) {
    return 0;
}

dom_tree_node_t* tokenize(FILE* stream, tokenizer_state_t state) {
    return NULL;
}