#ifndef HTML_PARSER_MISC_H
#define HTML_PARSER_MISC_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_CAPACITY 8
#define CAPACITY_EXPANSION_COEF 2

inline uint64_t parser_umax(const uint64_t a, const uint64_t b) {
    return a > b ? a : b;
}
inline int64_t parser_max(const int64_t a, const int64_t b) {
    return a > b ? a : b;
}
inline uint64_t parser_umin(const uint64_t a, const uint64_t b) {
    return a < b ? a : b;
}
inline int64_t parser_min(const int64_t a, const int64_t b) {
    return a < b ? a : b;
}

typedef struct {
    int*  data;
    size_t length;
    size_t capacity;
} string_t;

int parser_strcmp(const string_t* str1, const string_t* str2);
int parser_cstrcmp(const char* str1, const string_t* str2)
bool contains(const char* arr[], size_t arr_size, const string_t* str);

int append_char(string_t* str, int c);

#endif //HTML_PARSER_MISC_H