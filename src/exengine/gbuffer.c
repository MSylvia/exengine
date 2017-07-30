#include "gbuffer.h"
#include "window.h"
#include "shader.h"

GLuint gbuffer, gpositon, gnormal, gcolorspec, grenderbuffer;
GLuint gvao, gvbo;
GLuint gshader, gmainshader;
int width, height;

void gbuffer_init()
{
  glGenFramebuffers(1, &gbuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

  glfwGetFramebufferSize(display.window, &width, &height);

  // position buffer
  glGenTextures(1, &gpositon);
  glBindTexture(GL_TEXTURE_2D, gpositon);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gpositon, 0);

  // normal buffer
  glGenTextures(1, &gnormal);
  glBindTexture(GL_TEXTURE_2D, gnormal);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gnormal, 0);

  // color + spec (albedo)
  glGenTextures(1, &gcolorspec);
  glBindTexture(GL_TEXTURE_2D, gcolorspec);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gcolorspec, 0);

  // set buffers for rendering
  GLuint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
  glDrawBuffers(3, attachments);

  // generate render buffer
  glGenRenderbuffers(1, &grenderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, grenderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, grenderbuffer);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    printf("Error! Framebuffer is not complete in gbuffer\n");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // compile shaders
  gshader     = shader_compile("data/gbuffer.vs", "data/gbuffer.fs", NULL);
  gmainshader = shader_compile("data/gmain.vs", "data/gmain.fs", NULL);

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
  glGenVertexArrays(1, &gvao);
  glGenBuffers(1, &gvbo);
  glBindVertexArray(gvao);

  // vertices
  glBindBuffer(GL_ARRAY_BUFFER, gvbo);
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

void gbuffer_first()
{
  glUseProgram(gshader);
  glViewport(0, 0, width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glCullFace(GL_BACK);
}

void gbuffer_render(GLuint shader)
{
  // bind textures
  glUniform1i(glGetUniformLocation(shader, "u_position"), 0);
  glUniform1i(glGetUniformLocation(shader, "u_norm"), 1);
  glUniform1i(glGetUniformLocation(shader, "u_colorspec"), 2);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gpositon);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gnormal);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, gcolorspec);

  glUniform1i(glGetUniformLocation(shader, "u_point_depth"), 4);

  // draw quad
  glBindVertexArray(gvao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void gbuffer_destroy()
{
  glDeleteRenderbuffers(1, &grenderbuffer);
  glDeleteFramebuffers(1, &gbuffer);
  glDeleteTextures(1, &gpositon);
  glDeleteTextures(1, &gnormal);
  glDeleteTextures(1, &gcolorspec);
}