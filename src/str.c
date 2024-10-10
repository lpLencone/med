#include "str.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

size_t str_count(str_t const *s, char c, size_t index)
{
    assert(index <= s->length);
    size_t count = 0;
    while (index < s->length) {
        if (s->data[index++] == c) {
            count++;
        }
    }
    return count;
}

size_t str_count_rev(str_t const *s, char c, size_t index)
{
    assert(index <= s->length);
    size_t count = 0;
    while (index > 0) {
        if (s->data[--index] == c) {
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

static int entry_filter(const struct dirent *entry)
{
    bool ret = strcmp(entry->d_name, ".") != 0;
    return ret;
}

bool str_readdir(str_t *s, char const *dirname, size_t *out_entry_count)
{
    struct dirent **entrylist = NULL;
    int entry_count = scandir(dirname, &entrylist, entry_filter, alphasort);
    if (entry_count == -1) {
        debugf("Could not scan directry: %s\n", strerror(errno));
        return false;
    }
    for (int i = 0; i < entry_count; i++) {
        str_push_cstr(s, entrylist[i]->d_name);
        str_push_cstr(s, "\n");
    }
    *out_entry_count = entry_count;
    return true;
}

bool str_isnull(str_t const *s)
{
    return s->data == NULL;
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

// String View

strview_t sv_from_str(str_t const *str)
{
    return (strview_t) {
        .data = str->data,
        .length = str->length,
    };
}

size_t sv_cspn(strview_t sv, char const *reject)
{
    size_t length = 0;
    while (length < sv.length) {
        char *c = strchr(reject, sv.data[length]);
        if (c != NULL) {
            break;
        }
        length++;
    }
    return length;
}

bool sv_token_subcstr(strview_t *sv, char const *sub, strview_t *out)
{
    size_t sublen = strlen(sub);
    for (size_t i = 0; i + sublen < sv->length; i++) {
        if (strncmp(sv->data + i, sub, sublen) == 0) {
            if (out != NULL) {
                out->data = sv->data;
                out->length = i;
            }
            sv->data += i;
            sv->length -= i;
            return true;
        }
    }
    return false;
}

bool sv_token_cspn(strview_t *sv, char const *reject, strview_t *out)
{
    size_t length = sv_cspn(*sv, reject);
    if (out != NULL) {
        out->data = sv->data;
        out->length = length;
    }
    sv->data += length;
    sv->length -= length;
    return sv->length > 0;
}

bool sv_token_cspn_consume(strview_t *sv, char const *reject, strview_t *out)
{
    if (!sv_token_cspn(sv, reject, out)) {
        return false;
    }
    size_t length = min(sv->length, strspn(sv->data, reject));
    sv->data += length;
    sv->length -= length;
    return true;
}

FILE *sv_fopen(strview_t sv_filename, char const *mode)
{
    char *filename = malloc(sv_filename.length + 1);
    memcpy(filename, sv_filename.data, sv_filename.length);
    filename[sv_filename.length] = '\0';
    FILE *fp = fopen(filename, mode);
    free(filename);
    return fp;
}
