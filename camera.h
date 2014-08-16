#ifndef CAMERA_H_
#define CAMERA_H_

#include "vector_math.h"

#define CameraMouseSpeed 0.1 /* rad/sec */
#define CameraMoveSpeed  5.0 /* units per second, for each dimension */

typedef struct camera {
  vec3 eye;
  float elevation, azimuth;
  float fov;
  float aspect;

  vec3 forward, right, up;
} camera;

void camera_init(camera *camera, float aspect_ratio);

void camera_reorient(camera *camera, float dazimuth, float delevation);
void camera_move(camera *camera, float forward, float right, float up);

mat4 camera_view(const camera *camera);
mat4 camera_projection(const camera *camera);

#endif
