#include "exengine/camera.h"
#include "exengine/texture.h"
#include "exengine/pointlight.h"
#include "exengine/dirlight.h"
#include "exengine/scene.h"
#include "exengine/exe_list.h"
#include "exengine/iqm.h"
#include "exengine/skybox.h"
#include "exengine/entity.h"
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
  memcpy(scene->gravity, (vec3){0.0f, -0.1f, 0.0f}, sizeof(vec3));

  // init the camera
  camera = fps_camera_new(0.0f, 0.0f, 0.0f, 0.03f, 90.0f);
  scene->fps_camera = camera;
}

void game_run()
{
  model_t *m6 = iqm_load_model(scene, "data/level.iqm", 1);
  list_add(scene->model_list, m6);

  // dir_light_t *dl = dir_light_new((vec3){8.0f, 16.0f, 8.0f}, (vec3){1.0f, 1.0f, 1.1f}, 1);
  // list_add(scene->dir_light_list, dl);

  // skybox_t *s = skybox_new("sky");
  // scene->skybox = s;

  entity_t *e = entity_new(scene, (vec3){0.5f, 1.0f, 0.5f});
  e->position[1] = 1.1f;
  e->position[0] = 1.1f;
  e->position[2] = 5.0f;
  float move_speed = 1.5f;

  model_t *d = iqm_load_model(scene, "data/human.iqm", 0);
  list_add(scene->model_list, d);
  // d->position[1] = 1.5f;
  // d->rotation[0] = -90.0f;
  // model_set_anim(d, 1);
  // d->current_anim = NULL;
  
  model_t *g2 = iqm_load_model(scene, "data/gun.iqm", 0);
  g2->use_transform = 1;
  list_add(scene->model_list, g2);

  model_t *g = iqm_load_model(scene, "data/gun.iqm", 0);
  list_add(scene->model_list, g);
  camera->view_model = g;
  float lastyaw = 0;
  int aim = 0;
  vec3 oldpos;
  camera->view_model_offset[0]  = 0.0f;
  camera->view_model_offset[1]  = -0.15f;
  camera->view_model_offset[2]  = -0.25f;

  point_light_t *pl = point_light_new((vec3){0.0f, 0.0f, 0.0f}, (vec3){0.2f, 0.2f, 0.25f}, 1);
  memcpy(pl->position, e->position, sizeof(vec3));
  list_add(scene->point_light_list, pl);
  pl->is_shadow = 0;

  double last_frame_time = glfwGetTime();
  while (!glfwWindowShouldClose(display.window)) {
    // handle window events
    glfwPollEvents();

    // calculate delta time
    double current_frame_time = glfwGetTime();
    delta_time = (float)current_frame_time - (float)last_frame_time;
    last_frame_time = current_frame_time;

    entity_update(e);
    memcpy(camera->position, e->position, sizeof(vec3));
    camera->position[1] += e->radius[1];
    memcpy(pl->position, camera->position, sizeof(vec3));

    if (keys_down[GLFW_KEY_F]) {
      float r = (float)rand()/(float)(RAND_MAX/1.5f);
      float g = (float)rand()/(float)(RAND_MAX/0.8f);
      float b = (float)rand()/(float)(RAND_MAX/1.5f);
      point_light_t *l = point_light_new((vec3){0.0f, 0.0f, 0.0f}, (vec3){r, g, b}, 1);
      memcpy(l->position, camera->position, sizeof(vec3));
      list_add(scene->point_light_list, l);

      model_t *bulb = iqm_load_model(scene, "data/bulb.iqm", 0);
      bulb->rotation[0] = -90.0f;
      bulb->scale = 0.5f;
      bulb->is_lit = 0;
      bulb->is_shadow = 0;
      memcpy(bulb->position, e->position, sizeof(vec3));
      list_add(scene->model_list, bulb);

      keys_down[GLFW_KEY_F] = 0;
    }

    /* debug entity movement */
    float y = e->velocity[1];
    vec3 temp;
    vec3_scale(temp, e->velocity, 0.3f);
    temp[1] = 0.0f;
    // model_get_bone_transform(d, "knee_L", g2->transform);
    
    // can shit
    if (camera->view_model_offset[0] < 0.15f && lastyaw - camera->yaw > 0)
      camera->view_model_offset[0] += ((lastyaw - camera->yaw) * 0.15f) * delta_time;
    if (camera->view_model_offset[0] > 0.05f && lastyaw - camera->yaw < 0)
      camera->view_model_offset[0] += ((lastyaw - camera->yaw) * 0.15f) * delta_time;
    lastyaw = camera->yaw;

    if (buttons_down[GLFW_MOUSE_BUTTON_RIGHT]) {
      if (aim == 0)
        memcpy(oldpos, camera->view_model_offset, sizeof(vec3));
      memcpy(camera->view_model_offset, (vec3){0.0f, -0.16f, 0.1f}, sizeof(vec3));
      aim = 1;
      move_speed = 0.8f;
    } else if (aim == 1) {
      memcpy(camera->view_model_offset, oldpos, sizeof(vec3));
      aim = 0;
    }
    
    if (aim == 0)
      camera->view_model_offset[1] = (camera->front[1] * 0.1f) - 0.2f;;

    if (e->grounded == 1) 
      vec3_sub(e->velocity, e->velocity, temp);
    else
      move_speed = 0.1f;
    
    e->velocity[1] = y;
    if (e->grounded == 0)
      e->velocity[1] -= 0.8f * delta_time;
    else if (e->velocity[1] <= 0.0f)
      e->velocity[1] = 0.0f;

    if (e->grounded == 0 && e->velocity[1] > 0.0f) {
      float speed = move_speed * 4.0f;
      vec3_norm(temp, e->velocity);
      vec3_scale(temp, temp, speed * delta_time);
      temp[1] = 0.0f;
      vec3_sub(e->velocity, e->velocity, temp);

      vec3_scale(temp, camera->front, speed * delta_time);
      temp[1] = 0.0f;
      vec3_add(e->velocity, e->velocity, temp);
    }

    vec3 speed, side;
    if (keys_down[GLFW_KEY_W]) {
      vec3_scale(speed, camera->front, move_speed * delta_time);
      speed[1] = 0.0f;
      vec3_add(e->velocity, e->velocity, speed);
    }
    if (keys_down[GLFW_KEY_S]) {
      vec3_scale(speed, camera->front, move_speed * delta_time);
      speed[1] = 0.0f;
      vec3_sub(e->velocity, e->velocity, speed);
    }
    if (keys_down[GLFW_KEY_A]) {
      vec3_mul_cross(side, camera->front, camera->up);
      vec3_norm(side, side);
      vec3_scale(side, side, (move_speed*0.9f) * delta_time);
      speed[1] = 0.0f;
      vec3_sub(e->velocity, e->velocity, side);
    }
    if (keys_down[GLFW_KEY_D]) {
      vec3_mul_cross(side, camera->front, camera->up);
      vec3_norm(side, side);
      vec3_scale(side, side, (move_speed*0.9f) * delta_time);
      speed[1] = 0.0f;
      vec3_add(e->velocity, e->velocity, side);
    }

    if (keys_down[GLFW_KEY_Q])
      e->velocity[1] = 0.5f;
    if (keys_down[GLFW_KEY_Z])
      e->velocity[1] = -0.5f;

    if (keys_down[GLFW_KEY_SPACE] && e->grounded == 1) {
      vec3_scale(speed, camera->front, move_speed * delta_time);
      speed[1] = 0.0f;
      vec3_add(e->velocity, e->velocity, speed);
      vec3_scale(temp, e->velocity, 0.05f);
      vec3_add(e->velocity, e->velocity, temp);
      e->velocity[1] = 0.2f;
    }
    if (keys_down[GLFW_KEY_LEFT_CONTROL]) {
      e->radius[1] = 0.5f;
      move_speed = 1.2f;
    } else {
      if (e->radius[1] != 1.0f) {
        e->position[1] += 0.5f;
      }
      move_speed = 2.5f;
      e->radius[1] = 1.0f;
    }
    if (keys_down[GLFW_KEY_ESCAPE])
      break;
    /* ------ */

    memcpy(pl->position, e->position, sizeof(vec3));
    pl->position[1] += 1.0f;
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