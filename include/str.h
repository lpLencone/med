#ifndef STR_H_
#define STR_H_

#include <stddef.h>
#include <stdio.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} str_t;

void str_free(str_t *s);

void str_insert(str_t *s, char const *cstr, size_t length, size_t index);
void str_remove(str_t *s, size_t length, size_t index);

size_t str_find_char(str_t const *s, char c, size_t index);
size_t str_find_char_rev(str_t const *s, char c, size_t index);
size_t str_count(str_t const *s, char c, size_t index);
size_t str_count_rev(str_t const *s, char c, size_t index);

void str_load_file(str_t *s, FILE *fp);
void str_write_file(str_t const *s, FILE *fp);

#define SVARG(sv) (int) (sv).length, (sv).data

typedef struct {
    char const *data;
    size_t length;
} strview_t;

strview_t sv_from_str(str_t const *str);
size_t sv_cspn(strview_t sv, char const *reject);
bool sv_token_subcstr(strview_t *sv, char const *subcstr, strview_t *out);
bool sv_token_cspn(strview_t *sv, char const *c, strview_t *out);
bool sv_token_cspn_consume(strview_t *sv, char const *reject, strview_t *out);

FILE *sv_fopen(strview_t sv_filename, char const *mode);

#endif // STR_H_
