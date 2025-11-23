#ifndef HTML_PARSER_MISC_H
#define HTML_PARSER_MISC_H

#include <stdlib.h>
#include <stdbool.h>

#define NULL_CHARACTER          0x0000
#define CHARACTER_TABULATION    0x0009
#define LINE_FEED               0x000A
#define FORM_FEED               0x000C
#define CARRIAGE_RETURN         0x000D
#define SPACE                   0x0020

#define REPLACEMENT_CHARACTER   0xFFFD
#define SOLIDUS                 0x002F
#define LESS_THAN_SIGN          0x003C
#define GREATER_THAN_SIGN       0x003E
#define QUOTATION_MARK          0x0022
#define APOSTROPHE              0x0027

#define STRING_DEFAULT_CAPACITY 8
#define ATTR_LIST_DEFAULT_CAPACITY 4
#define CAPACITY_EXPANSION_COEF 2

#define parser_max(a, b) (a > b ? a : b)
#define parser_min(a, b) (a < b ? a : b)

static inline bool is_ascii_upper_alpha(const int c) {
    return (0x0041 <= c && c <= 0x005A);
}

static inline bool is_ascii_lower_alpha(const int c) {
    return (0x0061 <= c && c <= 0x007A);
}

static inline bool is_ascii_alpha(const int c) {
    return is_ascii_upper_alpha(c) || is_ascii_lower_alpha(c);
}

static inline bool is_html_whitespace(int c) {
    switch (c) {
        case CHARACTER_TABULATION:
        case LINE_FEED:
        case FORM_FEED:
        case CARRIAGE_RETURN:
        case SPACE:
            return true;
        default:
            return false;
    }
}

typedef struct {
    int*  data;
    size_t length;
    size_t capacity;
} string_t;

int parser_string_init(string_t* str);
int parser_string_init_sized(string_t* str, size_t capacity);
int parser_string_init_cstr(string_t* str, const char* cstr);
int parser_string_clear(string_t* str);
int parser_string_delete(string_t* str);
int parser_strcmp(const string_t* str1, const string_t* str2);
int parser_cstrcmp(const char* str1, const string_t* str2);
int parser_string_append_char(string_t* str, int c);
int parser_move_string(string_t* dest, string_t* src);

/// Attribute type
typedef struct {
    string_t    name;
    string_t    value;
} attribute_t;

/// Attribute list
typedef struct {
    attribute_t* items;
    size_t count;
    size_t capacity;
} attribute_list_t;

int attribute_list_init(attribute_list_t* list);
int attribute_list_init_sized(attribute_list_t* list, size_t capacity);
int attribute_list_clear(attribute_list_t* list);
int attribute_list_delete(attribute_list_t* list);
int attribute_list_append(attribute_list_t* list, const attribute_t* item);
int attribute_list_move(attribute_list_t* dest, attribute_list_t* src);

bool contains(const char* arr[], size_t arr_size, const string_t* str);

#endif //HTML_PARSER_MISC_H