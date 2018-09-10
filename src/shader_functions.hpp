#pragma once

#include <GLES3/gl3.h>

GLuint load_shader(GLenum type, const char *shaderSrc);
bool create_program(GLuint& prog, const char* vert, const char* frag);
