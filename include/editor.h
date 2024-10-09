#ifndef EDITOR_H_
#define EDITOR_H_

#include <stddef.h>

#include "str.h"

// It would be nice to insert text anywhere in a buffer regardless of cursor position.

typedef struct {
    str_t buffer;
    size_t cursor;
    str_t pathname;

    bool dired;
    size_t dired_entry_count;
} editor_t;

void editor_free(editor_t *e);
void editor_new(editor_t *e);

// Moving
void editor_forward_char(editor_t *e);
void editor_backward_char(editor_t *e);
void editor_move_end_of_line(editor_t *e);
void editor_move_beginning_of_line(editor_t *e);
void editor_next_line(editor_t *e);
void editor_previous_line(editor_t *e);

// Editing
void editor_insert_text(editor_t *e, char const *text, size_t text_size);
void editor_delete_backward_char(editor_t *e);
void editor_delete_char(editor_t *e);
void editor_newline(editor_t *e);

// File I/O
void editor_load_file(editor_t *e);
void editor_save_buffer(editor_t *e);

// Dired functions

void editor_dired(editor_t *e);
void editor_dired_find_file(editor_t *e);

// Get editor Information
size_t editor_get_line_count(editor_t const *e);
size_t editor_get_cursor_row(editor_t const *e);
size_t editor_get_cursor_col(editor_t const *e);
char editor_get_char(editor_t const *e);
size_t editor_nth_char_index(editor_t const *e, char c, size_t nth);

#endif // EDITOR_H_
