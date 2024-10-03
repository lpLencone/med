#ifndef GLSLINKER_H_
#define GLSLINKER_H_

#include <GL/glew.h>

#include "lib.h"

bool glslink_program(GLuint *program, slice_t vert_filenames, slice_t frag_filenames);

#endif // GLSLINKER_H_
