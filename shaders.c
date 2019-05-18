/* pknowles Wed 18 Aug 2010 09:21:32 PM EST */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __APPLE__
#include <GL/glew.h>
#endif
//#include <GL/gl.h>
//#include <GL/glu.h>
#include <GLUT/glut.h> /* Mac OS X */

#ifdef _WIN32
#pragma warning(disable:4996)
#include <windows.h>
#endif

#include "shaders.h"

int oglError(int line, const char* file)
{
  GLenum glErr;
  int retCode = 0;
  while ((glErr = glGetError()) != GL_NO_ERROR) {
    /* Extract file name from path */
    const char* p = strrchr(file, '\\');
    if (p != NULL) 
      file = p+1;
    /* Print each error message */
    printf("glError 0x%x in %s at %i: %s\n", glErr, file, line, gluErrorString(glErr));
    retCode = 1;
  }
  return retCode;
}

int shaderError(GLuint shader, const char* name)
{
  int infologLength = 0;
  int charsWritten = 0;
  int success = 0;
  GLchar *infoLog;

  CHECK_GL_ERROR;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
  CHECK_GL_ERROR;
  if (!success) {
    if (infologLength > 1) {
      infoLog = (GLchar *)malloc(infologLength);
      glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
      printf("Shader InfoLog (%s):\n%s", name, infoLog);
      free(infoLog);
    } else
      printf("Shader InfoLog (%s): <no info log>\n", name);
    return 1;
  }
  CHECK_GL_ERROR;
  return 0;
}

int programError(GLuint program, const char* vert, const char* frag)
{
  int infologLength = 0;
  int charsWritten = 0;
  int success = 0;
  GLchar *infoLog;

  CHECK_GL_ERROR;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);
  CHECK_GL_ERROR;
  if (!success) {
    if (infologLength > 1) {
      infoLog = (GLchar *)malloc(infologLength);
      glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog);
      printf("Program InfoLog (%s/%s):\n%s", vert, frag, infoLog);
      free(infoLog);
    } else
      printf("Program InfoLog (%s/%s): <no info log>\n", vert, frag);
    return 1;
  }
  CHECK_GL_ERROR;
  return 0;
}

char* readFile(const char* filename)
{
  int size;
  char* data;
  FILE* file = fopen(filename, "rb");
  if (!file) return NULL;
  fseek(file, 0, 2);
  size = ftell(file);
  fseek(file, 0, 0);
  data = (char*)malloc(size+1);
  fread(data, sizeof(char), size, file);
  fclose(file);
  data[size] = '\0';
  return data;
}

void cleanupShader(GLuint vert, GLuint frag, char *vertSrc, char *fragSrc) 
{
  glDeleteShader(vert);
  glDeleteShader(frag);
  free(vertSrc);
  free(fragSrc);
}

GLuint getShader(const char* vertexFile, const char* fragmentFile)
{
  char* vertSrc;
  char* fragSrc;

  CHECK_GL_ERROR;

  /* Read the contents of the source files */
  vertSrc = readFile(vertexFile);
  fragSrc = readFile(fragmentFile);

  /* Check they exist */
  if (!vertSrc || !fragSrc) {
    free(vertSrc); 
    free(fragSrc); 
    printf("Error reading shaders %s & %s\n", vertexFile, fragmentFile); 
    fflush(stdout); 
    return 0;
  }

  /* Create the shaders */
  GLuint vert, frag, program;
  vert = glCreateShader(GL_VERTEX_SHADER);
  frag = glCreateShader(GL_FRAGMENT_SHADER);

  /* Pass in the source code for the shaders */
  glShaderSource(vert, 1, (const GLchar**)&vertSrc, NULL);
  glShaderSource(frag, 1, (const GLchar**)&fragSrc, NULL);

  /* Compile and check each for errors */
  glCompileShader(vert);
  if (shaderError(vert, vertexFile)) {
    cleanupShader(vert, frag, vertSrc, fragSrc);
    return 0;
  }
  glCompileShader(frag);
  if (shaderError(frag, fragmentFile)) {
    cleanupShader(vert, frag, vertSrc, fragSrc);
    return 0;
  }

  /* Create program, attach shaders, link and check for errors */
  program = glCreateProgram();
  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);
  if (programError(program, vertexFile, fragmentFile)) {
    cleanupShader(vert, frag, vertSrc, fragSrc);
    glDeleteProgram(program); 
    return 0;
  }

  /* Clean up intermediates and return the program */
  cleanupShader(vert, frag, vertSrc, fragSrc);

  return program; /* NOTE: use glDeleteProgram to free resources */
}

