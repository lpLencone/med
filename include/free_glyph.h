#ifndef FREE_GLYPH_H_
#define FREE_GLYPH_H_

#include "la.h"
#include "glslinker.h"

#include <GL/glew.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

typedef struct {
    v2f_t pos;
    v2f_t size;
    v2f_t uv_pos;
    v2f_t uv_size;
    v4f_t fg;
    v4f_t bg;
} ftglyph_t;

typedef struct {
    float ax; // advance.x
    float ay; // advance.y
    float bw; // bitmap.width;
    float bh; // bitmap.rows;
    float bl; // bitmap_left;
    float bt; // bitmap_top;
    float tx; // x offset of glyph in texture coordinates
} ftglyph_metrics_t;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint atlas;
    GLuint shader;

    unsigned int atlas_w;
    unsigned int atlas_h;

    GLint u_time;
    GLint u_resolution;
    GLint u_scale;
    GLint u_camera;

#define GLYPH_BUFFER_CAP 640 * 1024
    ftglyph_t buffer[GLYPH_BUFFER_CAP];
    size_t count;

    ftglyph_metrics_t metrics[128];
} free_glyph_buffer_t;

void fgb_init(
        free_glyph_buffer_t *fgb, FT_Face face, char const *vert_filename,
        char const *frag_filename);

void fgb_flush(free_glyph_buffer_t *fgb);

void fgb_render_text(
        free_glyph_buffer_t *fgb, char const *text, size_t text_size, v2f_t pos, v4f_t fg,
        v4f_t bg);

v2f_t fgb_cursor_pos(
        free_glyph_buffer_t *fgb, char const *text, size_t text_size);

size_t fgb_char_width(free_glyph_buffer_t *fgb, char c);

#endif // FREE_GLYPH_H_
