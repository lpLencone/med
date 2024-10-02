#ifndef CURSOR_RENDERER_H_
#define CURSOR_RENDERER_H_

#include <GL/glew.h>

typedef struct {
    GLuint shader;

    GLint u_pos;
    GLint u_size;
    GLint u_time;
    GLint u_camera;
    GLint u_resolution;
    GLint u_last_moved;
} cursor_renderer_t;

cursor_renderer_t cr_init(char const *vert_filename, char const *frag_filename);

void cr_render(void);

#endif // CURSOR_RENDERER_H_
