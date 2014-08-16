#ifndef VECTOR_MATH_H_
#define VECTOR_MATH_H_

#define Pi 3.14159265358979323846

typedef struct vec3 {
  float x, y, z;
} vec3;

vec3 vec3_normalize(vec3 v);

float vec3_dot(vec3 a, vec3 b);
vec3  vec3_cross(vec3 a, vec3 b);

vec3 vec3_sub(vec3 a, vec3 b);
vec3 vec3_add(vec3 a, vec3 b);
vec3 vec3_mul(vec3 a, vec3 b);

vec3 vec3_scale(float f, vec3 a);

typedef struct mat3 {
  float data[9];
} mat3;

#define mat3_at(m, x, y) ((m).data[(x)+3*(y)])

mat3 mat3_transposed_inverse(mat3 m);

typedef struct mat4 {
  float data[16];
} mat4;

#define mat4_at(m, x, y) ((m).data[(x)+4*(y)])

#define Mat4Identity \
  (mat4){            \
    {1, 0, 0, 0,     \
     0, 1, 0, 0,     \
     0, 0, 1, 0,     \
     0, 0, 0, 1}     \
  }

mat4 mat4_mul(mat4 a, mat4 b);

mat3 mat4_upper_left_33(mat4 m);

mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up);
mat4 mat4_perspective(float fov, float aspect, float z_near, float z_far);

vec3 mat4_apply(mat4 m, vec3 v);

#endif
