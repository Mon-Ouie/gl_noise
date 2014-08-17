#ifndef NOISE_GEN_H_
#define NOISE_GEN_H_

#include <stddef.h>
#include <GL/glew.h>
#include "vector_math.h"

void single_cell(size_t width, size_t height, size_t depth, GLfloat *noise,
                 size_t x, size_t y, size_t z);
void white_noise(size_t width, size_t height, size_t depth, GLfloat *noise);
void perlin3d(size_t width, size_t height, size_t depth, GLfloat *noise,
               size_t octave_count, vec3 start, vec3 scale);
void simplex3d(size_t width, size_t height, size_t depth, GLfloat *noise,
               size_t octave_count, vec3 start, vec3 scale);

typedef struct perlin4d_gen {
  GLuint prog;
  GLuint shader;

  GLuint shader_input, shader_output;
  size_t width, height, depth;

  GLint slice_w;
} perlin4d_gen;

void perlin4d_init(perlin4d_gen *gen,
                   size_t width, size_t height, size_t depth,
                   size_t octave_count, vec4 start, vec4 scale);
void perlin4d_release(perlin4d_gen *gen);

void perlin4d_slice(perlin4d_gen *gen, GLfloat w, GLfloat *noise);

#endif
