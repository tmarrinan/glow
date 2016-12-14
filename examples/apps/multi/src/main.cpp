#include <iostream>

#include "glow.h"


using namespace std;

void display1(glow *gl, int win, unsigned long t, unsigned int dt, void *data);
void display2(glow *gl, int win, unsigned long t, unsigned int dt, void *data);
void resize(glow *gl, int win, unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, void *data);
void animate(glow *gl, unsigned int id, void *data);

int main (int argc, char **argv) {
	glow *gl = new glow();

	//gl->initialize(GLOW_OPENGL_CORE, 3, 2, GLOW_FLAGS_NONE);
	gl->initialize(GLOW_OPENGL_LEGACY, 0, 0, GLOW_FLAGS_NONE);
	int win1 = gl->createWindow("Window #1", 600,  24, 1024, 768, GLOW_WINDOW_BASE);
	int win2 = gl->createWindow("Window #2",   0,  24,  600, 400, GLOW_WINDOW_BASE);

	gl->renderFunction(win1, display1, NULL);
	gl->resizeFunction(win1, resize, NULL);

	gl->renderFunction(win2, display2, NULL);
	gl->resizeFunction(win2, resize, NULL);

	gl->setTimeout(animate, 100, &win1);
	gl->setTimeout(animate, 100, &win2);
	
	string version, shadingVersion;
	gl->getGLVersions(&version, &shadingVersion);
	printf("Using OpenGL: %s, GLSL: %s\n", version.c_str(), shadingVersion.c_str());

	gl->runLoop();

	return 0;
}

void display1(glow *gl, int win, unsigned long t, unsigned int dt, void *data) {
	float c = (float)(abs((int)(t % 4000) - 2000)) / 2000.0f;
	
	glClearColor(0.2*c, 0.2*c, 0.8*c, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->swapBuffers(win);
}

void display2(glow *gl, int win, unsigned long t, unsigned int dt, void *data) {
	float c = (float)(abs((int)(t % 4000) - 2000)) / 2000.0f;
	
	glClearColor(0.8*c, 0.2*c, 0.2*c, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->swapBuffers(win);
}

void resize(glow *gl, int win, unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, void *data) {
    printf("window size: %ux%u (%ux%u)\n", wW, wH, rW, rH);
	glViewport(0, 0, rW, rH);
	gl->requestRenderFrame(win);
}

void animate(glow *gl, unsigned int id, void *data) {
	int win = *((int*)data);
	
	if (gl->requestRenderFrame(win))
		gl->setTimeout(animate, 100, data);
}
