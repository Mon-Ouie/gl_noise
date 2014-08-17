#include "noise_gen.h"
#include "shader_utils.h"

#include <stdlib.h>
#include <stddef.h>
#include <GL/glew.h>
#include <stdio.h>

static void make_permutation_table(GLint *array, size_t n);
static void shuffle(GLint *array, size_t n);

#define GLSL(code) \
  "#version 430\n" \
  #code

void single_cell(size_t width, size_t height, size_t depth, float *noise,
                 size_t x, size_t y, size_t z) {
  for (size_t i = 0; i < width*height*depth; i++)
    noise[i] = 0.0;
  noise[x + y*width + z*width*height] = 1.0;
}

void white_noise(size_t width, size_t height, size_t depth, float *noise) {
  for (size_t i = 0; i < width*height*depth; i++)
    noise[i] = (float)rand()/RAND_MAX;
}

const char *src_perlin3d = GLSL(
  layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

  layout(packed, binding = 0) buffer inBuf {
    vec3 gradients[12];
    int permutations[512];
  };

  /* Couldn't get this to work using an image3D, not sure what I was doing
   * wrong.  */
  layout(std430, binding = 1) buffer outBuf {
    float data[];
  };

  uniform ivec3 size;

  uniform vec3 start;
  uniform vec3 scale;

  uniform int octave_count;

  float perlin_smoothstep(float t) {
    return t*t*t*(t*(t*6 - 15) + 10);
  }

  int gradient_index(ivec3 pos) {
    int hash_z = permutations[pos.z];
    int hash_y = permutations[pos.y + hash_z];
    int hash_x = permutations[pos.x + hash_y];
    return hash_x % 12;
  }

  float perlin_noise(vec3 pos) {
    ivec3 cell = ivec3(pos);
    ivec3 cell_mod256 = cell % 256;

    vec3 dist[8];
    float noises[8];

    int i = 0;
    for (int z = 0; z < 2; z++) {
      for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
          int g = gradient_index(cell_mod256 + ivec3(x,y,z));

          dist[i]   = pos - vec3(cell + ivec3(x,y,z));
          noises[i] = dot(dist[i], gradients[g]);

          i++;
        }
      }
    }

    float t = perlin_smoothstep(dist[0].x);
    float u = perlin_smoothstep(dist[0].y);
    float v = perlin_smoothstep(dist[0].z);

    for (int i = 0; i < 4; i++)
      noises[i] = mix(noises[i], noises[i+4], v);
    for (int i = 0; i < 2; i++)
      noises[i] = mix(noises[i], noises[i+2], u);
    return 16*mix(noises[0], noises[1], t);

  }

  float multioctave_noise(int n, vec3 pos) {
    float ret = 0.0;
    float factor = 1.0;
    float norm = 0.0;
    for (int i = 0; i < n; i++) {
      float amplitude = 1.0 / factor;
      ret += amplitude * perlin_noise(pos * factor);
      norm += amplitude;
      factor *= 2.0;
    }

    return ret / norm;
  }

  void main() {
    ivec3 image_pos = ivec3(gl_GlobalInvocationID);
    vec3  noise_pos = vec3(start) + vec3(image_pos)*vec3(scale);

    data[image_pos.x + size.x*image_pos.y + size.x*size.y*image_pos.z] =
      multioctave_noise(octave_count, noise_pos);
  }
);

#define PermutationTableSize 256

const float gradients[] = {
   1,  1,  0,
  -1,  1,  0,
   1, -1,  0,
  -1, -1,  0,
   1,  0,  1,
  -1,  0,  1,
   1,  0, -1,
  -1,  0, -1,
   0,  1,  1,
   0, -1,  1,
   0,  1, -1,
   0, -1, -1,
};

void perlin3d(size_t width, size_t height, size_t depth, float *noise,
               size_t octave_count, vec3 start, vec3 scale) {
  GLint permutations[PermutationTableSize];
  make_permutation_table(permutations, PermutationTableSize);

  GLuint shader_input;
  glGenBuffers(1, &shader_input);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shader_input);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               sizeof(gradients) + 2*sizeof(permutations),
               NULL, GL_STATIC_READ);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(gradients), gradients);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(gradients),
                  sizeof(permutations), permutations);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                  sizeof(gradients) + sizeof(permutations),
                  sizeof(permutations), permutations);

  GLuint shader_output;
  glGenBuffers(1, &shader_output);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shader_output);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*width*height*depth,
               NULL, GL_STREAM_READ);

  GLuint shader = create_shader(GL_COMPUTE_SHADER, src_perlin3d);
  GLuint prog = glCreateProgram();
  glAttachShader(prog, shader);
  glLinkProgram(prog);
  check_link_errors(prog);

  glUseProgram(prog);

  glUniform3i(glGetUniformLocation(prog, "size"), width, height, depth);
  glUniform3f(glGetUniformLocation(prog, "start"), start.x, start.y, start.z);
  glUniform3f(glGetUniformLocation(prog, "scale"), scale.x, scale.y, scale.z);
  glUniform1i(glGetUniformLocation(prog, "octave_count"), octave_count);
  glUniform1i(glGetUniformLocation(prog, "outTex"), 0);

  glDispatchCompute(width, height, depth);
  glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                     sizeof(float)*width*height*depth, noise);
  glUseProgram(0);

  glDeleteProgram(prog);
  glDeleteShader(shader);

  glDeleteBuffers(1, &shader_output);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  glDeleteBuffers(1, &shader_input);
}

static void make_permutation_table(GLint *array, size_t n) {
  for (size_t i = 0; i < n; i++) array[i] = i;
  shuffle(array, n);
}

static void shuffle(GLint *array, size_t n) {
  for (size_t i = n; i > 0; i--) {
    size_t j = rand() % i;
    GLint tmp = array[i-1];
    array[i-1] = array[j];
    array[j] = tmp;
  }
}
