#ifndef EDITOR_H_
#define EDITOR_H_

#include <stddef.h>

#include "str.h"

typedef struct editor editor_t;
typedef void (*editor_callback_fn)(editor_t *);

typedef struct editor {
    // TODO: handle many buffers
    str_t *buffer;      // Points to the currently focused buffer
    size_t *cursor;     // Cursor into the currently focused buffer

    str_t text_buffer;
    size_t text_cursor;

    str_t pathname;

    bool mark_set;
    size_t mark;

    bool fsnav;
    size_t fsnav_entry_count;

    bool mini;
    char const *miniprompt;
    str_t minibuffer;
    size_t minicursor;
    editor_callback_fn minicallback;
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
void editor_self_insert(editor_t *e, char c);
void editor_delete_backward_char(editor_t *e);
void editor_delete_char(editor_t *e);
void editor_newline(editor_t *e);
void editor_set_mark(editor_t *e);
void editor_reset(editor_t *e);

// Minibuffer
void editor_minibuffer_terminate(editor_t *e);

// I-Search
void editor_isearch(editor_t *e);

// File I/O
void editor_load_file(editor_t *e, char const *filename);
void editor_save_buffer(editor_t *e);

// fsnav functions
void editor_fsnav(editor_t *e);
void editor_fsnav_find_file(editor_t *e);

// Get editor Information
size_t editor_get_line_count(editor_t const *e);
size_t editor_get_cursor_row(editor_t const *e);
size_t editor_get_cursor_col(editor_t const *e);
char editor_get_char(editor_t const *e);
size_t editor_nth_char_index(editor_t const *e, char c, size_t nth);

#endif // EDITOR_H_
