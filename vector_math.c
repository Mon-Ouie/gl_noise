#include "vector_math.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

vec3 vec3_normalize(vec3 v) {
  GLfloat norm = sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);
  return (vec3){v.x/norm, v.y/norm, v.z/norm};
}

GLfloat vec3_dot(vec3 a, vec3 b) {
  return a.x*b.x+a.y*b.y+a.z*b.z;
}

vec3 vec3_cross(vec3 a, vec3 b) {
  return (vec3){
    a.y*b.z - a.z*b.y,
    a.z*b.x - a.x*b.z,
    a.x*b.y - a.y*b.x
  };
}

vec3 vec3_sub(vec3 a, vec3 b) {
  return (vec3){a.x-b.x, a.y-b.y, a.z-b.z};
}

vec3 vec3_add(vec3 a, vec3 b) {
  return (vec3){a.x+b.x, a.y+b.y, a.z+b.z};
}

vec3 vec3_mul(vec3 a, vec3 b) {
  return (vec3){a.x*b.x, a.y*b.y, a.z*b.z};
}

vec3 vec3_scale(GLfloat f, vec3 a) {
  return (vec3){f*a.x, f*a.y, f*a.z};
}

mat3 mat3_transposed_inverse(mat3 m) {
  GLfloat det = mat3_at(m, 0, 0) * mat3_at(m, 1, 1) * mat3_at(m, 2, 2)
            + mat3_at(m, 1, 0) * mat3_at(m, 2, 1) * mat3_at(m, 0, 2)
            + mat3_at(m, 2, 0) * mat3_at(m, 0, 1) * mat3_at(m, 1, 2)
            - mat3_at(m, 0, 2) * mat3_at(m, 1, 1) * mat3_at(m, 2, 0)
            - mat3_at(m, 1, 2) * mat3_at(m, 2, 1) * mat3_at(m, 0, 0)
            - mat3_at(m, 2, 2) * mat3_at(m, 0, 1) * mat3_at(m, 1, 0);

  mat3 inv;

  mat3_at(inv, 0, 0) = + mat3_at(m, 1, 1) * mat3_at(m, 2, 2)
                       - mat3_at(m, 1, 2) * mat3_at(m, 2, 1);
  mat3_at(inv, 1, 0) = - mat3_at(m, 0, 1) * mat3_at(m, 2, 2)
                       + mat3_at(m, 0, 2) * mat3_at(m, 2, 1);
  mat3_at(inv, 2, 0) = + mat3_at(m, 0, 1) * mat3_at(m, 1, 2)
                       - mat3_at(m, 0, 2) * mat3_at(m, 1, 1);

  mat3_at(inv, 0, 1) = - mat3_at(m, 1, 0) * mat3_at(m, 2, 2)
                       + mat3_at(m, 1, 2) * mat3_at(m, 2, 0);
  mat3_at(inv, 1, 1) = + mat3_at(m, 0, 0) * mat3_at(m, 2, 2)
                       - mat3_at(m, 0, 2) * mat3_at(m, 2, 0);
  mat3_at(inv, 2, 1) = - mat3_at(m, 0, 0) * mat3_at(m, 1, 2)
                       + mat3_at(m, 0, 2) * mat3_at(m, 1, 0);

  mat3_at(inv, 0, 2) = + mat3_at(m, 1, 0) * mat3_at(m, 2, 1)
                       - mat3_at(m, 1, 1) * mat3_at(m, 2, 0);
  mat3_at(inv, 1, 2) = - mat3_at(m, 0, 0) * mat3_at(m, 2, 1)
                       + mat3_at(m, 0, 1) * mat3_at(m, 2, 0);
  mat3_at(inv, 2, 2) = + mat3_at(m, 0, 0) * mat3_at(m, 1, 1)
                       - mat3_at(m, 0, 1) * mat3_at(m, 1, 0);

  for (size_t i = 0; i < 9; i++) inv.data[i] /= det;

  return inv;
}

mat4 mat4_mul(mat4 a, mat4 b) {
  mat4 out;

  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      mat4_at(out, j, i) = 0;
      for (size_t k = 0; k < 4; k++)
        mat4_at(out, j, i) += mat4_at(a, k, i) * mat4_at(b, j, k);
    }
  }

  return out;
}

mat3 mat4_upper_left_33(mat4 m) {
  mat3 ret;
  memcpy(&mat3_at(ret, 0, 0), &mat4_at(m, 0, 0), 3*sizeof(GLfloat));
  memcpy(&mat3_at(ret, 0, 1), &mat4_at(m, 0, 1), 3*sizeof(GLfloat));
  memcpy(&mat3_at(ret, 0, 2), &mat4_at(m, 0, 2), 3*sizeof(GLfloat));
  return ret;
}

mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up) {
  vec3 f = vec3_normalize(vec3_sub(center, eye));
  up = vec3_normalize(up);

  vec3 s = vec3_cross(f, up);
  vec3 u = vec3_cross(s, f);

  return (mat4){
    { s.x,  s.y,  s.z, -vec3_dot(s, eye),
      u.x,  u.y,  u.z, -vec3_dot(u, eye),
     -f.x, -f.y, -f.z,  vec3_dot(f, eye),
        0,    0,    0,  1}
  };
}

mat4 mat4_perspective(GLfloat fov, GLfloat aspect, GLfloat z_near,
                      GLfloat z_far) {
  GLfloat f = tanf(Pi/2 - fov/2);

  return (mat4){
    {f / aspect, 0, 0, 0,
     0, f, 0, 0,
     0, 0, (z_far+z_near)/(z_near-z_far), 2*z_far*z_near/(z_near-z_far),
     0, 0, -1, 0}
  };
}

vec3 mat4_apply(mat4 m, vec3 v) {
  return (vec3){
    v.x*mat4_at(m, 0, 0) + v.y*mat4_at(m, 1, 0) + v.z*mat4_at(m, 2, 0) +
      mat4_at(m, 3, 0),
    v.x*mat4_at(m, 0, 1) + v.y*mat4_at(m, 1, 1) + v.z*mat4_at(m, 2, 1) +
      mat4_at(m, 3, 1),
    v.x*mat4_at(m, 0, 2) + v.y*mat4_at(m, 1, 2) + v.z*mat4_at(m, 2, 2) +
      mat4_at(m, 3, 2),
  };
}
