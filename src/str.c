#include "str.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"

#define STR_INIT_CAP 16

static void str_grow(str_t *s, size_t size);

void str_free(str_t *s)
{
    if (s->capacity > 0) {
        free(s->data);
    }
    *s = (str_t) { 0 };
}

void str_insert(str_t *s, char const *cstr, size_t length, size_t index)
{
    assert(index <= s->length);
    str_grow(s, length);
    memmove(s->data + index + length, s->data + index, s->length - index);
    memcpy(s->data + index, cstr, length);
    s->length += length;
    assert(s->length < s->capacity);
    s->data[s->length] = '\0';
}

void str_remove(str_t *s, size_t length, size_t index)
{
    assert(index + length <= s->length);
    memmove(s->data + index, s->data + index + length, s->length - (index + length));
    s->length -= length;
    s->data[s->length] = '\0';
}

size_t str_find_char(str_t const *s, char c, size_t index)
{
    while (index < s->length && s->data[index] != c) {
        index++;
    }
    return index;
}

size_t str_find_char_rev(str_t const *s, char c, size_t index)
{
    assert(index <= s->length);
    while (index > 0 && s->data[--index] != c) {
        continue;
    }
    return index;
}

size_t str_count_rev(const str_t *s, char c, size_t index)
{
    assert(index <= s->length);
    size_t count = 0;
    while (index > 0) {
        if (s->data[index] == c) {
            count++;
        }
    }
    return count;
}

void str_load_file(str_t *s, FILE *fp)
{
    assert(fseek(fp, 0, SEEK_END) >= 0);
    long filesize = ftell(fp);
    assert(filesize >= 0);
    assert(fseek(fp, 0, SEEK_SET) >= 0);

    str_grow(s, filesize);
    assert(fread(s->data, sizeof *s->data, s->capacity, fp) == (size_t) filesize);
    s->length += filesize;
    s->data[s->length] = '\0';
}

void str_write_file(str_t const *s, FILE *fp)
{
    fwrite(s->data, sizeof *s->data, s->length, fp);
}

static void str_grow(str_t *s, size_t size)
{
    size_t cap = s->capacity;
    if (cap < STR_INIT_CAP) {
        cap = STR_INIT_CAP;
    }
    while (cap < s->length + size + 1) { // 1 byte for '\0'
        cap *= 2;
    }
    if (cap != s->capacity) {
        s->capacity = cap;
        s->data = realloc(s->data, s->capacity);
    }
}
