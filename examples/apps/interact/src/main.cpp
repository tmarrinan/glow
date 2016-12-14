#include <iostream>

#include "glow.h"


using namespace std;

void display(glow *gl, int win, unsigned long t, unsigned int dt, void *data);
void resize(glow *gl, int win, unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, void *data);
void onMouseDown(glow *gl, int win, unsigned short button, int x, int y, void *data);
void onMouseUp(glow *gl, int win, unsigned short button, int x, int y, void *data);
void onMouseMove(glow *gl, int wid, int x, int y, void *data);
void onKeyDown(glow *gl, int wid, unsigned short key, int x, int y, void *data);
void onKeyUp(glow *gl, int wid, unsigned short key, int x, int y, void *data);
void onScrollWheel(glow *gl, int wid, int dx, int dy, int x, int y, void *data);

int main (int argc, char **argv) {
	glow *gl = new glow();

	//gl->initialize(GLOW_OPENGL_CORE, 3, 2, GLOW_FLAGS_NONE);
	gl->initialize(GLOW_OPENGL_LEGACY, 0, 0, GLOW_FLAGS_NONE);
	int win = gl->createWindow("Interaction", GLOW_CENTER_HORIZONTAL, GLOW_CENTER_VERTICAL, 1024, 768, GLOW_WINDOW_BASE);

	gl->renderFunction(win, display, NULL);
	gl->resizeFunction(win, resize, NULL);
	
	gl->mouseDownListener(win, onMouseDown, NULL);
	gl->mouseUpListener(win, onMouseUp, NULL);
	gl->mouseMoveListener(win, onMouseMove, NULL);
	gl->keyDownListener(win, onKeyDown, NULL);
	gl->keyUpListener(win, onKeyUp, NULL);
	gl->scrollWheelListener(win, onScrollWheel, NULL);
	
	string version, shadingVersion;
	gl->getGLVersions(&version, &shadingVersion);
	printf("Using OpenGL: %s, GLSL: %s\n", version.c_str(), shadingVersion.c_str());

	glClearColor(1.0, 1.0, 1.0, 1.0);

	gl->runLoop();

	return 0;
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

void onMouseDown(glow *gl, int win, unsigned short button, int x, int y, void *data) {
	switch (button) {
		case GLOW_MOUSE_BUTTON_LEFT:
			printf("left mouse button down: %d,%d\n", x, y);
			break;
		case GLOW_MOUSE_BUTTON_RIGHT:
			printf("right mouse button down: %d,%d\n", x, y);
			break;
		default:
			break;
	}
}

void onMouseUp(glow *gl, int win, unsigned short button, int x, int y, void *data) {
	switch (button) {
		case GLOW_MOUSE_BUTTON_LEFT:
			printf("left mouse button up: %d,%d\n", x, y);
			break;
		case GLOW_MOUSE_BUTTON_RIGHT:
			printf("right mouse button up: %d,%d\n", x, y);
			break;
		default:
			break;
	}
}

void onMouseMove(glow *gl, int wid, int x, int y, void *data) {
	//printf("mouse move: %d, %d\n", x, y);
}

void onKeyDown(glow *gl, int wid, unsigned short key, int x, int y, void *data) {
	printf("key down: %u\n", key);
}

void onKeyUp(glow *gl, int wid, unsigned short key, int x, int y, void *data) {
	printf("key up: %u\n", key);
}

void onScrollWheel(glow *gl, int wid, int dx, int dy, int x, int y, void *data) {
	printf("scroll wheel: %d, %d\n", dx, dy);
}
