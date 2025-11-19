#include "misc.h"

#include <stdio.h>
#include <string.h>

int parser_strcmp(const string_t* str1, const string_t* str2) {
    const size_t len = parser_umin(str1->length, str2->length);
    for (size_t i = 0; i < len; i++) {
        if (str1->data[i] != str2->data[i])
            return str1->data[i] - str2->data[i];
    }

    if (str1->length != str2->length)
        return (int)(str1->length - str2->length);

    return 0;
}

int parser_cstrcmp(const char* str1, const string_t* str2) {
    const size_t len = parser_umin(strlen(str1), str2->length);
    for (size_t i = 0; i < len; i++) {
        if (str2->data[i] > CHAR_MAX)
            return 1;
        if (str1[i] != str2->data[i])
            return (unsigned char)str1[i] - str2->data[i];
    }

    if (strlen(str1) != str2->length)
        return (int)(strlen(str1) - str2->length);

    return 0;
}

bool contains(const char* arr[], const size_t arr_size, const string_t* str) {
    for (size_t i = 0; i < arr_size; i++) {
        if (parser_cstrcmp(arr[i], str) == 0) return true;
    }

    return false;
}

int append_char(string_t* str, const int c) {
    if (str->length + 1 >= str->capacity) {
        const size_t new_capacity = (str->capacity == 0) ? DEFAULT_CAPACITY : str->capacity * CAPACITY_EXPANSION_COEF;
        int* new_data = realloc(str->data, new_capacity * sizeof(int));
        if (!new_data) {
            fprintf(stderr, "Out of memory allocating new string");
            return 1;
        }
        str->data = new_data;
        str->capacity = new_capacity;
    }

    str->data[str->length++] = c;
    return 0;
}