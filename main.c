#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "noise_renderer.h"
#include "noise_gen.h"
#include "camera.h"
#include "vector_math.h"

#include <GLFW/glfw3.h>

#define ScreenWidth  640
#define ScreenHeight 480

#define MidX (ScreenWidth/2)
#define MidY (ScreenHeight/2)

#define AspectRatio ((GLfloat)ScreenWidth/ScreenHeight)

#define NoiseStart (vec3){0, 0, 0}
#define NoiseScale (vec3){1.0/LevelWidth, 1.0/LevelHeight, 1.0/LevelDepth}
#define OctaveCount 3

#define AnimatedNoiseStart (vec4){0, 0, 0, 0}
#define AnimatedNoiseScale (vec4){1.0/LevelWidth, 1.0/LevelHeight, \
                                  1.0/LevelDepth, 0.01}

static int has_option(int argc, char **argv, char *opt);

void gl_debug(GLenum source, GLenum type, GLuint id,
              GLenum severity, GLsizei length,
              const GLchar *message, void *user_param) {
  fprintf(stderr, "[GL] %s\n", message);
}

int main(int argc, char **argv) {
  int status = 0;

  srand(time(NULL));

  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW.\n");
    status = 1;
    goto fail_init_glfw;
  }

  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
  if (has_option(argc, argv, "--debug"))
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  GLFWwindow *window = glfwCreateWindow(ScreenWidth, ScreenHeight, "gl_noise",
                                        NULL, NULL);
  if (!window) {
    fprintf(stderr, "Failed to create a new window.\n");
    status = 1;
    goto fail_create_window;
  }

  glfwMakeContextCurrent(window);

  glewInit();

  if (has_option(argc, argv, "--debug")) {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
                          0, 0, GL_TRUE);
    glDebugMessageCallback(gl_debug, NULL);
  }

  GLfloat *noise = malloc(sizeof(*noise)*LevelWidth*LevelHeight*LevelDepth);
  if (!noise) {
    fprintf(stderr, "Failed to create noise buffer.\n");
    status = 1;
    goto fail_alloc_noise;
  }

  perlin4d_gen gen;
  int animated = 0;

  if (has_option(argc, argv, "--test"))
    single_cell(LevelWidth, LevelHeight, LevelDepth, noise, 5, 5, 5);
  else if (has_option(argc, argv, "--white"))
    white_noise(LevelWidth, LevelHeight, LevelDepth, noise);
  else if (has_option(argc, argv, "--simplex"))
    simplex3d(LevelWidth, LevelHeight, LevelDepth, noise,
              OctaveCount, NoiseStart, NoiseScale);
  else if (has_option(argc, argv, "--perlin4d")) {
    perlin4d_init(&gen, LevelWidth, LevelHeight, LevelDepth, OctaveCount,
                  AnimatedNoiseStart, AnimatedNoiseScale);
    perlin4d_slice(&gen, 0, noise);
    animated = 1;
  }
  else
    perlin3d(LevelWidth, LevelHeight, LevelDepth, noise,
             OctaveCount, NoiseStart, NoiseScale);

  noise_renderer prog;
  noise_renderer_init(&prog, animated ? NoiseAnimated : NoiseConstant);

  if (generate_geometry(&prog, noise) != 0) {
    fprintf(stderr, "An error occured while generating noise.\n");
    status = 1;
    goto fail_generate_geometry;
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
  float start_time = old_time;
  while (!glfwWindowShouldClose(window)) {
    set_mvp(&prog,
            Mat4Identity,
            camera_view(&camera),
            camera_projection(&camera));
    render(&prog);

    if (animated) {
      perlin4d_slice(&gen, glfwGetTime() - start_time, noise);
      if (generate_geometry(&prog, noise) != 0) {
        fprintf(stderr, "An error occured while generating noise.\n");
        status = 1;
        goto fail_generate_mid_loop;
      }
    }

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

fail_generate_mid_loop: noise_renderer_release(&prog);
fail_generate_geometry: free(noise);
fail_alloc_noise:       glfwDestroyWindow(window);
fail_create_window:     glfwTerminate();
fail_init_glfw:         return status;
}

static int has_option(int argc, char **argv, char *opt) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], opt) == 0)
      return 1;
  }

  return 0;
}
