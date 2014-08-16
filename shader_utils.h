#ifndef SHADER_UTILS_H_
#define SHADER_UTILS_H_

#include <GL/glew.h>

/**
 * Compiles the shader and prints any errors.
 */
GLuint create_shader(GLenum mode, const char *src);

/**
 * This prints the program's info log if an error occured.
 */
void check_link_errors(GLuint prog);

#endif
