#include "editor.h"

#include "lib.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void editor_free(editor_t *e)
{
    (void) e;
    debugf("%s[Not freeing editor...]\n", "");
}

// TODO: eventually allow a zeroed out editor to be valid. Right now I need to setup
// pointers
void editor_new(editor_t *e)
{
    *e = (editor_t) { 0 };
    e->buffer = &e->text_buffer;
    e->cursor = &e->text_cursor;
}

// Movement

void editor_forward_char(editor_t *e)
{
    if (*e->cursor < e->buffer->length) {
        *e->cursor+=1;
    }
}

void editor_backward_char(editor_t *e)
{
    if (*e->cursor > 0) {
        *e->cursor -= 1;
    }
}

void editor_move_end_of_line(editor_t *e)
{
    *e->cursor = str_find_char(e->buffer, '\n', *e->cursor);
}

void editor_move_beginning_of_line(editor_t *e)
{
    size_t index = str_find_char_rev(e->buffer, '\n', *e->cursor);
    *e->cursor = index + (index != 0);
}

void editor_next_line(editor_t *e)
{
    size_t target_col = editor_get_cursor_col(e);

    // Move to next line
    editor_move_end_of_line(e);
    editor_forward_char(e);

    // Move to target column
    size_t line_length = str_find_char(e->buffer, '\n', *e->cursor) - *e->cursor;
    if (line_length > target_col) {
        *e->cursor += target_col;
    } else {
        *e->cursor += line_length;
    }
}

void editor_previous_line(editor_t *e)
{
    size_t target_col = editor_get_cursor_col(e);

    // Move to previous line
    editor_move_beginning_of_line(e);
    editor_backward_char(e);

    size_t cur = *e->cursor;
    editor_move_beginning_of_line(e);
    // Verify that the cursor changed lines through the difference between the cursor pos
    // before and after the `move_beginning_of_line` call
    if (cur != *e->cursor) {
        // Move to target column
        size_t line_length = str_find_char(e->buffer, '\n', *e->cursor) - *e->cursor;
        if (line_length > target_col) {
            *e->cursor += target_col;
        } else {
            *e->cursor += line_length;
        }
    }
}

// Editing

static void editor_delete_selection(editor_t *e)
{
    assert(e->mark_set);
    size_t start, end;
    if (*e->cursor > e->mark) {
        start = e->mark;
        end = *e->cursor;
    } else {
        start = *e->cursor;
        end = e->mark;
    }
    str_remove(e->buffer, end - start, start);
    *e->cursor = start;
    e->mark_set = false;
}

void editor_insert(editor_t *e, char const *text, size_t text_size)
{
    if (e->mark_set) {
        editor_delete_selection(e);
    }
    str_insert(e->buffer, text, text_size, *e->cursor);
    *e->cursor += text_size;
}

void editor_self_insert(editor_t *e, char c)
{
    editor_insert(e, &c, 1);
}

void editor_delete_char(editor_t *e)
{
    if (e->mark_set) {
        editor_delete_selection(e);
        return;
    }
    if (*e->cursor < e->buffer->length) {
        str_remove(e->buffer, 1, *e->cursor);
    }
}

void editor_delete_backward_char(editor_t *e)
{
    if (e->mark_set) {
        editor_delete_selection(e);
        return;
    }
    if (*e->cursor > 0) {
        *e->cursor -= 1;
        editor_delete_char(e);
    }
}

void editor_newline(editor_t *e)
{
    // TODO: /* electric */
    editor_self_insert(e, '\n');
}

void editor_set_mark(editor_t *e)
{
    // TODO: for now
    assert(!e->fsnav);
    e->mark_set = true;
    e->mark = *e->cursor;
}

// Minibuffer

static void
editor_minibuffer_start(editor_t *e, char const *prompt, char const *placeholder)
{
    assert(!e->mini);
    e->mini = true;
    e->miniprompt = prompt;
    if (placeholder == NULL) {
        placeholder = "";
    }
    str_push_cstr(&e->minibuffer, placeholder);
    e->minicursor = e->minibuffer.length;
    e->buffer = &e->minibuffer;
    e->cursor = &e->minicursor;
}

void editor_minibuffer_terminate(editor_t *e)
{
    assert(e->mini);
    assert(e->minicallback != NULL);
    e->minicallback(e);
    str_free(&e->minibuffer);
    e->minicursor = 0;
    e->mini = false;
    // TODO: introduce alternate buffers
    e->buffer = &e->text_buffer;
    e->cursor = &e->text_cursor;
    e->minicallback = NULL;
}

// I-Search

void editor_isearch(editor_t *e)
{
    editor_minibuffer_start(e, "I-Search: ", NULL);
}

// File I/O

void editor_load_file(editor_t *e, char const *filename)
{
    e->buffer = &e->text_buffer;
    e->cursor = &e->text_cursor;

    // Load file contents
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        panic("Could not open file \"%s\": %s", filename, strerror(errno));
    }
    str_free(&e->text_buffer);
    str_load_file(&e->text_buffer, fp);
    fclose(fp);

    str_free(&e->pathname);
    if (*filename == '/') {
        str_push_cstr(&e->pathname, filename);
    } else {
        char cwd[512] = { 0 };
        getcwd(cwd, 512);
        cwd[strlen(cwd)] = '/';
        str_push_cstr(&e->pathname, cwd);
        str_push_cstr(&e->pathname, filename);
    }

    *e->cursor = 0;
}

static void editor_set_pathname(editor_t *e)
{
    str_free(&e->pathname);
    str_push_cstr(&e->pathname, e->minibuffer.data);
    editor_save_buffer(e);
}

void editor_save_buffer(editor_t *e)
{
    if (str_isnull(&e->pathname)) {
        char cwd[512] = { 0 };
        getcwd(cwd, 512);
        cwd[strlen(cwd)] = '/';
        editor_minibuffer_start(e, "Filename: ", cwd);
        e->minicallback = editor_set_pathname;
        return;
    }

    struct stat filestat;
    if (stat(e->pathname.data, &filestat) == -1 && ENOENT != errno) {
        panic("Could not stat %s: %s\n", e->pathname.data, strerror(errno));
    }

    if (ENOENT == errno || ((filestat.st_mode & S_IFMT) == S_IFREG)) {
        FILE *fp = fopen(e->pathname.data, "w");
        if (fp == NULL) {
            panic("Could not open file \"%s\": %s\n", e->pathname.data, strerror(errno));
        }
        str_write_file(&e->text_buffer, fp);
        fclose(fp);
    } else {
        panic("Not a regular file 0o%o: %s", (filestat.st_mode & S_IFMT),
              e->pathname.data);
    }
}

/// fsnav functions

static void pathname_parent(str_t *pathname)
{
    size_t divisor = str_find_char_rev(pathname, '/', pathname->length);
    str_remove_from(pathname, divisor + (divisor == 0));
}

static void editor_read_pathname(editor_t *e)
{
    printf("[EDITOR] Trying to read \"%s\"\n", e->pathname.data);
    struct stat filestat;
    if (stat(e->pathname.data, &filestat) == -1) {
        panic("Could not stat %s: %s\n", e->pathname.data, strerror(errno));
    }

    switch (filestat.st_mode & S_IFMT) {
        case S_IFDIR:
            str_free(&e->text_buffer);
            if (!str_readdir(&e->text_buffer, e->pathname.data, &e->fsnav_entry_count)) {
                panic("Could not read directory: %s\n", strerror(errno));
            }
            e->fsnav = true;
            break;
        case S_IFREG:
            editor_load_file(e, e->pathname.data);
            e->fsnav = false;
            break;
        default:
            panic("Unknown file type.");
    }
    *e->cursor = 0;
}

void editor_fsnav(editor_t *e)
{
    if (str_isnull(&e->pathname)) {
        str_push_cstr(&e->pathname, ".");
    } else {
        pathname_parent(&e->pathname);
    }
    editor_read_pathname(e);
}

void editor_fsnav_find_file(editor_t *e)
{
    editor_move_beginning_of_line(e);
    size_t entry_length = strcspn(e->text_buffer.data + e->text_cursor, "\n");
    if (strncmp(e->text_buffer.data + e->text_cursor, "..", entry_length) == 0) {
        pathname_parent(&e->pathname);
    } else {
        str_push_cstr(&e->pathname, "/");
        str_push(&e->pathname, e->text_buffer.data + e->text_cursor, entry_length);
    }
    editor_read_pathname(e);
}

// Get editor Information

size_t editor_get_line_count(editor_t const *e)
{
    size_t line = 0;
    for (size_t cursor = 0; cursor < e->text_buffer.length; cursor++) {
        if (e->text_buffer.data[cursor] == '\n') {
            line++;
        }
    }
    return line;
}

size_t editor_get_cursor_row(editor_t const *e)
{
    return str_count_rev(&e->text_buffer, '\n', e->text_cursor);
}

size_t editor_get_cursor_col(editor_t const *e)
{
    size_t index = str_find_char_rev(&e->text_buffer, '\n', e->text_cursor);
    return e->text_cursor - index - (index != 0);
}

void editor_get_cursor_line_boundaries(editor_t const *e, size_t *start, size_t *end)
{
    *start = str_find_char_rev(&e->text_buffer, '\n', e->text_cursor);
    *end = str_find_char(&e->text_buffer, '\n', e->text_cursor);
}

char editor_get_char(editor_t const *e)
{
    if (e->text_cursor < e->text_buffer.length) {
        return e->text_buffer.data[e->text_cursor];
    }
    return '\0';
}

size_t editor_nth_char_index(editor_t const *e, char c, size_t nth)
{
    size_t cursor = 0;
    for (size_t char_nth = 0; cursor < e->text_buffer.length; cursor++) {
        if (char_nth == nth) {
            return cursor;
        }
        if (e->text_buffer.data[cursor] == c) {
            char_nth++;
        }
    }
    return cursor;
}
