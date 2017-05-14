#include "exengine/camera.h"
#include "exengine/texture.h"
#include "exengine/pointlight.h"
#include "exengine/scene.h"
#include "exengine/exe_list.h"
#include "exengine/iqm.h"
#include "inc/game.h"

float delta_time;
fps_camera_t *camera = NULL;
scene_t *scene;
conf_t conf;

void game_init()
{
  // load config
  conf_load(&conf, "data/conf.cfg");

  // load config vars
  uint32_t width = 0, height = 0;
  width = conf_get_int(&conf, "window_width");
  height = conf_get_int(&conf, "window_height");
  
  // init the window and gl
  if (!window_init(width, height, "Underwater Zombie Maniac")) {
    game_exit();
    return;
  }

  // init the scene
  scene = scene_new();

  // init the camera
  camera = fps_camera_new(0.0f, 0.0f, 0.0f, 0.03f, 90.0f);
  scene->fps_camera = camera;
}

void game_run()
{ 
  // test iqm model shit
  model_t *m = iqm_load_model(scene, "data/cube.iqm");
  mesh_set_anim(m->mesh_list->data, 1);
  m->rotation[0] = -90.0f;
  m->position[1] = -10.0f;
  list_add(scene->model_list, m);

  model_t *m6 = iqm_load_model(scene, "data/level.iqm");
  mesh_t *m7 = m6->mesh_list->data;
  mesh_t *m8 = m6->mesh_list->next->data;
  m6->rotation[0] = -90.0f;
  m6->position[1] = -10.0f;
  list_add(scene->model_list, m6);

  double last_frame_time = glfwGetTime();
  while (!glfwWindowShouldClose(display.window)) {
    // handle window events
    glfwPollEvents();

    // calculate delta time
    double current_frame_time = glfwGetTime();
    delta_time = (float)current_frame_time - (float)last_frame_time;
    last_frame_time = current_frame_time;
    
    if (keys_down[GLFW_KEY_F]) {
      point_light_t *l = point_light_new((vec3){0.0f, 0.0f, 0.0f}, (vec3){50.0f, 50.0f, 50.0f}, 1);
      memcpy(l->position, camera->position, sizeof(vec3));
      list_add(scene->point_light_list, l);

      model_t *m = iqm_load_model(scene, "data/bulb.iqm");
      m->rotation[0] = -90.0f;
      m->is_lit = 0;
      memcpy(m->position, camera->position, sizeof(vec3));
      list_add(scene->model_list, m);

      keys_down[GLFW_KEY_F] = 0;
    }

    /* debug cam movement */
    vec3 speed, side;
    if (keys_down[GLFW_KEY_W]) {
      vec3_scale(speed, camera->front, 0.3f);
      vec3_add(camera->position, camera->position, speed);
    }
    if (keys_down[GLFW_KEY_S]) {
      vec3_scale(speed, camera->front, 0.3f);
      vec3_sub(camera->position, camera->position, speed);
    }
    if (keys_down[GLFW_KEY_A]) {
      vec3_mul_cross(side, camera->front, camera->up);
      vec3_norm(side, side);
      vec3_scale(side, side, 0.2f);
      vec3_sub(camera->position, camera->position, side);
    }
    if (keys_down[GLFW_KEY_D]) {
      vec3_mul_cross(side, camera->front, camera->up);
      vec3_norm(side, side);
      vec3_scale(side, side, 0.2f);
      vec3_add(camera->position, camera->position, side);
    }
    if (keys_down[GLFW_KEY_SPACE])
      camera->position[1] += 0.2f;
    if (keys_down[GLFW_KEY_LEFT_CONTROL])
      camera->position[1] -= 0.2f;
    if (keys_down[GLFW_KEY_ESCAPE])
      break;
    /* ------ */

    scene_update(scene, delta_time);
    scene_draw(scene);

    glfwSwapBuffers(display.window);
  }

}

void game_exit()
{
  scene_destroy(scene);
  conf_free(&conf);
  window_destroy();
  printf("Exiting\n");
}
