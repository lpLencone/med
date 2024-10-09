#include "renderer.h"

#include <stdbool.h>

#include "lib.h"

enum vertex_attr {
    VERTEX_ATTR_UV = 0,
    VERTEX_ATTR_POS,
    VERTEX_ATTR_COLOR,
};

void renderer_free(renderer_t *r)
{
    glDeleteVertexArrays(1, &r->vao);
    glDeleteBuffers(1, &r->vbo);
}

void renderer_init(renderer_t *r)
{
    r->vertex_count = 0;

    glGenVertexArrays(1, &r->vao);
    glBindVertexArray(r->vao);

    glGenBuffers(1, &r->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof r->vertices, r->vertices, GL_DYNAMIC_DRAW);

#define gl_set_vertex_attribute(attr, buffer, member)                                    \
    do {                                                                                 \
        glEnableVertexAttribArray(attr);                                                 \
        glVertexAttribPointer(                                                           \
                attr, sizeof(*buffer).member / sizeof(*buffer).member.x, GL_FLOAT,       \
                GL_FALSE, sizeof *buffer, (GLvoid *) offsetof(typeof(*buffer), member)); \
    } while (false)

    gl_set_vertex_attribute(VERTEX_ATTR_POS, r->vertices, pos);
    gl_set_vertex_attribute(VERTEX_ATTR_COLOR, r->vertices, color);
    gl_set_vertex_attribute(VERTEX_ATTR_UV, r->vertices, uv);
#undef gl_set_vertex_attribute
}

void renderer_vertex(renderer_t *r, v2f_t p, v4f_t c, v2f_t uv)
{
    if (r->vertex_count >= RENDERER_VERTICES_CAP) {
        panic("Ran out of vertices");
    }
    vertex_t *v = r->vertices + r->vertex_count++;
    v->pos = p;
    v->color = c;
    v->uv = uv;
}

void renderer_triangle(
        renderer_t *r, v2f_t p0, v2f_t p1, v2f_t p2, v4f_t c0, v4f_t c1, v4f_t c2,
        v2f_t uv0, v2f_t uv1, v2f_t uv2)
{
    renderer_vertex(r, p0, c0, uv0);
    renderer_vertex(r, p1, c1, uv1);
    renderer_vertex(r, p2, c2, uv2);
}

// p3 - p2
//  | \ |
// p0 - p1
void renderer_quad(
        renderer_t *r, v2f_t p0, v2f_t p1, v2f_t p2, v2f_t p3, v4f_t c0, v4f_t c1,
        v4f_t c2, v4f_t c3, v2f_t uv0, v2f_t uv1, v2f_t uv2, v2f_t uv3)
{
    renderer_triangle(r, p0, p1, p3, c0, c1, c3, uv0, uv1, uv3);
    renderer_triangle(r, p1, p3, p2, c1, c3, c2, uv1, uv3, uv2);
}

void renderer_solid_rect(renderer_t *r, v2f_t p, v2f_t s, v4f_t c)
{
    v2f_t uv = v2fs(0);
    renderer_quad(
            r, p, v2f(p.x + s.x, p.y), v2f_add(p, s), v2f(p.x, p.y + s.y), c, c, c, c, uv,
            uv, uv, uv);
}

void renderer_image_rect(renderer_t *r, v2f_t p, v2f_t s, v2f_t uvp, v2f_t uvs, v4f_t c)
{
    renderer_quad(
            r, p, v2f(p.x + s.x, p.y), v2f_add(p, s), v2f(p.x, p.y + s.y), c, c, c, c,
            uvp, v2f(uvp.x + uvs.x, uvp.y), v2f_add(uvp, uvs), v2f(uvp.x, uvp.y + uvs.y));
}

void renderer_draw(renderer_t *r)
{
    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferSubData(
            GL_ARRAY_BUFFER, 0, r->vertex_count * sizeof *r->vertices, r->vertices);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) r->vertex_count);
    r->vertex_count = 0;
}
