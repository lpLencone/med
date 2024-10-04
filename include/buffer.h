#ifndef BUFFER_H_
#define BUFFER_H_

#include <stddef.h>

#include "str.h"

// It would be nice to insert text anywhere in the buffer regardless of cursor position.

typedef struct {
    str_t string;
    size_t cursor;
} buffer_t;

void buffer_free(buffer_t *b);

// Moving
void buffer_forward_char(buffer_t *b);
void buffer_backward_char(buffer_t *b);
void buffer_move_end_of_line(buffer_t *b);
void buffer_move_beginning_of_line(buffer_t *b);
void buffer_next_line(buffer_t *b);
void buffer_previous_line(buffer_t *b);

// Editing
void buffer_insert_text(buffer_t *b, char const *text, size_t text_size);
void buffer_delete_backward_char(buffer_t *b);
void buffer_delete_char(buffer_t *b);
void buffer_newline(buffer_t *b);

// File I/O
void buffer_load_file(buffer_t *b, FILE *fp);
void buffer_save_to_file(buffer_t const *b, FILE *fp);

// Get Buffer Information
size_t buffer_get_cursor_row(buffer_t const *b);
size_t buffer_get_cursor_col(buffer_t const *b);
char buffer_get_char(buffer_t const *b);

#endif // BUFFER_H_
