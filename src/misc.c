#include "misc.h"

#include <stdio.h>
#include <string.h>

int parser_string_init(string_t* str) {
    return parser_string_init_sized(str, STRING_DEFAULT_CAPACITY);
}

int parser_string_init_sized(string_t* str, const size_t capacity) {
    str->data = calloc(capacity, sizeof(int));
    if (!str->data) {
        fprintf(stderr, "Out of memory allocating new string\n");
        return 1;
    }
    str->capacity = capacity;
    str->length = 0;
    return 0;
}

int parser_string_init_cstr(string_t* str, const char* cstr) {
    if (!cstr)
        return 1;

    const size_t len = strlen(cstr);
    if (parser_string_init_sized(str, len) != 0) {
        return 1;
    }
    for (size_t i = 0; i < len; i++) {
        str->data[i] = (int)(cstr[i]);
    }
    str->length = len;

    return 0;
}

int parser_string_clear(string_t* str) {
    str->length = 0;
    return 0;
}

int parser_string_delete(string_t* str) {
    free(str->data);
    str->capacity = 0;
    str->length = 0;
    return 0;
}

int parser_strcmp(const string_t* str1, const string_t* str2) {
    const size_t len = parser_min(str1->length, str2->length);
    for (size_t i = 0; i < len; i++) {
        if (str1->data[i] != str2->data[i])
            return str1->data[i] - str2->data[i];
    }

    if (str1->length != str2->length)
        return (int)(str1->length - str2->length);

    return 0;
}

int parser_cstrcmp(const char* str1, const string_t* str2) {
    const size_t len = parser_min(strlen(str1), str2->length);
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

int parser_string_append_char(string_t* str, const int c) {
    if (str->length + 1 > str->capacity) {
        const size_t new_capacity = (str->capacity == 0) ? STRING_DEFAULT_CAPACITY : str->capacity * CAPACITY_EXPANSION_COEF;
        int* new_data = realloc(str->data, new_capacity * sizeof(int));
        if (!new_data) {
            fprintf(stderr, "Out of memory allocating new string\n");
            return 1;
        }
        str->data = new_data;
        str->capacity = new_capacity;
    }

    str->data[str->length++] = c;
    return 0;
}

int parser_move_string(string_t* dest, string_t* src) {
    if (!dest || !src)
        return 1;
    *dest = *src;

    src->data = NULL;
    src->capacity = 0;
    src->length = 0;

    return 0;
}

int attribute_list_init(attribute_list_t* list) {
    return attribute_list_init_sized(list, ATTR_LIST_DEFAULT_CAPACITY);
}

int attribute_list_init_sized(attribute_list_t* list, const size_t capacity) {
    list->items = calloc(capacity, sizeof(attribute_t));
    if (!list->items) {
        fprintf(stderr, "Out of memory allocating new attribute list\n");
        return 1;
    }
    list->capacity = capacity;
    list->count = 0;
    return 0;
}

int attribute_list_clear(attribute_list_t* list) {
    list->count = 0;
    return 0;
}

int attribute_list_delete(attribute_list_t* list) {
    if (!list) return 0;
    for (size_t i = 0; i < list->count ; i++) {
        attribute_t* item = &list->items[i];
        parser_string_delete(&item->name);
        parser_string_delete(&item->value);
    }
    free(list->items);
    list->capacity = 0;
    list->count = 0;
    return 0;
}

int attribute_list_append(attribute_list_t* list, const attribute_t* item) {
    if (list->count + 1 > list->capacity) {
        const size_t new_capacity = (list->capacity == 0) ?
            ATTR_LIST_DEFAULT_CAPACITY :
            list->capacity * CAPACITY_EXPANSION_COEF;
        attribute_t* new_items = realloc(list->items, new_capacity * sizeof(attribute_t));
        if (!new_items) {
            fprintf(stderr, "Out of memory allocating new attribute list\n");
            return 1;
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count++] = *item;
    return 0;
}

int attribute_list_move(attribute_list_t* dest, attribute_list_t* src) {
    if (!dest || !src)
        return 1;

    *dest = *src;

    src->items = NULL;
    src->capacity = 0;
    src->count = 0;

    return 0;
}

bool contains(const char* arr[], const size_t arr_size, const string_t* str) {
    for (size_t i = 0; i < arr_size; i++) {
        if (parser_cstrcmp(arr[i], str) == 0) return true;
    }

    return false;
}