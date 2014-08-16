#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "noise_renderer.h"
#include "noise_gen.h"
#include "camera.h"
#include "vector_math.h"

#include <GLFW/glfw3.h>

#define ScreenWidth  640
#define ScreenHeight 480

#define MidX (ScreenWidth/2)
#define MidY (ScreenHeight/2)

#define AspectRatio ((float)ScreenWidth/ScreenHeight)

int main(int argc, char **argv) {
  int status = 0;

  glfwInit();
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
  GLFWwindow *window = glfwCreateWindow(ScreenWidth, ScreenHeight, "gl_noise",
                                        NULL, NULL);
  glfwMakeContextCurrent(window);

  glewInit();

  float *noise = malloc(sizeof(*noise)*LevelWidth*LevelHeight*LevelDepth);
  single_cell(noise, 5, 5, 5);

  noise_renderer prog;
  noise_renderer_init(&prog);

  if (generate_geometry(&prog, noise) != 0) {
    fprintf(stderr, "An error occured while generating noise.\n");
    status = 1;
  }

  glViewport(0, 0, ScreenWidth, ScreenHeight);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetCursorPos(window, MidX, MidY);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_FRAMEBUFFER_SRGB);

  camera camera;
  camera_init(&camera, AspectRatio);

  float old_time = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    set_mvp(&prog,
            Mat4Identity,
            camera_view(&camera),
            camera_projection(&camera));
    render(&prog);

    float new_time = glfwGetTime();
    float delta_t = (new_time - old_time);
    old_time = new_time;

    double mouse_x, mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    glfwSetCursorPos(window, MidX, MidY);
    camera_reorient(&camera,
                    CameraMouseSpeed*delta_t*(MidX - mouse_x),
                    CameraMouseSpeed*delta_t*(MidY - mouse_y));

    float distance = CameraMoveSpeed * delta_t;
    float forward = 0, right = 0, up = 0;

    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) forward += distance;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) forward -= distance;

    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) right -= distance;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) right += distance;

    if (glfwGetKey(window, GLFW_KEY_SPACE)      == GLFW_PRESS) up += distance;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) up -= distance;

    camera_move(&camera, forward, right, up);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  noise_renderer_release(&prog);

  glfwDestroyWindow(window);
  glfwTerminate();
  return status;
}
