#ifndef RENDERER_H_
#define RENDERER_H_

#include <GL/glew.h>

#include "la.h"
#include "lib.h"

#define RENDERER_VERTICES_CAP 640 * 1024

typedef struct {
    v2f_t uv;
    v2f_t pos;
    v4f_t color;
} vertex_t;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint program;
    vertex_t vertices[RENDERER_VERTICES_CAP];
    size_t vertex_count;
} renderer_t;

void renderer_init(renderer_t *r, slice_t vert_filenames, slice_t frag_filenames);

void renderer_vertex(renderer_t *r, v2f_t p, v4f_t c, v2f_t uv);

void renderer_triangle(
        renderer_t *r, v2f_t p0, v2f_t p1, v2f_t p2, v4f_t c0, v4f_t c1, v4f_t c2,
        v2f_t uv0, v2f_t uv1, v2f_t uv2);

void renderer_quad(
        renderer_t *r, v2f_t p0, v2f_t p1, v2f_t p2, v2f_t p3, v4f_t c0, v4f_t c1,
        v4f_t c2, v4f_t c3, v2f_t uv0, v2f_t uv1, v2f_t uv2, v2f_t uv3);

void renderer_solid_rect(renderer_t *r, v2f_t p, v2f_t size, v4f_t c);

void renderer_image_rect(renderer_t *r, v2f_t p, v2f_t size, v2f_t uvp, v2f_t uvs, v4f_t c);

void renderer_draw(renderer_t *r);

void renderer_use(renderer_t const *r);
void renderer_uniform1f(renderer_t const *r, char const *u_name, float f);
void renderer_uniform2f(renderer_t const *r, char const *u_name, float f, float g);
void renderer_uniform4f(
        renderer_t const *r, char const *u_name, float f, float g, float h, float i);

#endif // RENDERER_H_
