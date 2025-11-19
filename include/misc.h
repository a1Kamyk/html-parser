#ifndef HTML_PARSER_MISC_H
#define HTML_PARSER_MISC_H

#include <stdlib.h>
#include <stdbool.h>

#define DEFAULT_CAPACITY 8
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

typedef struct {
    int*  data;
    size_t length;
    size_t capacity;
} string_t;

int parser_string_init(string_t* str);
int parser_string_init_sized(string_t* str, size_t capacity);
int parser_string_clear(string_t* str);
int parser_string_delete(string_t* str);
int parser_strcmp(const string_t* str1, const string_t* str2);
int parser_cstrcmp(const char* str1, const string_t* str2);
int parser_string_append_char(string_t* str, int c);

bool contains(const char* arr[], size_t arr_size, const string_t* str);

#endif //HTML_PARSER_MISC_H