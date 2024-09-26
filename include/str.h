#ifndef STR_H_
#define STR_H_

#include <stdio.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} str_t;

void str_free(str_t *s);

void str_insert(str_t *s, char const *cstr, size_t length, size_t index);
void str_remove(str_t *s, size_t length, size_t index);

size_t str_find_char(const str_t *s, char c, size_t index);
size_t str_find_char_rev(const str_t *s, char c, size_t index);

void str_load_file(str_t *s, FILE *fp);
void str_write_file(str_t const *s, FILE *fp);

#endif // STR_H_
