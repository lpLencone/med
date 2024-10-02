#include "cursor_renderer.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib.h"
#include "shader.h"

cursor_renderer_t
cr_init(char const *vert_filename, char const *frag_filename)
{
    GLuint shader;
    if (!shader_init(&shader, vert_filename, frag_filename)) {
        panic("Could not initialize cursor shader.");
    }

    return (cursor_renderer_t) {
        .shader = shader,
        .u_pos = glGetUniformLocation(shader, "u_pos"),
        .u_size = glGetUniformLocation(shader, "u_size"),
        .u_time = glGetUniformLocation(shader, "u_time"),
        .u_camera = glGetUniformLocation(shader, "u_camera"),
        .u_resolution = glGetUniformLocation(shader, "u_resolution"),
        .u_last_moved = glGetUniformLocation(shader, "u_last_moved"),
    };
}

void cr_render(void)
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
