#include "cursor_renderer.h"
#include "lib.h"

void cr_init(cursor_renderer_t *cr)
{
    char const *vert_filenames[] = { "shaders/cursor.vert", "shaders/project.glsl" };
    char const *frag_filename = "shaders/cursor.frag";
    renderer_init(&cr->r, slice_from(vert_filenames, 2), slice_from(&frag_filename, 1));
}

void cr_draw(cursor_renderer_t *cr, v2f_t cur_pos, v2f_t cur_size, v4f_t color)
{
    renderer_solid_rect(&cr->r, cur_pos, cur_size, color);
    renderer_draw(&cr->r);
}

void cr_set_time(cursor_renderer_t *cr, float time)
{
    renderer_uniform1f(&cr->r, "u_time", time);
}

void cr_set_time_moved(cursor_renderer_t *cr, float time_moved)
{
    renderer_uniform1f(&cr->r, "u_time_moved", time_moved);
}

void cr_set_scale(cursor_renderer_t *cr, float scale)
{
    renderer_uniform1f(&cr->r, "u_scale", scale);
}

void cr_set_camera(cursor_renderer_t *cr, v2f_t camera)
{
    renderer_uniform2f(&cr->r, "u_camera", v2(camera));
}

void cr_set_resolution(cursor_renderer_t *cr, v2f_t resolution)
{
    renderer_uniform2f(&cr->r, "u_resolution", v2(resolution));
}
