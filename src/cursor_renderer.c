#include "program_object.h"
#include "cursor_renderer.h"
#include "lib.h"

void cr_init(cursor_renderer_t *cr)
{
    char const *vert_filenames[] = { "shaders/cursor.vert", "shaders/project.glsl" };
    char const *frag_filename = "shaders/cursor.frag";
    program_object_link(&cr->program, vert_filenames, 2, &frag_filename, 1);
    renderer_init(&cr->r);
}

void cr_draw(cursor_renderer_t *cr, v2f_t cur_pos, v2f_t cur_size, v4f_t color)
{
    renderer_solid_rect(&cr->r, cur_pos, cur_size, color);
    renderer_draw(&cr->r);
}

void cr_use(cursor_renderer_t const *cr)
{
    program_object_use(cr->program);
}

static char const *uniform_name(enum cursor_uniform u);

void cr_set_float(cursor_renderer_t const *cr, enum cursor_uniform u, float f)
{
    cr_use(cr);
    program_object_uniform1f(cr->program, uniform_name(u), f);
}

void cr_set_v2f(cursor_renderer_t const *cr, enum cursor_uniform u, v2f_t v)
{
    cr_use(cr);
    program_object_uniform2f(cr->program, uniform_name(u), v2(v));
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
