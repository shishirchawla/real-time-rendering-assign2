#include <SDL/SDL.h>

#define GL_GLEXT_PROTOTYPES
//#include <GL/gl.h>
#include <GLUT/glut.h> /* Mac OS X */

/* Implement these yourself */
void init();
void reshape(int w, int h);
void update(float dt);
void display(SDL_Surface *screen);
void event(SDL_Event *event);
void cleanup();

/* Updated in the main loop */
extern int frame_rate;

/* Call this to quit. */
void quit();

