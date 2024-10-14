#ifndef FREETYPE_RENDERER_H_
#define FREETYPE_RENDERER_H_

#include <stdbool.h>

#include <GL/glew.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "la.h"
#include "renderer.h"

#define METRICS_LENGTH 128

enum ft_program {
    FTP_RAINBOW,
    FTP_COLOR,
    FTP_COUNT,
};

enum ft_uniform {
    FTU_TIME,
    FTU_RESOLUTION,
    FTU_CAMERA,
    FTU_SCALE,
    FTU_COUNT,
};

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
    renderer_t r;
    GLuint atlas;

    FT_UInt atlas_w;
    FT_UInt atlas_h;
    FT_UInt atlas_low;

    GLuint program[FTP_COUNT];
    enum ft_program current_program;

    ft_glyph_metrics_t metrics[METRICS_LENGTH];

    // TODO: check whether all of the uniforms were set at least once before the first
    // call to ftr_draw; panic otherwise
} ft_renderer_t;

void ftr_free(ft_renderer_t *ftr);
bool ftr_init(ft_renderer_t *ftr, FT_Face face);

void ftr_draw(ft_renderer_t *ftr);

v2f_t ftr_render_text(
        ft_renderer_t *ftr, char const *text, size_t text_size, v2f_t pos, v4f_t color);

v2f_t ftr_cursor_pos(ft_renderer_t *ftr, char const *text, size_t text_size);

float ftr_get_max_line_width(ft_renderer_t *ftr, char const *text, size_t text_size);

float ftr_char_width(ft_renderer_t *ftr, char c);

void ftr_use(ft_renderer_t *ftr, enum ft_program p);

#define ftr_set(ftr, u, p) \
    _Generic(p, float: ftr_set_float, v2f_t: ftr_set_v2f)(ftr, u, p)
void ftr_set_float(ft_renderer_t *ftr, enum ft_uniform u, float f);
void ftr_set_v2f(ft_renderer_t *ftr, enum ft_uniform u, v2f_t v);

#endif // FREETYPE_RENDERER_H_
