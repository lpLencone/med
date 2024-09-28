#include "shader.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "str.h"

bool shader_init(GLuint *shader, char const *vert_filename, char const *frag_filename)
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
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n\t%shader\n", info_log);
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

    *shader = glCreateProgram();
    glAttachShader(*shader, vert_shader);
    glAttachShader(*shader, frag_shader);
    glLinkProgram(*shader);
    glGetProgramiv(*shader, GL_LINK_STATUS, &status);

    if (!status) {
        glGetProgramInfoLog(*shader, 512, NULL, info_log);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n\t%s\n", info_log);
        return false;
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    return true;
}
