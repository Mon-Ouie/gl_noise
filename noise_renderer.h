#ifndef NOISE_RENDERER_H_
#define NOISE_RENDERER_H_

#include "vector_math.h"
#include <GL/glew.h>
#include <stdint.h>

#define LevelWidth  10
#define LevelHeight 10
#define LevelDepth  10

#define MaxCubeCount   (1+LevelWidth*LevelHeight*LevelDepth)
#define MaxSquareCount (MaxCubeCount*6)
#define MaxVertexCount (MaxSquareCount*4)
#define MaxIndexCount  (MaxSquareCount*6)

#define MaxVertexBufferSize (MaxVertexCount * sizeof(vertex))
#define MaxIndexBufferSize  (MaxIndexCount  * sizeof(GLushort))

#define DensityThreshold 0.5

typedef struct color {
  uint8_t r, g, b;
} color;

typedef struct vertex {
  vec3 pos;
  vec3 normal;
  color color;
} vertex;

typedef struct noise_renderer {
  GLuint prog, vs, fs;
  GLuint vbo, ibo, vao;
  size_t index_count;

  struct {
    GLint model_view;
    GLint projection;
    GLint normal_matrix;

    struct {
      GLint ambient;
      GLint diffuse;
      GLint specular;

      GLint shininess;
    } mat;

    struct {
      GLint pos;

      GLint ambient;
      GLint diffuse;
      GLint specular;
    } light;
  } uniforms;
} noise_renderer;

void noise_renderer_init(noise_renderer *renderer);
void noise_renderer_release(noise_renderer *renderer);

/**
 * Generates one cube for every element in the noise buffer with a value above
 * the DensityThreshold, as well as a box around the whole scene.
 */
int generate_geometry(noise_renderer *renderer, float *noise);

void render(const noise_renderer *renderer);
void set_mvp(noise_renderer *renderer,
             mat4 model, mat4 view, mat4 projection);

#endif
