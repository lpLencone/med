#include "editor.h"

#include "lib.h"
#include <assert.h>

void editor_free(editor_t *e)
{
    str_free(&e->string);
    *e = (editor_t) {0};
}

// Movement

void editor_forward_char(editor_t *e)
{
    if (e->cursor < e->string.length) {
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
    e->cursor = str_find_char(&e->string, '\n', e->cursor);
}

void editor_move_beginning_of_line(editor_t *e)
{
    size_t index = str_find_char_rev(&e->string, '\n', e->cursor);
    e->cursor = index + (index != 0);
}

void editor_next_line(editor_t *e)
{
    size_t target_col = editor_get_cursor_col(e);

    // Move to next line
    editor_move_end_of_line(e);
    editor_forward_char(e);

    // Move to target column
    size_t line_length = str_find_char(&e->string, '\n', e->cursor) - e->cursor;
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
        size_t line_length = str_find_char(&e->string, '\n', e->cursor) - e->cursor;
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
    str_insert(&e->string, text, text_size, e->cursor);
    e->cursor += text_size;
}

void editor_delete_char(editor_t *e)
{
    if (e->cursor < e->string.length) {
        str_remove(&e->string, 1, e->cursor);
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

void editor_load_file(editor_t *e, FILE *fp)
{
    editor_free(e);
    str_load_file(&e->string, fp);
}

void editor_save_to_file(editor_t const *e, FILE *fp)
{
    str_write_file(&e->string, fp);
}

// Get editor Information

size_t editor_get_line_count(editor_t const *e)
{
    size_t line = 0;
    for (size_t cursor = 0; cursor < e->string.length; cursor++) {
        if (e->string.data[cursor] == '\n') {
            line++;
        }
    }
    return line;
}

size_t editor_get_cursor_row(editor_t const *e)
{
    return str_count_rev(&e->string, '\n', e->cursor);
}

size_t editor_get_cursor_col(editor_t const *e)
{
    size_t index = str_find_char_rev(&e->string, '\n', e->cursor);
    return e->cursor - index - (index != 0);
}

char editor_get_char(editor_t const *e)
{
    if (e->cursor < e->string.length) {
        return e->string.data[e->cursor];
    }
    return '\0';
}

size_t editor_nth_char_index(editor_t const *e, char c, size_t nth)
{
    size_t cursor = 0;
    for (size_t char_nth = 0; cursor < e->string.length; cursor++) {
        if (char_nth == nth) {
            return cursor;
        }
        if (e->string.data[cursor] == c) {
            char_nth++;
        }
    }
    return cursor;
}

