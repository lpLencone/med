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

void cr_use(cursor_renderer_t const *cr)
{
    renderer_use(&cr->r);    
}

static char const *uniform_name(enum cursor_uniform u);

void cr_set_float(cursor_renderer_t const *cr, enum cursor_uniform u, float f)
{
    renderer_uniform1f(&cr->r, uniform_name(u), f);
}

void cr_set_v2f(cursor_renderer_t const *cr, enum cursor_uniform u, v2f_t v)
{
    renderer_uniform2f(&cr->r, uniform_name(u), v2(v));
}

static char const *uniform_name(enum cursor_uniform u)
{
    switch (u) {
        case CU_TIME:
            return "u_time";
        case CU_TIME_MOVED:
            return "u_time_moved";
        case CU_SCALE:
            return "u_scale";
        case CU_CAMERA:
            return "u_camera";
        case CU_RESOLUTION:
            return "u_resolution";
        default:
            panic("Unreachable");
    }
}
