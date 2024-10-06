#ifndef CURSOR_RENDERER_H_
#define CURSOR_RENDERER_H_

#include "renderer.h"

typedef struct {
    renderer_t r;
} cursor_renderer_t;

void cr_init(cursor_renderer_t *cr);
void cr_draw(
        cursor_renderer_t *cr, v2f_t cur_pos, v2f_t cur_size, v4f_t color);

void cr_set_time(cursor_renderer_t *cr, float time);
void cr_set_time_moved(cursor_renderer_t *cr, float time_moved);
void cr_set_scale(cursor_renderer_t *cr, float scale);
void cr_set_camera(cursor_renderer_t *cr, v2f_t camera);
void cr_set_resolution(cursor_renderer_t *cr, v2f_t resolution);

#endif // CURSOR_RENDERER_H_
