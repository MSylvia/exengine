#include <mesh.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mesh_t* mesh_new(vertex_t* vertices, size_t vcount, GLuint *indices, size_t icount, GLuint texture)
{
  mesh_t* m = malloc(sizeof(mesh_t));

  m->texture = texture;
  m->vcount = vcount;
  m->icount = icount;
  m->current_anim  = NULL;
  m->current_time  = 0.0;
  m->current_frame = 0;

  mat4x4_identity(m->transform);

  glGenVertexArrays(1, &m->VAO);
  glGenBuffers(1, &m->VBO);
  glGenBuffers(1, &m->EBO);

  glBindVertexArray(m->VAO);

  // vertices
  glBindBuffer(GL_ARRAY_BUFFER, m->VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t)*m->vcount, &vertices[0], GL_STATIC_DRAW);

  // indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*m->icount, &indices[0], GL_STATIC_DRAW);

  // position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  // tex coords
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  // normals
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*)(5 * sizeof(GLfloat)));
  glEnableVertexAttribArray(2);

  // tangents
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*)(8 * sizeof(GLfloat)));
  glEnableVertexAttribArray(3);

  // color
  glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex_t), (GLvoid*)(12 * sizeof(GLfloat)));
  glEnableVertexAttribArray(4);

  // blend indexes
  glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex_t), (GLvoid*)(12 * sizeof(GLfloat)+(4 * sizeof(GLubyte))));
  glEnableVertexAttribArray(5);

  // blend weights
  glVertexAttribPointer(6, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex_t), (GLvoid*)(12 * sizeof(GLfloat)+(8 * sizeof(GLubyte))));
  glEnableVertexAttribArray(6);

  glBindVertexArray(0);

  return m;
}

void mesh_draw(mesh_t* m, GLuint shader_program)
{
  // bind vao/ebo/tex
  glBindVertexArray(m->VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->EBO);
  glBindTexture(GL_TEXTURE_2D, m->texture);

  // pass transform matrix to shader
  GLuint transform_loc = glGetUniformLocation(shader_program, "u_model");
  glUniformMatrix4fv(transform_loc, 1, GL_FALSE, m->transform[0]);
  
  // pass bone data
  if (m->bones != NULL) {
    GLuint bone_loc = glGetUniformLocation(shader_program, "u_bone_matrix");
    glUniformMatrix4fv(bone_loc, m->bones_len, GL_TRUE, &m->skeleton[0][0][0]);
  }

  // draw mesh
  glDrawElements(GL_TRIANGLES, m->icount, GL_UNSIGNED_INT, 0);

  // unbind buffers
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void mesh_destroy(mesh_t* m)
{
  glDeleteVertexArrays(1, &m->VAO);
  glDeleteBuffers(1, &m->VBO);
  glDeleteBuffers(1, &m->EBO);

  if (m->bones != NULL)
    free(m->bones);

  if (m->anims != NULL)
    free(m->anims);

  if (m->frames != NULL)
    free(m->frames);

  free(m);
  m = NULL;
}

void mesh_update(mesh_t *m, float delta_time)
{
  anim_t *anim = m->current_anim;

  if (anim == NULL)
    return;

  uint32_t current_frame = m->current_time * anim->rate;
  uint32_t len = anim->last - anim->first;
  if (current_frame > len && !anim->loop)
    return;

  m->current_time += delta_time;
  m->current_frame = anim->first + current_frame;

  if (m->current_frame > len) {
    if (anim->loop) {
      m->current_time -= len / anim->rate;
      m->current_frame = anim->first + m->current_time * anim->rate;
    } else {
      m->current_frame = anim->last;
    }
  }

  printf("FRAME: %d (%f)\n", m->current_frame, m->current_time);

  mesh_set_pose(m, m->frames[m->current_frame]);
  mesh_update_matrices(m);
}

void mesh_update_matrices(mesh_t *m)
{
  mat4x4 transform[m->bones_len];
  frame_t pose = m->pose;

  for (int i=0; i<m->bones_len; i++) {
    bone_t b = m->bones[i];

    mat4x4 mat, result;
    calc_bone_matrix(mat, pose[i].translate, pose[i].rotate, pose[i].scale);
    mat4x4_identity(result);

    if (b.parent > 0) {
      mat4x4_mul(transform[i], mat, transform[b.parent]);
      mat4x4_mul(result, m->inverse_base[i], transform[i]);
    } else {
      mat4x4_dup(transform[i], mat);
      mat4x4_mul(result, m->inverse_base[i], mat);
    }

    mat4x4_dup(m->skeleton[i], result);
  }
}

void mesh_set_pose(mesh_t *m, frame_t frame)
{
  for (int i=0; i<m->bones_len; i++) {
    pose_t f = frame[i];
    
    quat rotate;
    memcpy(rotate, f.rotate, sizeof(quat));
    quat_norm(rotate, rotate);

    memcpy(m->pose[i].translate,  f.translate, sizeof(vec3));
    memcpy(m->pose[i].rotate,     rotate,      sizeof(quat));
    memcpy(m->pose[i].scale,      f.scale,     sizeof(vec3));
  }
}

void calc_bone_matrix(mat4x4 m, vec3 pos, quat rot, vec3 scale)
{
  mat4x4 mat;

  printf("TRANSLATE: %f %f %f\n", pos[0], pos[1], pos[2]);
  printf("ROTATE: %f %f %f\n", rot[0], rot[1], rot[2]);
  printf("SCALE: %f %f %f\n", scale[0], scale[1], scale[2]);

  mat4x4_identity(m);

  mat4x4_translate(mat, pos);
  mat4x4_mul(m, m, mat);

  mat4x4_rotate_quat(mat, rot);
  mat4x4_mul(m, m, mat);

  mat4x4_scale_xyz(mat, scale);
  mat4x4_mul(m, m, mat);
}
