#include "noise_gen.h"
#include "noise_renderer.h"

void single_cell(float *noise, size_t x, size_t y, size_t z) {
  for (size_t i = 0; i < LevelWidth*LevelHeight*LevelDepth; i++)
    noise[i] = 0.0;
  noise[x + y*LevelWidth + z*LevelWidth*LevelHeight] = 1.0;
}
