#ifndef PROGRAM_OBJECT_H_
#define PROGRAM_OBJECT_H_

#include <GL/glew.h>

bool program_object_link(
        GLuint *program, char const **vert_filenames, size_t vert_filename_count,
        char const **frag_filenames, size_t frag_filename_count);

void program_object_use(GLuint program);
void program_object_uniform1f(GLuint program, char const *uniform_name, float f);
void program_object_uniform2f(GLuint program, char const *uniform_name, float f, float g);

#endif // PROGRAM_OBJECT_H_
