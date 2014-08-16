#include <math.h>

#include "camera.h"

static float fmod_divsign(float x, float y);

void camera_init(camera *camera, float aspect_ratio) {
  camera->aspect = aspect_ratio;

  camera->eye = (vec3){3, 3, 3};

  camera->elevation = 0;
  camera->azimuth   = Pi/2;

  camera->fov = Pi/4;
  camera_reorient(camera, 0, 0);
}

void camera_reorient(camera *camera, float dazimuth, float delevation) {
  camera->elevation = camera->elevation + delevation;
  camera->azimuth   = camera->azimuth + dazimuth;

  if (camera->elevation > Pi/2) camera->elevation = Pi/2;
  else if (camera->elevation < -Pi/2) camera->elevation = -Pi/2;

  camera->azimuth = fmod_divsign(camera->azimuth, 2*Pi);

  camera->forward = (vec3){
    cosf(camera->elevation)*sinf(camera->azimuth),
    sinf(camera->elevation),
    cosf(camera->elevation)*cosf(camera->azimuth)
  };

  camera->right = (vec3){
    sinf(camera->azimuth - Pi/2),
    0,
    cosf(camera->azimuth - Pi/2)
  };

  camera->up = vec3_cross(camera->right, camera->forward);
}

void camera_move(camera *camera, float forward, float right, float up) {
  camera->eye = vec3_add(camera->eye, vec3_scale(forward, camera->forward));
  camera->eye = vec3_add(camera->eye, vec3_scale(right, camera->right));
  camera->eye = vec3_add(camera->eye, vec3_scale(up, camera->up));
}

mat4 camera_view(const camera *camera) {
  return mat4_look_at(camera->eye,
                      vec3_add(camera->eye, camera->forward),
                      camera->up);
}

mat4 camera_projection(const camera *camera) {
  return mat4_perspective(camera->fov, camera->aspect, 0.1, 2000);
}

static float fmod_divsign(float x, float y) {
  return x - floorf(x/y)*y;
}
