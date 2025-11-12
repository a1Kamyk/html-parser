#include "preprocessor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

static int copy_between_streams(FILE* original_stream,
                                FILE* new_stream,
                                const size_t start_pos,
                                const size_t amount) {
    if (fseek(original_stream, (off_t)start_pos, SEEK_SET) != 0) return STDIO_ERROR;
    static const size_t BUF_SIZE = 1024;
    char copy_buffer[BUF_SIZE];
    size_t total = 0;

    while (total < amount) {
        const size_t to_read = MIN(amount - total, BUF_SIZE);
        const size_t n = fread(copy_buffer, 1, to_read, original_stream);
        if (n == 0) return ferror(original_stream);

        fwrite(copy_buffer, 1, n, new_stream);
        if (ferror(new_stream) != 0) return ferror(new_stream);
        total += n;
    }

    return 0;
}

static int remove_positions(const size_t* positions,
                            const size_t positions_count,
                            FILE* original_stream,
                            FILE* new_stream,
                            size_t start_pos,
                            const size_t end_pos) {
    size_t i = 0;
    if (positions[i] == 0) i++;
    for (; i < positions_count; i++) {
        const size_t amount = positions[i] - start_pos;
        const int res = copy_between_streams(original_stream, new_stream, start_pos, amount);
        if (res != 0) return STDIO_ERROR;
        start_pos = positions[i] + 1;
    }
    const int res = copy_between_streams(original_stream, new_stream, start_pos, end_pos);
    if (res != 0) return STDIO_ERROR;

    return 0;
}

int normalize_newlines(const char* filename) {
    FILE* stream = fopen(filename, "rb");
    if (!stream) {
        printf("Error opening file %s\n", filename);
        return STDIO_ERROR;
    }

    char* temp_filepath = malloc(strlen(filename) + strlen(temp_dir) + 1);
    if (!temp_filepath) return MEMORY_ERROR;
    strcpy(temp_filepath, temp_dir);
    strcat(temp_filepath, filename);
    // Remove old temporary file if exists
    int res = remove(temp_filepath);
    if (res != 0 && errno != ENOENT) {
        free(temp_filepath);
        return STDIO_ERROR;
    }

    FILE* new_stream = fopen(temp_filepath, "ab");
    if (!new_stream) {
        free(temp_filepath);
        return STDIO_ERROR;
    }

    static const size_t BUF_SIZE = 1024;
    int c = fgetc(stream);
    size_t curr_pos = 0, start_pos = 0;
    size_t remove_count = 0;
    size_t remove_pos_buffer[BUF_SIZE];

    while (c != EOF) {
        if (remove_count >= BUF_SIZE) {
            res = remove_positions(remove_pos_buffer, remove_count, stream, new_stream, start_pos, curr_pos);
            if (res != 0) {
                free(temp_filepath);
                fclose(stream);
                return STDIO_ERROR;
            }
            start_pos = curr_pos;
            remove_count = 0;
        }

        if (c == '\r') {
            remove_pos_buffer[remove_count++] = curr_pos;
        }

        curr_pos++;
        c = fgetc(stream);
    }

    if (remove_count != 0) {
        res = remove_positions(remove_pos_buffer, remove_count, stream, new_stream, start_pos, curr_pos);
        if (res != 0) {
            free(temp_filepath);
            fclose(stream);
            return STDIO_ERROR;
        }
    }

    free(temp_filepath);
    fclose(stream);
    return 0;
}