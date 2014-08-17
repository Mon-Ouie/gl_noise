#ifndef NOISE_RENDERER_H_
#define NOISE_RENDERER_H_

#include "vector_math.h"
#include <GL/glew.h>
#include <stdint.h>

#define LevelWidth  30
#define LevelHeight 30
#define LevelDepth  30

#define MaxCubeCount   (1+LevelWidth*LevelHeight*LevelDepth)
#define MaxSquareCount (MaxCubeCount*6)
#define MaxVertexCount (MaxSquareCount*4)
#define MaxIndexCount  (MaxSquareCount*6)

#define MaxVertexBufferSize (MaxVertexCount * sizeof(vertex))
#define MaxIndexBufferSize  (MaxIndexCount  * sizeof(GLuint))

#define DensityThreshold 0.5

typedef struct color {
  GLubyte r, g, b;
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

typedef enum noise_usage {
  NoiseConstant,
  NoiseAnimated,
} noise_usage;

void noise_renderer_init(noise_renderer *renderer, noise_usage usage);
void noise_renderer_release(noise_renderer *renderer);

/**
 * Generates one cube for every element in the noise buffer with a value above
 * the DensityThreshold, as well as a box around the whole scene.
 */
int generate_geometry(noise_renderer *renderer, GLfloat *noise);

void render(const noise_renderer *renderer);
void set_mvp(noise_renderer *renderer,
             mat4 model, mat4 view, mat4 projection);

#endif
