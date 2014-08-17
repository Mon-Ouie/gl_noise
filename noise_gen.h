#ifndef NOISE_GEN_H_
#define NOISE_GEN_H_

#include <stddef.h>
#include "vector_math.h"

void single_cell(size_t width, size_t height, size_t depth, float *noise,
                 size_t x, size_t y, size_t z);
void white_noise(size_t width, size_t height, size_t depth, float *noise);
void perlin3d(size_t width, size_t height, size_t depth, float *noise,
               size_t octave_count, vec3 start, vec3 scale);

#endif
