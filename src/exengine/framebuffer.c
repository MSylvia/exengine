#include <stdio.h>
#include "framebuffer.h"
#include "shader.h"
#include "window.h"
#include "exe_conf.h"

extern conf_t conf;

GLuint fbo, rbo, colorbuffer, ex_fbo_shader, fbo_vao, fbo_vbo;
int width, height;

void ex_framebuffer_init()
{
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // color buffer
  glfwGetFramebufferSize(display.window, &width, &height);
  width = conf_get_int(&conf, "window_width");
  height = conf_get_int(&conf, "window_height");
  glGenTextures(1, &colorbuffer);
  glBindTexture(GL_TEXTURE_2D, colorbuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  GLfloat border[] = {1.0, 1.0, 1.0, 1.0};
  glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, border);
  glBindTexture(GL_TEXTURE_2D, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorbuffer, 0);

  // depth buffer
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

  // test framebuffer
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    printf("Error! Framebuffer is not complete\n");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // load the fbo shader
  ex_fbo_shader = ex_shader_compile("fboshader.vs", "fboshader.fs", NULL);
  /* ------------------------- */

  /* -- screen quad -- */
  GLfloat vertices[] = {
    // pos         // uv
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
  };

  // vao for framebuffer
  glGenVertexArrays(1, &fbo_vao);
  glGenBuffers(1, &fbo_vbo);
  glBindVertexArray(fbo_vao);

  // vertices
  glBindBuffer(GL_ARRAY_BUFFER, fbo_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

  // position
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (GLvoid*)0);
  glEnableVertexAttribArray(0);

  // tex coords
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (GLvoid*)(2 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);
  /* ----------------- */
}

void ex_framebuffer_first()
{
  // first render pass
  glViewport(0, 0, width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
}

void ex_framebuffer_render_quad()
{
  // second render pass 
  glViewport(0, 0, width, height);
  // glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_FRAMEBUFFER_SRGB);

  // render screen quad
  glUseProgram(ex_fbo_shader);
  glBindVertexArray(fbo_vao);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(ex_uniform(ex_fbo_shader, "u_texture"), 0);
  glBindTexture(GL_TEXTURE_2D, colorbuffer);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void ex_framebuffer_destroy()
{
  glDeleteBuffers(1, &fbo_vbo);
  glDeleteVertexArrays(1, &fbo_vao);
  glDeleteRenderbuffers(1, &rbo);
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &colorbuffer);
}