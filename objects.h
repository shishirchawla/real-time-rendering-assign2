/* objects.h pknowles 2010-08-26 15:25:16 */

#ifndef OBJECTS_H
#define OBJECTS_H

#ifdef _WIN32
#include <windows.h>
#endif

/* For vertex buffer objects */
#define GL_GLEXT_PROTOTYPES

//#include <GL/gl.h>
#include <GLUT/glut.h> /* Mac OS X */
#include <stdarg.h>

typedef struct {
	float x, y, z;
} vector_t;

typedef struct {
	float u, v;
} parametric_t;

typedef struct {
	vector_t vert;
	vector_t norm;
} vertex_t;

typedef struct ObjectType {
	GLuint vertexBuffer;
	GLuint elementBuffer;
	GLuint normalBuffer;
	int numElements;
} Object;

typedef vertex_t (*ParametricObjFunc)(float, float, va_list*);

vertex_t parametricGrid(float u, float v, va_list* args); /* args: null */
vertex_t parametricSphere(float u, float v, va_list* args); /* args: radius */
vertex_t parametricTorus(float u, float v, va_list* args); /* args: inner-radius, outer-radius */

/*
USAGE:
myobject = createObject(<a parametric function from the list above>, <tessellation x>, <tessellation y>, <function arguments (args)>);
 */
Object* createObject(ParametricObjFunc parametric, int x, int y, ...);
Object* createObjectShader(ParametricObjFunc parametric, int x, int y, ...);
void drawObject(Object* obj);
void drawObjectNormals(Object* obj);
void drawObjectShader(Object* obj);
void freeObject(Object* obj);

#endif
