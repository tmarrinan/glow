#include <iostream>
#include <cmath>

#include "glow.h"


using namespace std;

void display(unsigned long t, unsigned int dt, glow *gl);
void idle(glow *gl);
void timeout(unsigned int id, glow *gl);
void resize(unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, glow *gl);
void onMouseDown(unsigned short button, int x, int y, glow *gl);
void onMouseUp(unsigned short button, int x, int y, glow *gl);
void onMouseMove(int x, int y, glow *gl);
void onScrollWheel(int dx, int dy, int x, int y, glow *gl);
void onKeyDown(unsigned short key, int x, int y, glow *gl);
void onKeyUp(unsigned short key, int x, int y, glow *gl);

unsigned int fontTex[2];

int main (int argc, char **argv) {
	if (strcmp(argv[0], "./text/text") != 0 && strcmp(argv[0], ".\\text\\text\\Debug\\text.exe") != 0) {
		printf(" program designed to be run from \'examples\' directory.\n");
		printf(" please cd to '<glow_dir>/examples' and run ./text/text\n");
		exit(1);
	}

    glow *gl = new glow();

    //gl->initialize(GLOW_OPENGL_CORE, 3, 2, GLOW_HIDPI_WINDOW);
    gl->initialize(GLOW_OPENGL_LEGACY, 0, 0, GLOW_BASE_WINDOW);
    gl->createWindow("TEST", GLOW_CENTER_HORIZONTAL, GLOW_CENTER_VERTICAL, 800, 640);

    gl->mouseDownListener(onMouseDown);
    gl->mouseUpListener(onMouseUp);
    gl->mouseMoveListener(onMouseMove);
    gl->scrollWheelListener(onScrollWheel);
    gl->keyDownListener(onKeyDown);
    gl->keyUpListener(onKeyUp);

    gl->renderFunction(display);
    gl->idleFunction(idle);
    gl->resizeFunction(resize);

    unsigned int t1 = gl->setTimeout(timeout, 5000);
    unsigned int t2 = gl->setTimeout(timeout, 1000);
    unsigned int t3 = gl->setTimeout(timeout, 2000);

    gl->cancelTimeout(t2);

    string version, shadingVersion;
    gl->getGLVersions(&version, &shadingVersion);
    printf("Using OpenGL: %s, GLSL: %s\n", version.c_str(), shadingVersion.c_str());

    string test1 = "的 uni";
	string test2 = "\xe7\x9a\x84 code \U00007684"; // \U unicode not working for Windows
	printf("%s %s\n", test1.c_str(), test2.c_str());

    GLOW_FontFace *face;
    unsigned char *textPx1, *textPx2;
    unsigned int textW1, textH1, textW2, textH2;
    unsigned char col[] = {255, 126, 0};
    //gl->createFontFace("./fonts/Arial.ttf", 96, &face);
    //gl->createFontFace("./fonts/AGaramondPro.otf", 96, &face);
    gl->createFontFace("./fonts/simsun.ttc", 96, &face);
    gl->renderStringToTexture(face, test1, true, &textW1, &textH1, &textPx1);
    gl->renderStringToTexture(face, test2, col, true, &textW2, &textH2, &textPx2);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenTextures(2, fontTex);
    glBindTexture(GL_TEXTURE_2D, fontTex[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, textW1, textH1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, textPx1);
    glBindTexture(GL_TEXTURE_2D, fontTex[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textW2, textH2, 0, GL_RGBA, GL_UNSIGNED_BYTE, textPx2);
    glBindTexture(GL_TEXTURE_2D, 0);

    printf("Text texture sizes: %dx%d, %dx%d\n", textW1, textH1, textW2, textH2);

    gl->runLoop();

    return 0;
}

void display(unsigned long t, unsigned int dt, glow *gl) {
    //printf("render: t=%.3f, dt=%.3f\n", (float)t/1000.0, (float)dt/1000.0);

    float c = (float)(abs((int)(t % 4000) - 2000)) / 2000.0f;
    glClearColor(c, c, c, 1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glBindTexture(GL_TEXTURE_2D, fontTex[0]);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.00f, -0.19f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(-0.25f, -0.19f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(-0.25f,  0.19f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.00f,  0.19f);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, fontTex[1]);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.25f, -0.19f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.00f, -0.19f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.00f,  0.19f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.25f,  0.19f);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

    gl->swapBuffers();
}

void idle(glow *gl) {
    gl->requestRenderFrame();
}

void timeout(unsigned int id, glow *gl) {
    printf("timeout: %u\n", id);
	gl->requestRenderFrame();
}

void resize(unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, glow *gl) {
    printf("window size: %ux%u (%ux%u)\n", wW, wH, rW, rH);
	glViewport(0, 0, rW, rH);
	gl->requestRenderFrame();
}

void onMouseDown(unsigned short button, int x, int y, glow *gl) {
    switch (button) {
        case GLOW_MOUSE_BUTTON_LEFT:
            printf("mousedown: left (%d, %d)\n", x, y);
            break;
        case GLOW_MOUSE_BUTTON_RIGHT:
            printf("mousedown: right (%d, %d)\n", x, y);
            break;
    }
}

void onMouseUp(unsigned short button, int x, int y, glow *gl) {
    switch (button) {
        case GLOW_MOUSE_BUTTON_LEFT:
            printf("mouseup: left (%d, %d)\n", x, y);
            break;
        case GLOW_MOUSE_BUTTON_RIGHT:
            printf("mouseup: right (%d, %d)\n", x, y);
            break;
    }
}

void onMouseMove(int x, int y, glow *gl) {
    //printf("mousemove: (%d, %d)\n", x, y);
}

void onScrollWheel(int dx, int dy, int x, int y, glow *gl) {
    printf("scroll wheel: %d %d (%d, %d)\n", dx, dy, x, y);
}

void onKeyDown(unsigned short key, int x, int y, glow *gl) {
    printf("keydown: %u [%c] (%d, %d)\n", key, (unsigned char)key, x, y);
}

void onKeyUp(unsigned short key, int x, int y, glow *gl) {
	printf("keyup: %u [%c] (%d, %d)\n", key, (unsigned char)key, x, y);
}

