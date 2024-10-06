#include "renderer.h"

#include "glslinker.h"
#include "lib.h"

enum vertex_attr {
    VERTEX_ATTR_UV = 0,
    VERTEX_ATTR_POS,
    VERTEX_ATTR_COLOR,
};

void renderer_init(renderer_t *r, slice_t vert_filenames, slice_t frag_filenames)
{
    r->vertex_count = 0;

    if (!glslink_program(&r->program, vert_filenames, frag_filenames)) {
        panic("Could not load freetype renderer shaders.");
    }

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

void renderer_solid_rect(renderer_t *r, v2f_t p, v2f_t size, v4f_t c)
{
    v2f_t uv = v2fs(0);
    renderer_quad(
            r, p, v2f(p.x + size.x, p.y), v2f_add(p, size), v2f(p.x, p.y + size.y), c, c,
            c, c, uv, uv, uv, uv);
}

void renderer_image_rect(renderer_t *r, v2f_t p, v2f_t size)
{
    v4f_t c = v4fs(0.0);
    renderer_quad(
            r, p, v2f(p.x + size.x, p.y), v2f_add(p, size), v2f(p.x, p.y + size.y), c, c,
            c, c, v2fs(0), v2f(1, 0), v2fs(1), v2f(0, 1));
}

void renderer_draw(renderer_t *r)
{
    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glUseProgram(r->program);
    glBufferSubData(
            GL_ARRAY_BUFFER, 0, r->vertex_count * sizeof *r->vertices, r->vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei) r->vertex_count);
    r->vertex_count = 0;
}

void renderer_uniform1f(renderer_t const *r, char const *u_name, float f)
{
    glUseProgram(r->program);
    glUniform1f(glGetUniformLocation(r->program, u_name), f);
}

void renderer_uniform2f(renderer_t const *r, char const *u_name, float f, float g)
{
    glUseProgram(r->program);
    glUniform2f(glGetUniformLocation(r->program, u_name), f, g);
}
