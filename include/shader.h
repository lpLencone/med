#ifndef SHADER_H_
#define SHADER_H_

#include <GL/glew.h>

#include <stdint.h>

#include "math_3d.h"

typedef struct {
    uint32_t program;
} shader_t;

bool shader_init(
        shader_t *s, char const *vert_filename, char const *frag_filename);
void shader_use(shader_t const *s);
void shader_uniform1i(shader_t const *s, char const *name, int value);
void shader_uniform1f(shader_t const *s, char const *name, float value);
void shader_uniform2f(shader_t const *s, char const *name, float v0, float v1);
void shader_uniform4f(
        shader_t const *s, char const *name, float v0, float v1, float v2,
        float v3);
void shader_uniform_v3f(shader_t const *s, char const *name, vec3_t v);
void shader_uniform_m4f(shader_t const *s, char const *name, mat4_t m);

#endif // SHADER_H_
