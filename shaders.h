/* pknowles Wed 18 Aug 2010 09:21:32 PM EST */

/*
NOTE: make sure to call glewInit before loading shaders

use getShader() to load, compile shaders and return a program
use glUseProgram(program) to activate it
use glUseProgram(0) to return to fixed pipeline rendering
use glDeleteProgram() to free resources
*/

#ifndef SHADERS_H
#define SHADERS_H

#define CHECK_GL_ERROR oglError(__LINE__, __FILE__)
int oglError(int line, const char* file);
GLuint getShader(const char* vertexFile, const char* fragmentFile);
#endif
