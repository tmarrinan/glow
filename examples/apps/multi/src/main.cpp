#include <iostream>

#include "glow.h"


using namespace std;

void init(glow *gl, int win, float bg[3]);
void display(glow *gl, int win, unsigned long t, unsigned int dt, void *data);
void resize(glow *gl, int win, unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, void *data);

int main (int argc, char **argv) {
	glow *gl = new glow();

	gl->initialize(GLOW_OPENGL_CORE, 3, 2, GLOW_FLAGS_NONE);
	int win1 = gl->createWindow("Window #1", 600,  24, 1024, 768, GLOW_WINDOW_BASE);
	int win2 = gl->createWindow("Window #2",   0,  24,  600, 400, GLOW_WINDOW_BASE);

	gl->renderFunction(win1, display, NULL);
	gl->resizeFunction(win1, resize, NULL);

	gl->renderFunction(win2, display, NULL);
	gl->resizeFunction(win2, resize, NULL);


	string version, shadingVersion;
	gl->getGLVersions(&version, &shadingVersion);
	printf("Using OpenGL: %s, GLSL: %s\n", version.c_str(), shadingVersion.c_str());

	float bg1[3] = {0.1, 0.1, 0.6}; 
	float bg2[3] = {0.6, 0.1, 0.1};
	init(gl, win1, bg1);
	init(gl, win2, bg2);

	gl->runLoop();

	return 0;
}

void init(glow *gl, int win, float bg[3]) {
    gl->setActiveWindow(win);

    glClearColor(bg[0], bg[1], bg[2], 1.0);
}

void display(glow *gl, int win, unsigned long t, unsigned int dt, void *data) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->swapBuffers(win);
}


void resize(glow *gl, int win, unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, void *data) {
    printf("window size: %ux%u (%ux%u)\n", wW, wH, rW, rH);
	glViewport(0, 0, rW, rH);
	gl->requestRenderFrame(win);
}
