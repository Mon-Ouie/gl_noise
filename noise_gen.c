#include "noise_gen.h"

void single_cell(size_t width, size_t height, size_t depth, float *noise,
                 size_t x, size_t y, size_t z) {
  for (size_t i = 0; i < width*height*depth; i++)
    noise[i] = 0.0;
  noise[x + y*width + z*width*height] = 1.0;
}
