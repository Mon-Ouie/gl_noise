#include <string.h>
#include <stdio.h>

#include "shader_utils.h"

GLuint create_shader(GLenum mode, const char *src) {
  GLuint ret = glCreateShader(mode);
  GLint length = strlen(src);
  glShaderSource(ret, 1, &src, &length);
  glCompileShader(ret);

  int status;
  char error[256];
  glGetShaderiv(ret, GL_COMPILE_STATUS, &status);
  glGetShaderInfoLog(ret, sizeof(error), 0, error);
  if (!status)
    fprintf(stderr, "compilation error: %s\n", error);

  return ret;
}

void check_link_errors(GLuint prog) {
  int status;
  char error[256];
  glGetProgramiv(prog, GL_LINK_STATUS, &status);
  glGetProgramInfoLog(prog, sizeof(error), 0, error);
  if (!status)
    fprintf(stderr, "Linking error: %s\n", error);
}
