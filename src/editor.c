#include "editor.h"

#include "lib.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void editor_free(editor_t *e)
{
    str_free(&e->buffer);
    *e = (editor_t) { 0 };
}

// Movement

void editor_forward_char(editor_t *e)
{
    if (e->cursor < e->buffer.length) {
        e->cursor++;
    }
}

void editor_backward_char(editor_t *e)
{
    if (e->cursor > 0) {
        e->cursor--;
    }
}

void editor_move_end_of_line(editor_t *e)
{
    e->cursor = str_find_char(&e->buffer, '\n', e->cursor);
}

void editor_move_beginning_of_line(editor_t *e)
{
    size_t index = str_find_char_rev(&e->buffer, '\n', e->cursor);
    e->cursor = index + (index != 0);
}

void editor_next_line(editor_t *e)
{
    size_t target_col = editor_get_cursor_col(e);

    // Move to next line
    editor_move_end_of_line(e);
    editor_forward_char(e);

    // Move to target column
    size_t line_length = str_find_char(&e->buffer, '\n', e->cursor) - e->cursor;
    if (line_length > target_col) {
        e->cursor += target_col;
    } else {
        e->cursor += line_length;
    }
}

void editor_previous_line(editor_t *e)
{
    size_t target_col = editor_get_cursor_col(e);

    // Move to previous line
    editor_move_beginning_of_line(e);
    editor_backward_char(e);

    size_t cur = e->cursor;
    editor_move_beginning_of_line(e);
    // Verify that the cursor changed lines through the difference between the cursor pos
    // before and after the `move_beginning_of_line` call
    if (cur != e->cursor) {
        // Move to target column
        size_t line_length = str_find_char(&e->buffer, '\n', e->cursor) - e->cursor;
        if (line_length > target_col) {
            e->cursor += target_col;
        } else {
            e->cursor += line_length;
        }
    }
}

// Editing

void editor_insert_text(editor_t *e, char const *text, size_t text_size)
{
    str_insert(&e->buffer, text, text_size, e->cursor);
    e->cursor += text_size;
}

void editor_delete_char(editor_t *e)
{
    if (e->cursor < e->buffer.length) {
        str_remove(&e->buffer, 1, e->cursor);
    }
}

void editor_delete_backward_char(editor_t *e)
{
    if (e->cursor > 0) {
        e->cursor--;
        editor_delete_char(e);
    }
}

void editor_newline(editor_t *e)
{
    editor_insert_text(e, "\n", 1);
}

// File I/O

void editor_load_file(editor_t *e)
{
    FILE *fp = fopen(e->pathname.data, "r");
    str_free(&e->buffer);
    e->cursor = 0;
    str_load_file(&e->buffer, fp);
    fclose(fp);
}

void editor_save_buffer(editor_t *e)
{
    if (str_isnull(&e->pathname)) {
        char cwd[512];
        getcwd(cwd, 512);
        str_push_cstr( &e->pathname, cwd);
        str_push_cstr( &e->pathname, "/*scratch*");
    }
    struct stat filestat;
    if (stat(e->pathname.data, &filestat) == -1 && ENOENT != errno) {
        panic("Could not stat %s: %s\n", e->pathname.data, strerror(errno));
    }

    if (ENOENT == errno || ((filestat.st_mode & S_IFMT) == S_IFREG)) {
        FILE *fp = fopen(e->pathname.data, "w");
        assert(fp != NULL);
        str_write_file(&e->buffer, fp);
        fclose(fp);
    } else {
        panic("Not a regular file 0o%o: %s", (filestat.st_mode & S_IFMT), e->pathname.data);
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
    struct stat filestat;
    if (stat(e->pathname.data, &filestat) == -1) {
        panic("Could not stat %s: %s\n", e->pathname.data, strerror(errno));
    }

    switch (filestat.st_mode & S_IFMT) {
        case S_IFDIR:
            str_free(&e->buffer);
            if (!str_readdir(&e->buffer, e->pathname.data, &e->fsnav_entry_count)) {
                panic("Could not read directory: %s\n", strerror(errno));
            }
            e->fsnav = true;
            break;
        case S_IFREG:
            editor_load_file(e);
            e->fsnav = false;
            break;
        default:
            panic("Unknown file type.");
    }
    e->cursor = 0;
}

void editor_fsnav(editor_t *e)
{
    if (str_isnull(&e->pathname)) {
        char cwd[512] = { 0 };
        if (getcwd(cwd, sizeof cwd) == NULL) {
            panic("getcwd: %s\n", strerror(errno));
        }
        str_push_cstr(&e->pathname, cwd);
    } else {
        pathname_parent(&e->pathname);
    }

    editor_read_pathname(e);
}

void editor_fsnav_find_file(editor_t *e)
{
    editor_move_beginning_of_line(e);
    size_t entry_length = strcspn(e->buffer.data + e->cursor, "\n");
    if (strncmp(e->buffer.data + e->cursor, "..", entry_length) == 0) {
        pathname_parent(&e->pathname);
    } else {
        str_push_cstr(&e->pathname, "/");
        str_push(&e->pathname, e->buffer.data + e->cursor, entry_length);
    }
    editor_read_pathname(e);
}

// Get editor Information

size_t editor_get_line_count(editor_t const *e)
{
    size_t line = 0;
    for (size_t cursor = 0; cursor < e->buffer.length; cursor++) {
        if (e->buffer.data[cursor] == '\n') {
            line++;
        }
    }
    return line;
}

size_t editor_get_cursor_row(editor_t const *e)
{
    return str_count_rev(&e->buffer, '\n', e->cursor);
}

size_t editor_get_cursor_col(editor_t const *e)
{
    size_t index = str_find_char_rev(&e->buffer, '\n', e->cursor);
    return e->cursor - index - (index != 0);
}

char editor_get_char(editor_t const *e)
{
    if (e->cursor < e->buffer.length) {
        return e->buffer.data[e->cursor];
    }
    return '\0';
}

size_t editor_nth_char_index(editor_t const *e, char c, size_t nth)
{
    size_t cursor = 0;
    for (size_t char_nth = 0; cursor < e->buffer.length; cursor++) {
        if (char_nth == nth) {
            return cursor;
        }
        if (e->buffer.data[cursor] == c) {
            char_nth++;
        }
    }
    return cursor;
}
