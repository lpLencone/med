#include "glslinker.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

static bool shader_compile(GLuint *shaders, slice_t filenames, GLenum shader_type);
static bool program_link(GLuint *program, slice_t shaders);

bool glslink_program(GLuint *program, slice_t vert_filenames, slice_t frag_filenames)
{
    size_t shader_count = vert_filenames.length + frag_filenames.length;
    GLuint *shaders = malloc(sizeof *shaders * shader_count);

    if (!shader_compile(shaders, vert_filenames, GL_VERTEX_SHADER)) {
        return false;
    }

    if (!shader_compile(
                shaders + vert_filenames.length, frag_filenames, GL_FRAGMENT_SHADER)) {
        for (size_t i = 0; i < vert_filenames.length; i++) {
            glDeleteShader(shaders[i]);
        }
        free(shaders);
        return false;
    }

    bool ret = program_link(program, slice_from(shaders, shader_count));

    for (size_t i = 0; i < shader_count; i++) {
        glDeleteShader(shaders[i]);
    }
    free(shaders);

    return ret;
}

static bool shader_compile(GLuint *shaders, slice_t filenames, GLenum shader_type)
{
    str_t shader_source = { 0 };
    for (size_t i = 0; i < filenames.length; i++) {
        char const *filename = ((char const **) filenames.data)[i];
        FILE *fp = fopen(filename, "r");
        assert(fp != NULL);
        str_load_file(&shader_source, fp);
        fclose(fp);

        eprintf("Compiling: %s\n", filename);

        shaders[i] = glCreateShader(shader_type);
        glShaderSource(shaders[i], 1, (GLchar const **) &shader_source.data, NULL);
        glCompileShader(shaders[i]);

        int status;
        char info_log[512] = { 0 };
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            glGetShaderInfoLog(shaders[i], 512, NULL, info_log);
            eprintf("Could not compile shader %s: %s\n", filename, info_log);

            for (size_t j = 0; j < i; j++) {
                glDeleteShader(shaders[j]);
            }
            return false;
        }
        str_free(&shader_source);
    }
    return true;
}

static bool program_link(GLuint *program, slice_t shaders)
{
    *program = glCreateProgram();
    for (size_t i = 0; i < shaders.length; i++) {
        glAttachShader(*program, ((GLuint const *) shaders.data)[i]);
    }
    glLinkProgram(*program);

    int status;
    char info_log[512] = { 0 };
    glGetProgramiv(*program, GL_LINK_STATUS, &status);
    if (!status) {
        glGetProgramInfoLog(*program, 512, NULL, info_log);
        eprintf("Could not link program: %s\n", info_log);
        return false;
    }

    return true;
}
