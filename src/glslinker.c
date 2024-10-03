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
        // TODO: delete compiled vertex shaders
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

static bool shader_source_link(str_t *source_str)
{
    strview_t source = sv_from_str(source_str);
    while (sv_token_subcstr(&source, "#use", NULL)) {
        strview_t directive_line = { 0 }, filename = { 0 };
        sv_token_cspn_consume(&source, "\n", &directive_line);
        strview_t directive_line_copy = directive_line;
        if (!sv_token_cspn_consume(&directive_line, " \"", NULL) ||
            !(sv_token_cspn(&directive_line, "\"", &filename))) {
            eprintln("%.*s", SVARG(directive_line_copy));
            eprintln("^ A filename must come after #include directive.");
            return false;
        }
        FILE *fp = sv_fopen(filename, "r");
        if (fp == NULL) {
            panic("Could not open file %.*s: %s", SVARG(filename), strerror(errno));
        }
        assert(fp != NULL);
        str_t included_source = { 0 };
        str_load_file(&included_source, fp);
        fclose(fp);

        size_t include_index = directive_line_copy.data - source_str->data;
        str_remove(source_str, directive_line_copy.length, include_index);
        str_insert(
                source_str, included_source.data, included_source.length, include_index);

        source.length += included_source.length;

        str_free(&included_source);
    }

    return true;
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

        shader_source_link(&shader_source);

        shaders[i] = glCreateShader(shader_type);
        // glShaderSource(shader to compile, source count, source array, flags)
        glShaderSource(shaders[i], 1, (GLchar const **) &shader_source.data, NULL);
        glCompileShader(shaders[i]);

        int status;
        char info_log[512] = { 0 };
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            glGetShaderInfoLog(shaders[i], 512, NULL, info_log);
            eprintln("Could not compile shader %s: %s", filename, info_log);
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
        eprintln("Could not link program: %s", info_log);
        return false;
    }

    return true;
}
