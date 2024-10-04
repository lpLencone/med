#include "cursor_renderer.h"

#include <stdio.h>
#include <stdlib.h>

#include "glslinker.h"
#include "lib.h"

static void cr_get_uniform_locations(GLuint program, GLint locations[CRU_COUNT]);

cursor_renderer_t cr_init(char const *vert_filename, char const *frag_filename)
{
    GLuint shader;
    if (!glslink_program(
                &shader,
                slice_from((char const *[]) { vert_filename, "shaders/project.glsl" }, 2),
                slice_from(&frag_filename, 1))) {
        panic("Could not initialize cursor shader.");
    }
    cursor_renderer_t cr = { .shader = shader };
    cr_get_uniform_locations(cr.shader, cr.u);
    return cr;
}

void cr_render(void)
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static char const *uniform_name(enum cruniform cru)
{
    switch (cru) {
        case CRU_POS:
            return "u_pos";
        case CRU_SIZE:
            return "u_size";
        case CRU_TIME:
            return "u_time";
        case CRU_CAMERA:
            return "u_camera";
        case CRU_SCALE:
            return "u_scale";
        case CRU_RESOLUTION:
            return "u_resolution";
        case CRU_LAST_MOVED:
            return "u_last_moved";
        default:
            panic("Unreachable");
    }
}

static_assert(CRU_COUNT == 7, "The amount of cursor renderer uniforms has changed.");

static void cr_get_uniform_locations(GLuint program, GLint locations[CRU_COUNT])
{
    for (enum cruniform cru = 0; cru < CRU_COUNT; cru++) {
        locations[cru] = glGetUniformLocation(program, uniform_name(cru));
    }
}
