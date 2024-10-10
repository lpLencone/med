#include "program_object.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "str.h"

static bool shader_compile(
        GLuint *shaders, char const **filenames, size_t filename_count,
        GLenum shader_type);
static bool program_link(GLuint *program, GLuint *shaders, size_t shader_count);

bool program_object_link(
        GLuint *program, char const **vert_filenames, size_t vert_filename_count,
        char const **frag_filenames, size_t frag_filename_count)
{
    bool ret = true;

    size_t shader_count = vert_filename_count + frag_filename_count;
    GLuint *shaders = malloc(sizeof *shaders * shader_count);
    size_t compiled_count = 0;

    if (!shader_compile(shaders, vert_filenames, vert_filename_count, GL_VERTEX_SHADER)) {
        defer(ret = false);
    }
    compiled_count += vert_filename_count;

    if (!shader_compile(
                shaders + vert_filename_count, frag_filenames, frag_filename_count,
                GL_FRAGMENT_SHADER)) {
        defer(ret = false);
    }
    compiled_count += frag_filename_count;

    ret = program_link(program, shaders, shader_count);

defer:
    for (size_t i = 0; i < compiled_count; i++) {
        glDeleteShader(shaders[i]);
    }
    free(shaders);

    return ret;
}

void program_object_use(GLuint program)
{
    glUseProgram(program);
}

void program_object_uniform1f(GLuint program, char const *uniform_name, float f)
{
    glUniform1f(glGetUniformLocation(program, uniform_name), f);
}

void program_object_uniform2f(GLuint program, char const *uniform_name, float f, float g)
{
    glUniform2f(glGetUniformLocation(program, uniform_name), f, g);
}

static bool shader_compile(
        GLuint *shaders, char const **filenames, size_t filename_count,
        GLenum shader_type)
{
    str_t shader_source = { 0 };
    for (size_t i = 0; i < filename_count; i++) {
        char const *filename = filenames[i];
        // printf("[Program Object :: shader_compile] Compiling file \"%s\"\n", filename);
        FILE *fp = fopen(filename, "r");
        assert(fp != NULL);
        str_load_file(&shader_source, fp);
        fclose(fp);

        shaders[i] = glCreateShader(shader_type);
        glShaderSource(shaders[i], 1, (GLchar const **) &shader_source.data, NULL);
        glCompileShader(shaders[i]);

        int status;
        char info_log[512] = { 0 };
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            glGetShaderInfoLog(shaders[i], 512, NULL, info_log);
            debugf("Could not compile shader %s: %s\n", filename, info_log);

            for (size_t j = 0; j < i; j++) {
                glDeleteShader(shaders[j]);
            }
            return false;
        }
        str_free(&shader_source);
    }
    return true;
}

static bool program_link(GLuint *program, GLuint *shaders, size_t shader_count)
{
    *program = glCreateProgram();
    for (size_t i = 0; i < shader_count; i++) {
        glAttachShader(*program, shaders[i]);
    }
    glLinkProgram(*program);

    int status;
    char info_log[512] = { 0 };
    glGetProgramiv(*program, GL_LINK_STATUS, &status);
    if (!status) {
        glGetProgramInfoLog(*program, 512, NULL, info_log);
        debugf("Could not link program: %s\n", info_log);
        return false;
    }

    return true;
}
