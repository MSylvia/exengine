#ifndef CAMERA_H
#define CAMERA_H

#define GLEW_STATIC
#include <GL/glew.h>

#include <mathlib.h>
#include <stdbool.h>

typedef struct {
  vec3 position, front, up;
  float yaw, pitch, last_x, last_y, fov, sensitivity;
  mat4x4 view, projection;
} iso_camera_t;

/**
 * [iso_camera_new create a new isometric ortho camera]
 * @param  x [x position]
 * @param  y [y position]
 * @param  z [z position]
 * @return   [iso_camera_t pointer]
 */
iso_camera_t* iso_camera_new(float x, float y, float z, float sensitivity, float fov);

/**
 * [iso_cam_resize reset projections etc]
 * @param cam [iso_camera_t pointer]
 */
void iso_camera_resize(iso_camera_t *cam);

/**
 * [iso_camera_update update the cams projections etc]
 * @param cam            [iso_camera_t pointer]
 * @param shader_program [shader program to use]
 */
void iso_camera_update(iso_camera_t *cam, GLuint shader_program);

#endif // CAMERA_H