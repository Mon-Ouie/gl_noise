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

void single_cell(size_t width, size_t height, size_t depth, GLfloat *noise,
                 size_t x, size_t y, size_t z) {
  for (size_t i = 0; i < width*height*depth; i++)
    noise[i] = 0.0;
  noise[x + y*width + z*width*height] = 1.0;
}

void white_noise(size_t width, size_t height, size_t depth, GLfloat *noise) {
  for (size_t i = 0; i < width*height*depth; i++)
    noise[i] = (GLfloat)rand()/RAND_MAX;
}

static
void perlin3d_like(size_t width, size_t height, size_t depth, GLfloat *noise,
                   size_t octave_count, vec3 start, vec3 scale,
                   const char *src);

static const char *src_perlin3d = GLSL(
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

void perlin3d(size_t width, size_t height, size_t depth, GLfloat *noise,
               size_t octave_count, vec3 start, vec3 scale) {
  perlin3d_like(width, height, depth, noise, octave_count, start, scale,
                src_perlin3d);
}

static const char *src_simplex3d = GLSL(
  layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

  layout(packed, binding = 0) buffer inBuf {
    vec3 gradients[12];
    int permutations[512];
  };

  layout(std430, binding = 1) buffer outBuf {
    float data[];
  };

  uniform ivec3 size;

  uniform vec3 start;
  uniform vec3 scale;

  uniform int octave_count;

  int gradient_index(ivec3 pos) {
    int hash_z = permutations[pos.z];
    int hash_y = permutations[pos.y + hash_z];
    int hash_x = permutations[pos.x + hash_y];
    return hash_x % 12;
  }

  vec3 skew(vec3 p) {
    float s = (p.x+p.y+p.z)/3.0;
    return p + vec3(s);
  }

  vec3 unskew(vec3 p) {
    float s = (p.x+p.y+p.z)/6.0;
    return p - vec3(s);
  }

  void find_simplex(in vec3 rel, out ivec3 a, out ivec3 b) {
    if (rel.x >= rel.y) {
      if (rel.y >= rel.z) {
        a = ivec3(1, 0, 0);
        b = ivec3(1, 1, 0);
      }
      else if (rel.x >= rel.z) {
        a = ivec3(1, 0, 0);
        b = ivec3(1, 0, 1);
      }
      else {
        a = ivec3(0, 0, 1);
        b = ivec3(1, 0, 1);
      }
    }
    else {
      if (rel.y < rel.z) {
        a = ivec3(0, 0, 1);
        b = ivec3(0, 1, 1);
      }
      else if (rel.x < rel.z) {
        a = ivec3(0, 1, 0);
        b = ivec3(0, 1, 1);
      }
      else {
        a = ivec3(0, 1, 0);
        b = ivec3(1, 1, 0);
      }
    }
  }

  float contribution(vec3 dist, vec3 g) {
    float t = 0.6 - dot(dist,dist);
    return t > 0 ? 8*t*t*t*t*dot(dist,g) : 0;
  }

  float simplex_noise(vec3 pos) {
    ivec3 cell = ivec3(skew(pos));
    ivec3 cell_mod256 = cell % 256;

    vec3 dist[4];
    dist[0] = pos - unskew(vec3(cell));

    ivec3 a;
    ivec3 b;
    find_simplex(dist[0], a, b);

    ivec3 vertex[4];
    vertex[0] = ivec3(0);
    vertex[1] = a;
    vertex[2] = b;
    vertex[3] = ivec3(1);

    for (int i = 1; i < 4; i++)
      dist[i] = dist[0] - vertex[i] + vec3(i)/6.0;

    float ret = 0.0;
    for (int i = 0; i < 4; i++) {
      int g = gradient_index(cell_mod256 + ivec3(vertex[i]));
      ret += contribution(dist[i], gradients[g]);
    }
    return 16*ret;
  }

  float multioctave_noise(int n, vec3 pos) {
    float ret = 0.0;
    float factor = 1.0;
    float norm = 0.0;
    for (int i = 0; i < n; i++) {
      float amplitude = 1.0 / factor;
      ret += amplitude * simplex_noise(pos * factor);
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

void simplex3d(size_t width, size_t height, size_t depth, GLfloat *noise,
               size_t octave_count, vec3 start, vec3 scale) {
  perlin3d_like(width, height, depth, noise, octave_count, start, scale,
                src_simplex3d);
}

static const char *src_perlin4d = GLSL(
  layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

  layout(packed, binding = 0) buffer inBuf {
    vec4 gradients[32];
    int permutations[512];
  };

  layout(std430, binding = 1) buffer outBuf {
    float data[];
  };

  uniform ivec3 size;

  uniform vec4 start;
  uniform vec4 scale;

  uniform float slice_w;

  uniform int octave_count;

  float perlin_smoothstep(float t) {
    return t*t*t*(t*(t*6 - 15) + 10);
  }

  int gradient_index(ivec4 pos) {
    int hash_w = permutations[pos.w];
    int hash_z = permutations[pos.z + hash_w];
    int hash_y = permutations[pos.y + hash_z];
    int hash_x = permutations[pos.x + hash_y];
    return hash_x % 32;
  }

  float perlin_noise(vec4 pos) {
    ivec4 cell = ivec4(pos);
    ivec4 cell_mod256 = cell % 256;

    vec4 dist[16];
    float noises[16];

    int i = 0;
    for (int w = 0; w < 2; w++) {
      for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
          for (int x = 0; x < 2; x++) {
            int g = gradient_index(cell_mod256 + ivec4(x,y,z,w));

            dist[i]   = pos - vec4(cell + ivec4(x,y,z,w));
            noises[i] = dot(dist[i], gradients[g]);

            i++;
          }
        }
      }
    }

    float t = perlin_smoothstep(dist[0].x);
    float u = perlin_smoothstep(dist[0].y);
    float v = perlin_smoothstep(dist[0].z);
    float w = perlin_smoothstep(dist[0].w);

    for (int i = 0; i < 8; i++)
      noises[i] = mix(noises[i], noises[i+8], w);
    for (int i = 0; i < 4; i++)
      noises[i] = mix(noises[i], noises[i+4], v);
    for (int i = 0; i < 2; i++)
      noises[i] = mix(noises[i], noises[i+2], u);
    return 16*mix(noises[0], noises[1], t);
  }

  float multioctave_noise(int n, vec4 pos) {
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
    vec4  noise_pos = start + vec4(image_pos*scale.xyz, 0) +
                      vec4(0, 0, 0, slice_w*scale.w);

    data[image_pos.x + size.x*image_pos.y + size.x*size.y*image_pos.z] =
      multioctave_noise(octave_count, noise_pos);
  }
);

static const GLfloat gradients4d[] = {
   0,  1,  1,  1,
   0,  1,  1, -1,
   0,  1, -1,  1,

   0, -1,  1,  1,
   0, -1,  1, -1,
   0, -1, -1,  1,
   0, -1, -1, -1,

   1,  0,  1,  1,
   1,  0,  1, -1,
   1,  0, -1,  1,
   1,  0, -1, -1,

  -1,  0,  1,  1,
  -1,  0,  1, -1,
  -1,  0, -1,  1,
  -1,  0, -1, -1,

   1,  1,  0,  1,
   1,  1,  0, -1,
   1, -1,  0,  1,
   1, -1,  0, -1,

  -1,  1,  0,  1,
  -1,  1,  0, -1,
  -1, -1,  0,  1,
  -1, -1,  0, -1,

   1,  1,  1,  0,
   1,  1, -1,  0,
   1, -1,  1,  0,
   1, -1, -1,  0,

  -1,  1,  1,  0,
  -1,  1, -1,  0,
  -1, -1,  1,  0,
  -1, -1, -1,  0
};

#define PermutationTableSize 256

void perlin4d_init(perlin4d_gen *gen,
                   size_t width, size_t height, size_t depth,
                   size_t octave_count, vec4 start, vec4 scale) {
  gen->width  = width;
  gen->height = height;
  gen->depth  = depth;

  GLint permutations[PermutationTableSize];
  make_permutation_table(permutations, PermutationTableSize);

  glGenBuffers(1, &gen->shader_input);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gen->shader_input);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               sizeof(gradients4d) + 2*sizeof(permutations),
               NULL, GL_STATIC_COPY);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(gradients4d),
                  gradients4d);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(gradients4d),
                  sizeof(permutations), permutations);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                  sizeof(gradients4d) + sizeof(permutations),
                  sizeof(permutations), permutations);

  glGenBuffers(1, &gen->shader_output);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gen->shader_output);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat)*width*height*depth,
               NULL, GL_STREAM_READ);

  gen->shader = create_shader(GL_COMPUTE_SHADER, src_perlin4d);
  gen->prog = glCreateProgram();
  glAttachShader(gen->prog, gen->shader);
  glLinkProgram(gen->prog);
  check_link_errors(gen->prog);

  glUseProgram(gen->prog);

  glUniform3i(glGetUniformLocation(gen->prog, "size"), width, height, depth);
  glUniform4f(glGetUniformLocation(gen->prog, "start"),
              start.x, start.y, start.z, start.w);
  glUniform4f(glGetUniformLocation(gen->prog, "scale"),
              scale.x, scale.y, scale.z, scale.w);
  glUniform1i(glGetUniformLocation(gen->prog, "octave_count"), octave_count);

  gen->slice_w = glGetUniformLocation(gen->prog, "slice_w");
  glUseProgram(0);
  /* glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); */
}

void perlin4d_release(perlin4d_gen *gen) {
  glDeleteProgram(gen->prog);
  glDeleteShader(gen->shader);

  glDeleteBuffers(1, &gen->shader_output);
  glDeleteBuffers(1, &gen->shader_input);
}

void perlin4d_slice(perlin4d_gen *gen, GLfloat w, GLfloat *noise) {
  glUseProgram(gen->prog);

  /* glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gen->shader_input); */
  /* glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gen->shader_output); */
  glUniform1f(gen->slice_w, w);

  glDispatchCompute(gen->width, gen->height, gen->depth);
  glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                     sizeof(GLfloat)*gen->width*gen->height*gen->depth,
                     noise);
  /* glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); */
  /* glUseProgram(0); */
}

const GLfloat gradients[] = {
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

static
void perlin3d_like(size_t width, size_t height, size_t depth, GLfloat *noise,
                   size_t octave_count, vec3 start, vec3 scale,
                   const char *src) {
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
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat)*width*height*depth,
               NULL, GL_STREAM_READ);

  GLuint shader = create_shader(GL_COMPUTE_SHADER, src);
  GLuint prog = glCreateProgram();
  glAttachShader(prog, shader);
  glLinkProgram(prog);
  check_link_errors(prog);

  glUseProgram(prog);

  glUniform3i(glGetUniformLocation(prog, "size"), width, height, depth);
  glUniform3f(glGetUniformLocation(prog, "start"), start.x, start.y, start.z);
  glUniform3f(glGetUniformLocation(prog, "scale"), scale.x, scale.y, scale.z);
  glUniform1i(glGetUniformLocation(prog, "octave_count"), octave_count);

  glDispatchCompute(width, height, depth);
  glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                     sizeof(GLfloat)*width*height*depth, noise);
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
