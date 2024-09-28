#ifndef SHADER_H_
#define SHADER_H_

#include <GL/glew.h>

bool shader_init(GLuint *shader, char const *vert_filename, char const *frag_filename);

#endif // SHADER_H_
