#include "shader.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "str.h"

bool shader_init(shader_t *s, char const *vert_filename, char const *frag_filename)
{
    int status;
    char info_log[1024];
    FILE *fp = NULL;

    str_t shader_source = {0};
    assert((fp = fopen(vert_filename, "r")) != NULL);
    str_load_file(&shader_source, fp);
    fclose(fp);
    GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
    // glShaderSource(shader object to compile, source count, source array,
    // flags)
    glShaderSource(vert_shader, 1, (GLchar const **) &shader_source.data, NULL);
    glCompileShader(vert_shader);

    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderInfoLog(vert_shader, 512, NULL, info_log);
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n\t%s\n", info_log);
        return false;
    }
    str_free(&shader_source);

    assert((fp = fopen(frag_filename, "r")) != NULL);
    str_load_file(&shader_source, fp);
    fclose(fp);
    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, (GLchar const **) &shader_source.data, NULL);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n\t%s\n", info_log);
        return false;
    }

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vert_shader);
    glAttachShader(shader_program, frag_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &status);

    if (!status) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n\t%s\n", info_log);
        return false;
    }

    s->program = shader_program;
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    return true;
}

void shader_use(shader_t const *s)
{
    glUseProgram(s->program);
}

void shader_uniform1i(shader_t const *s, char const *name, int value)
{
    glUniform1i(glGetUniformLocation(s->program, name), value);
}

void shader_uniform1f(shader_t const *s, char const *name, float value)
{
    glUniform1f(glGetUniformLocation(s->program, name), value);
}

void shader_uniform2f(shader_t const *s, char const *name, float v0, float v1)
{
    glUniform2f(glGetUniformLocation(s->program, name), v0, v1);
}

void shader_uniform2i(shader_t const *s, char const *name, int v0, int v1)
{
    glUniform2i(glGetUniformLocation(s->program, name), v0, v1);
}

void shader_uniform4f(
        shader_t const *s, char const *name, float v0, float v1, float v2, float v3)
{
    glUniform4f(glGetUniformLocation(s->program, name), v0, v1, v2, v3);
}

void shader_uniform_v3f(shader_t const *s, char const *name, v3f_t v)
{
    glUniform3fv(glGetUniformLocation(s->program, name), 1, (GLfloat *) &v);
}

// void shader_uniform_m4f(shader_t const *s, char const *name, mat4_t m)
// {
//     glUniformMatrix4fv(
//             glGetUniformLocation(s->program, name), 1, GL_FALSE, (GLfloat const *) m.m);
// }
