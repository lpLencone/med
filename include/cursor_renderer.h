#ifndef CURSOR_RENDERER_H_
#define CURSOR_RENDERER_H_

#include "renderer.h"

#include <stdbool.h>

enum cursor_uniform {
    CU_TIME,
    CU_TIME_MOVED,
    CU_SCALE,
    CU_CAMERA,
    CU_RESOLUTION,
    CU_COUNT,
};

typedef struct {
    renderer_t r;
    GLuint program;
} cursor_renderer_t;

void cr_free(cursor_renderer_t *cr);
bool cr_init(cursor_renderer_t *cr);

void cr_draw(cursor_renderer_t *cr, v2f_t cur_pos, v2f_t cur_size, v4f_t color);

void cr_use(cursor_renderer_t const *cr);
#define cr_set(cr, u, p) _Generic(p, float: cr_set_float, v2f_t: cr_set_v2f)(cr, u, p)
void cr_set_float(cursor_renderer_t const *cr, enum cursor_uniform u, float f);
void cr_set_v2f(cursor_renderer_t const *cr, enum cursor_uniform u, v2f_t v);

#endif // CURSOR_RENDERER_H_
