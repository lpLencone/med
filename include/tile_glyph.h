#ifndef TILE_GLYPH_H_
#define TILE_GLYPH_H_

#include "la.h"
#include "shader.h"

#include <GL/glew.h>

typedef struct {
    v2i_t tile;
    int ch;
    v4f_t fg;
    v4f_t bg;
} glyph_t;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint atlas;
    GLuint shader;

    GLint u_time;
    GLint u_resolution;
    GLint u_scale;
    GLint u_camera;

#define GLYPH_BUFFER_CAP 640 * 1024
    glyph_t buffer[GLYPH_BUFFER_CAP];
    size_t count;
} tile_glyph_buffer_t;

void tgb_init(
        tile_glyph_buffer_t *tgb, char const *atlas_filename, char const *vert_filename,
        char const *frag_filename);
void tgb_flush(tile_glyph_buffer_t *tgb);
void tgb_render_text(
        tile_glyph_buffer_t *tgb, char const *text, size_t text_size, v2i_t tile, v4f_t fg, v4f_t bg);

#endif // TILE_GLYPH_H_
