#ifndef FREETYPE_RENDERER_H_
#define FREETYPE_RENDERER_H_

#include "glslinker.h"
#include "la.h"

#include <GL/glew.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

enum ft_uniform {
    FTU_TIME,
    FTU_RESOLUTION,
    FTU_CAMERA,
    FTU_SCALE,
    FTU_COUNT,
};

typedef struct {
    v2f_t pos;
    v2f_t size;
    v2f_t uv_pos;
    v2f_t uv_size;
    v4f_t fg;
    v4f_t bg;
} ft_glyph_t;

typedef struct {
    float ax; // advance.x
    float ay; // advance.y
    float bw; // bitmap.width;
    float bh; // bitmap.rows;
    float bl; // bitmap_left;
    float bt; // bitmap_top;
    float tx; // x offset of glyph in texture coordinates
} ft_glyph_metrics_t;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint atlas;
    GLuint shader;

    FT_UInt atlas_w;
    FT_UInt atlas_h;
    FT_UInt atlas_low;

#define GLYPH_BUFFER_CAP 1024 * 1024
    ft_glyph_t buffer[GLYPH_BUFFER_CAP];
    size_t count;

    ft_glyph_metrics_t metrics[128];

    // TODO: check whether all of the uniforms were set at least once before the first call to ftr_flush; panic otherwise
} ft_renderer_t;

void ftr_init(
        ft_renderer_t *ftr, FT_Face face, char const *vert_filename,
        char const *frag_filename);

void ftr_flush(ft_renderer_t *ftr);

void ftr_render_text(
        ft_renderer_t *ftr, char const *text, size_t text_size, v2f_t pos, v4f_t fg,
        v4f_t bg);

v2f_t ftr_cursor_pos(ft_renderer_t *ftr, char const *text, size_t text_size);

float ftr_get_widest_line_width(
        ft_renderer_t *ftr, char const *text, size_t text_size, size_t line_start,
        size_t line_count);

float ftr_char_width(ft_renderer_t *ftr, char c);

void ftr_use(ft_renderer_t const *ftr);

#define ftr_set(ftr, u, p) \
    _Generic(p, float: ftr_set_float, v2f_t: ftr_set_v2f)(ftr, u, p)

void ftr_set_float(ft_renderer_t const *ftr, enum ft_uniform u, float f);
void ftr_set_v2f(ft_renderer_t const *ftr, enum ft_uniform u, v2f_t v);

#endif // FREETYPE_RENDERER_H_
