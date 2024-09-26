#include "buffer.h"

#include <assert.h>

static size_t buffer_cursor_column(buffer_t const *b);

void buffer_free(buffer_t *b)
{
    str_free(&b->string);
    *b = (buffer_t) { 0 };
}

// Movement

void buffer_forward_char(buffer_t *b)
{
    if (b->cursor < b->string.length) {
        b->cursor++;
    }
}

void buffer_backward_char(buffer_t *b)
{
    if (b->cursor > 0) {
        b->cursor--;
    }
}

void buffer_move_end_of_line(buffer_t *b)
{
    b->cursor = str_find_char(&b->string, '\n', b->cursor);
}

void buffer_move_beginning_of_line(buffer_t *b)
{
    size_t index = str_find_char_rev(&b->string, '\n', b->cursor);
    b->cursor = index + (index != 0);
}

void buffer_next_line(buffer_t *b)
{
    size_t target_col = buffer_cursor_column(b);

    // Move to next line
    buffer_move_end_of_line(b);
    buffer_forward_char(b);

    // Move to target column
    size_t line_length = str_find_char(&b->string, '\n', b->cursor) - b->cursor;
    if (line_length > target_col) {
        b->cursor += target_col;
    } else {
        b->cursor += line_length;
    }
}

void buffer_previous_line(buffer_t *b)
{
    size_t target_col = buffer_cursor_column(b);

    // Move to previous line
    buffer_move_beginning_of_line(b);
    buffer_backward_char(b);

    size_t cur = b->cursor;
    buffer_move_beginning_of_line(b);
    // Verify that the cursor changed lines through the difference between the cursor pos before and
    // after the `move_beginning_of_line` call
    if (cur != b->cursor) {
        // Move to target column
        size_t line_length = str_find_char(&b->string, '\n', b->cursor) - b->cursor;
        if (line_length > target_col) {
            b->cursor += target_col;
        } else {
            b->cursor += line_length;
        }
    }
}

// Editing

void buffer_insert_text(buffer_t *b, char const *text, size_t text_size)
{
    str_insert(&b->string, text, text_size, b->cursor);
    b->cursor += text_size;
}

void buffer_delete_backward_char(buffer_t *b)
{
    if (b->cursor > 0) {
        str_remove(&b->string, 1, b->cursor - 1);
        b->cursor--;
    }
}

void buffer_delete_char(buffer_t *b)
{
    if (b->cursor < b->string.length) {
        str_remove(&b->string, 1, b->cursor);
    }
}

void buffer_newline(buffer_t *b)
{
    buffer_insert_text(b, "\n", 1);
}

// File I/O

void buffer_load_file(buffer_t *b, FILE *fp)
{
    buffer_free(b);
    str_load_file(&b->string, fp);
}

void buffer_save_to_file(buffer_t const *b, FILE *fp)
{
    str_write_file(&b->string, fp);
}

static size_t buffer_cursor_column(buffer_t const *b)
{
    size_t index = str_find_char_rev(&b->string, '\n', b->cursor);
    return b->cursor - index - (index != 0);
}
