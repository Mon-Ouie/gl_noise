#ifndef NOISE_GEN_H_
#define NOISE_GEN_H_

#include <stddef.h>

void single_cell(size_t width, size_t height, size_t depth, float *noise,
                 size_t x, size_t y, size_t z);
void white_noise(size_t width, size_t height, size_t depth, float *noise);

#endif
