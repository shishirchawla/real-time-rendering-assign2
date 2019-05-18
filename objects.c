/* objects.c pknowles 2010-08-26 15:25:16 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "objects.h"

vertex_t parametricGrid(float u, float v, va_list* args)
{
	vertex_t ret;

	ret.norm.x = 0.0;
	ret.norm.y = 0.0;
	ret.norm.z = 1.0;
	ret.vert.x = (u - 0.5)*2.0;
	ret.vert.y = (v - 0.5)*2.0;
	ret.vert.z = 0.0;
	return ret;
}

vertex_t parametricSphere(float u, float v, va_list* args)
{
	/* http://mathworld.wolfram.com/Sphere.html */
	float radius;
	float pi = acosf(-1.0f);
	vertex_t ret;

	radius = va_arg(*args, double);
	u *= 2.0f * pi;
	v *= pi;
	ret.norm.x = cos(u) * sin(v);
	ret.norm.y = sin(u) * sin(v);
	ret.norm.z = cos(v);
	ret.vert.x = radius * ret.norm.x;
	ret.vert.y = radius * ret.norm.y;
	ret.vert.z = radius * ret.norm.z;
	return ret;
}

vertex_t parametricTorus(float u, float v, va_list* args)
{
	/* http://mathworld.wolfram.com/Torus.html */
	float R;
	float r;
	float pi = acosf(-1.0f);
	vertex_t ret;

	R = va_arg(*args, double);
	r = va_arg(*args, double);
	u *= 2.0f * pi;
	v *= 2.0f * pi;
	ret.norm.x = cos(u) * cos(v);
	ret.norm.y = sin(u) * cos(v);
	ret.norm.z = sin(v);
	ret.vert.x = (R + r * cos(v)) * cos(u);
	ret.vert.y = (R + r * cos(v)) * sin(u);
	ret.vert.z = r * sin(v);
	return ret;
}

Object* createObject(ParametricObjFunc paramObjFunc, int x, int y, ...)
{
	va_list args;
	unsigned int i, j;
	float u, v;
	int ci = 0; /* current index */
	vertex_t* vertices;
	unsigned int* indices;
	vector_t* normals;
	int numVertices;
	int numIndices;
	Object* obj;
#define INDEX(I, J) ((I)*y + (J))

	/* Initialize data */
	numVertices = x * y;
	numIndices = (y-1) * (x * 2 + 2);
	vertices = (vertex_t*)malloc(sizeof(vertex_t) * numVertices);
	indices = (unsigned int*)malloc(sizeof(unsigned int) * numIndices);
	normals = (vector_t*)malloc(sizeof(vector_t) * numVertices * 2);


	int normalCount = 0;
	float normalLength = 0.2;
	/* Construct vertex data */
	for (i = 0; i < x; ++i) {
		u = i/(float)(x-1);
		for (j = 0; j < y; ++j) {
			v = j/(float)(y-1);
			va_start(args, y);
			vertices[INDEX(i, j)] = paramObjFunc(u, v, &args);

			/* normal data */
			vector_t nv1, nv2;
			nv1 = vertices[INDEX(i,j)].vert;
			nv2.x = (vertices[INDEX(i,j)].vert.x + (vertices[INDEX(i,j)].norm.x * normalLength)) ;
			nv2.y = (vertices[INDEX(i,j)].vert.y + (vertices[INDEX(i,j)].norm.y * normalLength)) ;
			nv2.z = (vertices[INDEX(i,j)].vert.z + (vertices[INDEX(i,j)].norm.z * normalLength)) ;
			normals[normalCount++] = nv1;
			normals[normalCount++] = nv2 ;
		}
	}

	/* Construct index data */
	for (j = 0; j < y-1; ++j) {
		indices[ci++] = INDEX(0, j);
		for (i = 0; i < x; ++i) {
			indices[ci++] = INDEX(i, j);
			indices[ci++] = INDEX(i, j+1);
		}
		indices[ci++] = INDEX(i-1, j+1);
	}

	/* Double check the loops populated the data correctly */
	assert(ci == numIndices);

	/* Create VBOs */
	obj = (Object*)malloc(sizeof(Object));
	glGenBuffers(1, &obj->vertexBuffer);
	glGenBuffers(1, &obj->elementBuffer);
	glGenBuffers(1, &obj->normalBuffer);

	/* Buffer the vertex data */
	glBindBuffer(GL_ARRAY_BUFFER, obj->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t) * numVertices, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Buffer the normal data */
	glBindBuffer(GL_ARRAY_BUFFER, obj->normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vector_t) * numVertices * 2, normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Buffer the index data */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * numIndices, indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* Cleanup and return the object struct */
	obj->numElements = numIndices;
	free(vertices);
	free(normals);
	free(indices);
	return obj;
}

void drawObject(Object* obj)
{
	/* Enable vertex arrays and bind VBOs */
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, obj->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->elementBuffer);

	/* Draw object */
	glVertexPointer(3, GL_FLOAT, sizeof(vertex_t), (void*)0);
	glNormalPointer(GL_FLOAT, sizeof(vertex_t), (void*)sizeof(vector_t));
	glDrawElements(GL_TRIANGLE_STRIP, obj->numElements, GL_UNSIGNED_INT, (void*)0);

	/* Unbind/disable arrays. could also push/pop enables */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void drawObjectNormals(Object* obj)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, obj->normalBuffer);

	glVertexPointer(3, GL_FLOAT, 0, (void*)0);

	glDrawArrays(GL_LINES, 0, obj->numElements * 2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

Object* createObjectShader(ParametricObjFunc paramObjFunc, int x, int y, ...)
{
	unsigned int i, j;
	float u, v;
	int ci = 0; /* current index */
	parametric_t* vertices;
	unsigned int* indices;
	int numVertices;
	int numIndices;
	Object* obj;
#define INDEX(I, J) ((I)*y + (J))

	/* Initialize data */
	numVertices = x * y;
	numIndices = (y-1) * (x * 2 + 2);
	vertices = (parametric_t*)malloc(sizeof(parametric_t) * numVertices);
	indices = (unsigned int*)malloc(sizeof(unsigned int) * numIndices);

	/* Construct vertex data */
	for (i = 0; i < x; ++i) {
		u = i/(float)(x-1);
		for (j = 0; j < y; ++j) {
			v = j/(float)(y-1);
			vertices[INDEX(i, j)] = (parametric_t){.u = u, .v = v};
		}
	}

	/* Construct index data */
	for (j = 0; j < y-1; ++j) {
		indices[ci++] = INDEX(0, j);
		for (i = 0; i < x; ++i) {
			indices[ci++] = INDEX(i, j);
			indices[ci++] = INDEX(i, j+1);
		}
		indices[ci++] = INDEX(i-1, j+1);
	}

	/* Double check the loops populated the data correctly */
	assert(ci == numIndices);

	/* Create VBOs */
	obj = (Object*)malloc(sizeof(Object));
	glGenBuffers(1, &obj->vertexBuffer);
	glGenBuffers(1, &obj->elementBuffer);

	/* Buffer the vertex data */
	glBindBuffer(GL_ARRAY_BUFFER, obj->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(parametric_t) * numVertices, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Buffer the index data */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * numIndices, indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* Cleanup and return the object struct */
	obj->numElements = numIndices;
	free(vertices);
	free(indices);
	return obj;
}

void drawObjectShader(Object* obj)
{
	/* Enable vertex arrays and bind VBOs */
	glEnableClientState(GL_VERTEX_ARRAY);
	//glEnableClientState(GL_NORMAL_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, obj->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->elementBuffer);

	/* Draw object */
	glVertexPointer(2, GL_FLOAT, sizeof(parametric_t), (void*)0);
	glDrawElements(GL_TRIANGLE_STRIP, obj->numElements, GL_UNSIGNED_INT, (void*)0);

	/* Unbind/disable arrays. could also push/pop enables */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void freeObject(Object* obj)
{
	glDeleteBuffers(1, &obj->vertexBuffer);
	glDeleteBuffers(1, &obj->normalBuffer);
	glDeleteBuffers(1, &obj->elementBuffer);
	obj->vertexBuffer = 0;
	obj->normalBuffer = 0;
	obj->elementBuffer = 0;
	obj->numElements = 0;
}

