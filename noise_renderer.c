#include <stdlib.h>
#include <stddef.h>

#include "noise_renderer.h"
#include "shader_utils.h"

#define GLSL(code) \
  "#version 330\n"   \
  #code

const char *src_main_vs = GLSL(
  in vec3 pos;
  in vec3 normal;
  in vec3 color;

  struct light_source {
    vec3 pos;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
  };

  struct material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float shininess;
  };

  uniform light_source light;
  uniform material mat;

  out vec3 frag_base_color;

  out vec3 frag_normal;
  out vec3 frag_pos;
  out vec3 frag_light;

  uniform mat4 model_view;
  uniform mat4 projection;
  uniform mat3 normal_matrix;

  void main() {
    vec4 pos_rel_to_eye = model_view * vec4(pos, 1);

    frag_base_color = color;

    frag_normal = normalize(normal_matrix * normal);
    frag_pos    = -pos_rel_to_eye.xyz;
    frag_light  = light.pos + frag_pos;

    gl_Position  = projection * pos_rel_to_eye;
  }
);

const char *src_main_fs = GLSL(
  in vec3 frag_normal;
  in vec3 frag_light;
  in vec3 frag_pos;
  in vec3 frag_base_color;

  out vec4 frag_color;

  struct light_source {
    vec3 pos;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
  };

  struct material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float shininess;
  };

  uniform light_source light;
  uniform material mat;

  void main() {
    vec3 n = normalize(frag_normal);
    vec3 l = normalize(frag_light);
    vec3 v = normalize(frag_pos);
    vec3 r = reflect(-l, n);

    vec3 ambient = mat.ambient * light.ambient;
    vec3 diffuse = max(dot(n, l), 0.0) * light.diffuse * mat.diffuse;
    vec3 specular = pow(max(dot(r, v), 0.0), mat.shininess) *
      mat.specular * light.specular;

    frag_color = vec4(frag_base_color * (ambient + diffuse + specular), 1);
  }
);

void noise_renderer_init(noise_renderer *renderer) {
  glGenVertexArrays(1, &renderer->vao);
  glBindVertexArray(renderer->vao);

  glGenBuffers(1, &renderer->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
  glBufferData(GL_ARRAY_BUFFER, MaxVertexBufferSize, NULL, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                        (void*)offsetof(vertex, pos));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                        (void*)offsetof(vertex, normal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex),
                        (void*)offsetof(vertex, color));

  glGenBuffers(1, &renderer->ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxIndexBufferSize, NULL,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  renderer->vs = create_shader(GL_VERTEX_SHADER, src_main_vs);
  renderer->fs = create_shader(GL_FRAGMENT_SHADER, src_main_fs);
  renderer->prog = glCreateProgram();
  glAttachShader(renderer->prog, renderer->vs);
  glAttachShader(renderer->prog, renderer->fs);
  glBindFragDataLocation(renderer->prog, 0, "frag_color");
  glBindAttribLocation(renderer->prog, 0, "pos");
  glBindAttribLocation(renderer->prog, 1, "normal");
  glBindAttribLocation(renderer->prog, 2, "color");
  glLinkProgram(renderer->prog);
  check_link_errors(renderer->prog);

  renderer->uniforms.model_view = glGetUniformLocation(renderer->prog,
                                                      "model_view");
  renderer->uniforms.projection = glGetUniformLocation(renderer->prog,
                                                      "projection");
  renderer->uniforms.normal_matrix = glGetUniformLocation(renderer->prog,
                                                         "normal_matrix");

  renderer->uniforms.mat.ambient = glGetUniformLocation(renderer->prog,
                                                       "mat.ambient");
  renderer->uniforms.mat.diffuse = glGetUniformLocation(renderer->prog,
                                                       "mat.diffuse");
  renderer->uniforms.mat.specular = glGetUniformLocation(renderer->prog,
                                                        "mat.specular");

  renderer->uniforms.mat.shininess = glGetUniformLocation(renderer->prog,
                                                         "mat.shininess");

  renderer->uniforms.light.pos = glGetUniformLocation(renderer->prog,
                                                     "light.pos");

  renderer->uniforms.light.ambient = glGetUniformLocation(renderer->prog,
                                                         "light.ambient");
  renderer->uniforms.light.diffuse = glGetUniformLocation(renderer->prog,
                                                         "light.diffuse");
  renderer->uniforms.light.specular = glGetUniformLocation(renderer->prog,
                                                          "light.specular");

  glUseProgram(renderer->prog);

  glUniform3f(renderer->uniforms.mat.ambient, 0.1, 0.1, 0.1);
  glUniform3f(renderer->uniforms.light.ambient, 1, 1, 1);

  glUniform3f(renderer->uniforms.mat.diffuse, 0.2, 0.2, 0.2);
  glUniform3f(renderer->uniforms.light.diffuse, 1, 1, 1);

  glUniform3f(renderer->uniforms.mat.specular, 0.7, 0.7, 0.7);
  glUniform3f(renderer->uniforms.light.specular, 1, 1, 1);

  glUniform1f(renderer->uniforms.mat.shininess, 128);

  glUseProgram(0);
}

void noise_renderer_release(noise_renderer *renderer) {
  glDeleteProgram(renderer->prog);
  glDeleteShader(renderer->fs);
  glDeleteShader(renderer->vs);

  glDeleteVertexArrays(1, &renderer->vao);
  glDeleteBuffers(1, &renderer->ibo);
  glDeleteBuffers(1, &renderer->vbo);
}

static
void generate_square(size_t *index_count, size_t *vertex_count,
                     GLushort *indices, vertex *vertices,
                     float ax, float ay, float az,
                     float bx, float by, float bz,
                     float cx, float cy, float cz,
                     float dx, float dy, float dz,
                     float nx, float ny, float nz,
                     uint8_t r, uint8_t g, uint8_t b);

int generate_geometry(noise_renderer *renderer, float *noise) {
  int status = 0;

  GLushort *indices = malloc(MaxIndexBufferSize);
  if (!indices) {
    status = -1;
    goto fail_alloc_indices;
  }

  vertex *vertices = malloc(MaxVertexBufferSize);
  if (!vertices) {
    status = -1;
    goto fail_alloc_vertices;
  }

  size_t vertex_count   = 0;
  renderer->index_count = 0;

  /* left face */
  generate_square(&renderer->index_count, &vertex_count,
                  indices, vertices,
                  0, 0, 0,
                  0, 0, LevelDepth,
                  0, LevelHeight, 0,
                  0, LevelHeight, LevelDepth,
                  1, 0, 0,
                  127, 127, 127);

  /* right face */
  generate_square(&renderer->index_count, &vertex_count,
                  indices, vertices,
                  LevelWidth, 0, 0,
                  LevelWidth, 0, LevelDepth,
                  LevelWidth, LevelHeight, 0,
                  LevelWidth, LevelHeight, LevelDepth,
                  -1, 0, 0,
                  127, 127, 127);

  /* bottom face */
  generate_square(&renderer->index_count, &vertex_count,
                  indices, vertices,
                  0, 0, 0,
                  0, 0, LevelDepth,
                  LevelWidth, 0, 0,
                  LevelWidth, 0, LevelDepth,
                  0, 1, 0,
                  127, 127, 127);

  /* top face */
  generate_square(&renderer->index_count, &vertex_count,
                  indices, vertices,
                  0, LevelHeight, 0,
                  0, LevelHeight, LevelDepth,
                  LevelWidth, LevelHeight, 0,
                  LevelWidth, LevelHeight, LevelDepth,
                  0, -1, 0,
                  127, 127, 127);

  /* back face */
  generate_square(&renderer->index_count, &vertex_count,
                  indices, vertices,
                  0, 0, 0,
                  0, LevelHeight, 0,
                  LevelWidth, 0, 0,
                  LevelWidth, LevelHeight, 0,
                  0, 0, 1,
                  127, 127, 127);

  /* front face */
  generate_square(&renderer->index_count, &vertex_count,
                  indices, vertices,
                  0, 0, LevelDepth,
                  0, LevelHeight, LevelDepth,
                  LevelWidth, 0, LevelDepth,
                  LevelWidth, LevelHeight, LevelDepth,
                  0, 0, -1,
                  127, 127, 127);

  for (size_t z = 0; z < LevelDepth; z++) {
    for (size_t y = 0; y < LevelHeight; y++) {
      for (size_t x = 0; x < LevelWidth; x++) {
        if (noise[x + y*LevelWidth + z*LevelWidth*LevelHeight] >=
            DensityThreshold) {
          /* left face */
          generate_square(&renderer->index_count, &vertex_count,
                          indices, vertices,
                          x, y, z,
                          x, y, z+1,
                          x, y+1, z,
                          x, y+1, z+1,
                          -1, 0, 0,
                          0, 183, 235);

          /* right face */
          generate_square(&renderer->index_count, &vertex_count,
                          indices, vertices,
                          x+1, y, z,
                          x+1, y, z+1,
                          x+1, y+1, z,
                          x+1, y+1, z+1,
                          +1, 0, 0,
                          0, 183, 235);

          /* bottom face */
          generate_square(&renderer->index_count, &vertex_count,
                          indices, vertices,
                          x, y, z,
                          x, y, z+1,
                          x+1, y, z,
                          x+1, y, z+1,
                          0, -1, 0,
                          0, 183, 235);

          /* top face */
          generate_square(&renderer->index_count, &vertex_count,
                          indices, vertices,
                          x, y+1, z,
                          x, y+1, z+1,
                          x+1, y+1, z,
                          x+1, y+1, z+1,
                          0, +1, 0,
                          0, 183, 235);


          /* back face */
          generate_square(&renderer->index_count, &vertex_count,
                          indices, vertices,
                          x, y, z,
                          x, y+1, z,
                          x+1, y, z,
                          x+1, y+1, z,
                          0, 0, -1,
                          0, 183, 235);

          /* front face */
          generate_square(&renderer->index_count, &vertex_count,
                          indices, vertices,
                          x, y, z+1,
                          x, y+1, z+1,
                          x+1, y, z+1,
                          x+1, y+1, z+1,
                          0, 0, 1,
                          0, 183, 235);
        }
      }
    }
  }


  glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(vertex),
                  vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ibo);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                  renderer->index_count * sizeof(GLushort), indices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                     free(vertices);
fail_alloc_vertices: free(indices);
fail_alloc_indices:  return status;
}

void render(const noise_renderer *renderer) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(renderer->vao);
  glUseProgram(renderer->prog);
  glDrawElements(GL_TRIANGLES, renderer->index_count, GL_UNSIGNED_SHORT,
                 (void*)0);
  glUseProgram(0);
  glBindVertexArray(0);
}

void set_mvp(noise_renderer *renderer,
             mat4 model, mat4 view, mat4 projection) {
  mat4 model_view = mat4_mul(view, model);
  mat3 normal_matrix = mat3_transposed_inverse(mat4_upper_left_33(model_view));
  vec3 l = vec3_normalize(mat4_apply(view, (vec3){1, 1, 1}));

  glUseProgram(renderer->prog);
  glUniformMatrix4fv(renderer->uniforms.model_view, 1, GL_TRUE,
                     model_view.data);
  glUniformMatrix4fv(renderer->uniforms.projection, 1, GL_TRUE,
                     projection.data);
  glUniformMatrix3fv(renderer->uniforms.normal_matrix, 1, GL_TRUE,
                     normal_matrix.data);
  glUniform3f(renderer->uniforms.light.pos, l.x, l.y, l.z);
  glUseProgram(0);
}

static
void generate_square(size_t *index_count, size_t *vertex_count,
                     GLushort *indices, vertex *vertices,
                     float ax, float ay, float az,
                     float bx, float by, float bz,
                     float cx, float cy, float cz,
                     float dx, float dy, float dz,
                     float nx, float ny, float nz,
                     uint8_t r, uint8_t g, uint8_t b) {
  vec3 va = {ax, ay, az};
  vec3 vb = {bx, by, bz};
  vec3 vc = {cx, cy, cz};
  vec3 vd = {dx, dy, dz};

  vec3 n = {nx, ny, nz};

  color col = {r, g, b};

  vec3 vab = vec3_sub(vb, va);
  vec3 vbc = vec3_sub(vc, vb);

  if (vec3_dot(vec3_cross(vab, vbc), n) > 0) { /* vertices are in order */
    indices[(*index_count)++] = *vertex_count;
    indices[(*index_count)++] = *vertex_count + 1;
    indices[(*index_count)++] = *vertex_count + 2;

    indices[(*index_count)++] = *vertex_count + 2;
    indices[(*index_count)++] = *vertex_count + 1;
    indices[(*index_count)++] = *vertex_count + 3;
  }
  else { /* vertices are backwards */
    indices[(*index_count)++] = *vertex_count + 3;
    indices[(*index_count)++] = *vertex_count + 1;
    indices[(*index_count)++] = *vertex_count + 2;

    indices[(*index_count)++] = *vertex_count + 2;
    indices[(*index_count)++] = *vertex_count + 1;
    indices[(*index_count)++] = *vertex_count;
  }

  vertices[(*vertex_count)++] = (vertex){va, n, col};
  vertices[(*vertex_count)++] = (vertex){vb, n, col};
  vertices[(*vertex_count)++] = (vertex){vc, n, col};
  vertices[(*vertex_count)++] = (vertex){vd, n, col};
}
