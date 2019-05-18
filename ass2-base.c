/* $Id: tute-vertex.c 18 2006-07-26 10:28:03Z aholkner $ */
/* Updated pknowles, gl 2010 */

#ifndef __APPLE__
#include <GL/glew.h>
#endif
//#include <GL/glut.h> /* for screen text only */
#include <GLUT/glut.h> /* Mac OS X */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "shaders.h"
#include "sdl-base.h"
#include "objects.h"

#define CAMERA_VELOCITY 0.005		 /* Units per millisecond */
#define CAMERA_ANGULAR_VELOCITY 0.05	 /* Degrees per millisecond */
#define CAMERA_MOUSE_X_VELOCITY 0.3	 /* Degrees per mouse unit */
#define CAMERA_MOUSE_Y_VELOCITY 0.3	 /* Degrees per mouse unit */

#ifndef min
#define min(a, b) ((a)>(b)?(b):(a))
#endif
#ifndef max
#define max(a, b) ((a)>(b)?(a):(b))
#endif
#ifndef clamp
#define clamp(x, a, b) min(max(x, a), b)
#endif

static float camera_zoom;
static float camera_heading;	/* Direction in degrees the camera faces */
static float camera_pitch;	/* Up/down degrees for camera */
static int mouse1_down;		/* Left mouse button Up/Down. Only move camera when down. */
static int mouse2_down;		/* Right mouse button Up/Down. Only zoom camera when down. */

/* Object data */
Object* object = NULL;
static int tessellation = 2; /* Tessellation level */
const int min_tess = 2;
const int max_tess = 10;
const int min_shininess = 10.0;
const int max_shininess = 120.0;
float shapeRotation = 0; /* Shape Rotation */

/* Store the state (1 = pressed, 0 = not pressed) of each key  we're interested in. */
static char key_state[1024];

/* The opengl handle to our shader */
GLuint shader = 0;

/* Shapes */
enum Shape {
  SPHERE_S = 0,
  TORUS_S,
  GRID_S,
  NUM_SHAPES
} shape_t = SPHERE_S;
ParametricObjFunc shape_func = parametricSphere;

/* Bump Types */
enum Bump {
  NO_BUMPS = 0,
  BUMP_NORMALS_ONLY,
  BUMP_DISPLACEMENT,
  NUM_BUMP_STATES
} bump_t = NO_BUMPS;

/* Store render state variables.  Can be toggled with function keys. */
static struct {
  int wireframe;
  int lighting;
  int flatOrSmooth;
  int shaders;
  int lightingModel;
  GLboolean viewer_model;
  int vertexOrPixelLighting;
  int stateOSDorConsole;  // 0 - console, 1 - osd
  int animation;
  int normals;
} renderstate;

/* Light and materials */
static float light0_position[] = {2.0, 2.0, 2.0, 0.0};
static float material_ambient[] = {0.5, 0.5, 0.5, 1.0};
static float material_diffuse[] = {1.0, 0.0, 0.0, 1.0};
static float material_specular[] = {1.0, 1.0, 1.0, 1.0};
static float material_shininess = 50.0;

/* Scene globals */
int currentFramerate;
float currentFrametime;

void update_renderstate()
{
  if (renderstate.lighting)
    glEnable(GL_LIGHTING);
  else
    glDisable(GL_LIGHTING);

  glShadeModel(renderstate.flatOrSmooth ? GL_FLAT : GL_SMOOTH);

  glPolygonMode(GL_FRONT_AND_BACK, renderstate.wireframe ? GL_LINE : GL_FILL);
}

void regenerate_geometry()
{
  int subdivs;
  subdivs = 1 << (tessellation);

  /* Free previous object */
  if (object) 
    freeObject(object);

  fflush(stdout);

  /* Generate the new object. NOTE: different equations require different arguments. see objects.h */
  if (!renderstate.shaders)
    object = createObject(shape_func, subdivs + 1, subdivs + 1, 1.0, 0.5, 0.4);
  else
    object = createObjectShader(shape_func, subdivs + 1, subdivs + 1, 1.0, 0.5, 0.4);

  fflush(stdout);
}

void init()
{
  int argc = 0;
  char** argv = NULL;
  glutInit(&argc, argv); /* NOTE: this hack will not work on windows */
#ifndef __APPLE__
  glewInit();
#endif

  /* Load the shader */
  shader = getShader("shader.vert", "shader.frag");

  /* Lighting and colours */
  glClearColor(0, 0, 0, 0);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHT0);
  glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);
  glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, material_shininess);

  /* Camera */
  camera_zoom = 5.0;
  camera_heading = 0.0;
  camera_pitch = 0.0;
  mouse1_down = 0;
  mouse2_down = 0;

  /* Turn off all keystate flags */
  memset(key_state, 0, 1024);

  /* Default render modes */
  renderstate.wireframe = 0;
  renderstate.lighting = 1;
  renderstate.flatOrSmooth = 0;
  renderstate.shaders = 0;
  renderstate.lightingModel = 0;
  renderstate.vertexOrPixelLighting = 0;
  renderstate.stateOSDorConsole = 1;
  renderstate.animation = 0;
  renderstate.normals = 0;
  glGetBooleanv(GL_LIGHT_MODEL_LOCAL_VIEWER, &renderstate.viewer_model);

  update_renderstate();

  regenerate_geometry();
}

void reshape(int width, int height)
{
  glViewport(0, 0, width, height);

  /* Reset the projection matrix */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, width / (double) height, 0.1, 100.0);
  glMatrixMode(GL_MODELVIEW);
}

/* Draws buffer on screen. */
void drawString(char buffer[], int posX, int posY)
{
  char *bufp;

  glRasterPos2i(posX, posY);
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);
}

/* Prints State Information */
void printStateInfo(SDL_Surface *surface)
{
  /* if surface provided - draw on surface, else print on console */
  /* -> expects the surface to have correct projection setup for drawing bitmap. */
  if (surface) {
    char buffer[64];
    int posX = 10;
    int posY = surface->h - 5;
    int lineDelta = 15;
    int lineNum = 1;

    snprintf(buffer, sizeof buffer, "Shaders (s): %d", renderstate.shaders);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "FR: %d", currentFramerate);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Lighting (l): %d", renderstate.lighting);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Light Position (d): %d", (int)light0_position[3]);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Viewer Position (v): %d", renderstate.viewer_model);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Wireframe/Fill (w): %d", renderstate.wireframe);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Lighting Model (m): %d", renderstate.lightingModel);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Vertex/Pixel Lighting (p): %d", renderstate.vertexOrPixelLighting);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Animation (a): %d", renderstate.animation);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Normals (n): %d", renderstate.normals);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Shape (g): %d", shape_t);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Bumps (b): %d", bump_t);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Flat/Smooth Shading(f): %d", renderstate.flatOrSmooth);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Tesselation(T/t): %d", tessellation);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Shininess(H/h): %.0f", material_shininess);
    drawString(buffer, posX, posY-(lineNum++ * lineDelta));
    snprintf(buffer, sizeof buffer, "Switch between OSD and Console (o)");
    drawString(buffer, 10, 10);
  }
  else
  {
    printf("Shaders (s): %d\n", renderstate.shaders);
    printf("FR: %d\n", currentFramerate);
    printf("Lighting (l): %d\n", renderstate.lighting);
    printf("Light Position (d): %d\n", (int)light0_position[3]);
    printf("Viewer Position (v): %d\n", renderstate.viewer_model);
    printf("Wireframe/Fill (w): %d\n", renderstate.wireframe);
    printf("Lighting Model (m): %d\n", renderstate.lightingModel);
    printf("Vertex/Pixel Lighting (p): %d\n", renderstate.vertexOrPixelLighting);
    printf("Animation (a): %d\n", renderstate.animation);
    printf("Normals (n): %d\n", renderstate.normals);
    printf("Shape (g): %d\n", shape_t);
    printf("Bumps (b): %d\n", bump_t);
    printf("Flat/Smooth Shading(f): %d\n", renderstate.flatOrSmooth);
    printf("Tesselation(T/t): %d\n", tessellation);
    printf("Shininess(H/h): %.0f\n", material_shininess);
    printf("Switch between OSD and Console (o)\n");
  }
}

void drawOSD(SDL_Surface *surface)
{
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  /* Apply an orthographic projection temporarily */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, surface->w, 0, surface->h, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  /* Draw state info. */
  printStateInfo(surface);

  glPopMatrix();	/* Pop modelview */
  glMatrixMode(GL_PROJECTION);

  glPopMatrix();	/* Pop projection */
  glMatrixMode(GL_MODELVIEW);

  glPopAttrib();
}

void display(SDL_Surface *surface)
{
  /* Clear the colour and depth buffer */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Camera transformation */
  glLoadIdentity();
  glTranslatef(0, 0, -camera_zoom);
  glRotatef(-camera_pitch, 1, 0, 0);
  glRotatef(-camera_heading, 0, 1, 0);

  /* Set the light position (gets multiplied by the modelview matrix) */
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

  /* Draw the scene */
  /* Apply shape rotation and draw shape */
  glRotatef(shapeRotation, 0.0f, 1.0f, 0.0f);
  if (renderstate.shaders) {
    glUseProgram(shader); /* Use our shader for future rendering */
    drawObjectShader(object);
  } else {
    drawObject(object);
    if (renderstate.normals)
      drawObjectNormals(object);
  }

  glUseProgram(0);

  /* Draw OSD */
  if (renderstate.stateOSDorConsole)
    drawOSD(surface);

  CHECK_GL_ERROR;
}

/* Modifies shape rotation based on time factor. */
void animate(float dt)
{
  float speed = 25.0;

  shapeRotation += dt * speed;
  if (shapeRotation > 360.0f)
    shapeRotation -= 360.0f;
}


/* Called continuously. dt is time between frames in seconds */
void update(float dt)
{
  static float fpsTime = 0.0f;
  static int fpsFrames = 0;
  fpsTime += dt;
  fpsFrames += 1;
  if (fpsTime > 1.0f)
  {
    currentFramerate = fpsFrames / fpsTime;
    currentFrametime = (1 / currentFramerate) * 1000;
    fpsTime = 0.0f;
    fpsFrames = 0;

    /* if console info turned on - update every one second */
    if (!renderstate.stateOSDorConsole)
      printStateInfo(0);
  }

  /* if animation turned on - change shape rotation */
  if (renderstate.animation)
    animate(dt);
}

void set_mousestate(unsigned char button, int state)
{
  switch (button) {
    case SDL_BUTTON_LEFT:
      mouse1_down = state;
      break;
    case SDL_BUTTON_RIGHT:
      mouse2_down = state;
      break;
  }
}

void event(SDL_Event *event)
{
  static int first_mousemotion = 1;

  switch (event->type) {
    case SDL_KEYDOWN:
      key_state[event->key.keysym.sym] = 1;

      /* Handle non-state keys */
      switch (event->key.keysym.sym) {
        case SDLK_ESCAPE:
          quit();
          break;
        case SDLK_a:
          renderstate.animation = !renderstate.animation;
          break;
        case SDLK_f:
          renderstate.flatOrSmooth = !renderstate.flatOrSmooth;
          update_renderstate();
          break;
        case SDLK_o:
          renderstate.stateOSDorConsole = !renderstate.stateOSDorConsole;
          break;
        case SDLK_s:
          renderstate.shaders = !renderstate.shaders;
          regenerate_geometry();
          break;
        case SDLK_l:
          renderstate.lighting = !renderstate.lighting;
          update_renderstate();
          break;
        case SDLK_n:
          renderstate.normals = !renderstate.normals;
          // Set normal_view in shader
          GLint normal_view = glGetUniformLocation(shader, "normal_view");
          if (normal_view != -1)
          {
            glUseProgram(shader); 
            glUniform1i(normal_view, renderstate.normals);
            glUseProgram(0);
          }
          break;
        case SDLK_w:
          renderstate.wireframe = !renderstate.wireframe;
          update_renderstate();
          break;
        case SDLK_t:
          if ((key_state[SDLK_LSHIFT] || key_state[SDLK_RSHIFT])) {
            if (tessellation < max_tess) {
              ++tessellation;
              regenerate_geometry();
            } 
          }
          else {
            if (tessellation > min_tess) {
              --tessellation;
              regenerate_geometry();
            }
          }
          break;
        case SDLK_h:
          if ((key_state[SDLK_LSHIFT] || key_state[SDLK_RSHIFT])) {
            if (material_shininess < max_shininess) {
              material_shininess += 10.0;
            }
          }
          else {
            if (material_shininess > min_shininess) {
              material_shininess -= 10.0;
            }
          }
          glMaterialf(GL_FRONT, GL_SHININESS, material_shininess);
          break;
        case SDLK_v:
          {
            renderstate.viewer_model = !renderstate.viewer_model;
            // set viewer position in fixed pipeline
            glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, renderstate.viewer_model);
            // set viewer position in shaders
            GLint viewer = glGetUniformLocation(shader, "viewer");
            if (viewer != -1)
            {
              glUseProgram(shader); 
              glUniform1i(viewer, renderstate.viewer_model);
              glUseProgram(0);
            }
          }
          break;
        case SDLK_d:
          // change light 'w' value - directional/positional
          light0_position[3] = !light0_position[3];
          break;
        case SDLK_b:
          // set bump state in shader
          bump_t = (bump_t + 1) % (NUM_BUMP_STATES);
          GLint bumps = glGetUniformLocation(shader, "bumps");
          if (bumps != -1)
          {
            glUseProgram(shader);
            glUniform1i(bumps, bump_t);
            glUseProgram(0);
          }
          break;
        case SDLK_m:
          // set lighting model (phong/blinn-phong) in shader
          renderstate.lightingModel = !renderstate.lightingModel;
          GLint lighting_model = glGetUniformLocation(shader, "lighting_model");
          if (lighting_model != -1)
          {
            glUseProgram(shader); 
            glUniform1i(lighting_model, renderstate.lightingModel);
            glUseProgram(0);
          }
          break;
        case SDLK_p:
          // set lighting mode (vertex / pixel) in shader
          renderstate.vertexOrPixelLighting = !renderstate.vertexOrPixelLighting;
          GLint shader_type = glGetUniformLocation(shader, "shader_type");
          if (shader_type != -1)
          {
            glUseProgram(shader);
            glUniform1i(shader_type, renderstate.vertexOrPixelLighting);
            glUseProgram(0);
          }
          break;
        case SDLK_g:
          // set appropriate shape func based on switch
          shape_t = (shape_t + 1) % (NUM_SHAPES);
          switch(shape_t)
          {
            case 0:
              shape_func = parametricSphere;
              break;
            case 1:
              shape_func = parametricTorus;
              break;
            case 2:
              shape_func = parametricGrid;
              break;
            default:
              break;
          }
          // set shape type in shader
          GLint shape = glGetUniformLocation(shader, "shape");
          if (shape != -1)
          {
            glUseProgram(shader);
            glUniform1i(shape, shape_t);
            glUseProgram(0);
          }
          regenerate_geometry(); 
          break;
        default:
          break;
      }
      break;
    case SDL_KEYUP:
      key_state[event->key.keysym.sym] = 0;
      break;

    case SDL_MOUSEBUTTONDOWN:
      set_mousestate(event->button.button, 1);
      break;

    case SDL_MOUSEBUTTONUP:
      set_mousestate(event->button.button, 0);
      break;

    case SDL_MOUSEMOTION:
      if (first_mousemotion) {
        /* The first mousemotion event will have bogus xrel and
           yrel, so ignore it. */
        first_mousemotion = 0;
        break;
      }
      if (mouse1_down) {
        /* Only move the camera if the mouse is down*/
        camera_heading -= event->motion.xrel * CAMERA_MOUSE_X_VELOCITY;
        camera_pitch -= event->motion.yrel * CAMERA_MOUSE_Y_VELOCITY;
      }
      if (mouse2_down) {
        camera_zoom -= event->motion.yrel * CAMERA_MOUSE_Y_VELOCITY * 0.1;
      }
      break;
    default:
      break;
  }
}

void cleanup()
{
  /* Delete the shader */
  glDeleteProgram(shader);

  /* Free object data */
  if (object) 
    freeObject(object);
}
