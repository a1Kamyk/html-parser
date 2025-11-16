#include "misc.h"

#include <string.h>

bool contains(const char* arr[], const size_t arr_size, const char* str) {
    for (size_t i = 0; i < arr_size; i++) {
        if (strcmp(arr[i], str) == 0) return true;
    }

    return false;
}
