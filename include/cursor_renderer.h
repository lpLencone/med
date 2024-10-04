#ifndef CURSOR_RENDERER_H_
#define CURSOR_RENDERER_H_

#include <GL/glew.h>

enum cruniform {
    CRU_POS,
    CRU_SIZE,
    CRU_TIME,
    CRU_CAMERA,
    CRU_SCALE,
    CRU_RESOLUTION,
    CRU_LAST_MOVED,
    CRU_COUNT,
};

typedef struct {
    GLuint shader;
    GLint u[CRU_COUNT];
} cursor_renderer_t;

cursor_renderer_t cr_init(char const *vert_filename, char const *frag_filename);

void cr_render(void);

#endif // CURSOR_RENDERER_H_
