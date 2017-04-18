#include <iostream>
#include <algorithm>
#include <cmath>
#include <string>

#include "glow.h"


using namespace std;

typedef struct appData {
	string exePath;
	unsigned int textures[2];
	int textSizes[2][3];
} appData;

string getExePath(char* exe);
void init(glow *gl, int win, appData *data);
void display(glow *gl, int win, unsigned long t, unsigned int dt, void *data);
void idle(glow *gl, int win, void *data);
void resize(glow *gl, int win, unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, void *data);

int main (int argc, char **argv) {
	appData *data = new appData;
	data->exePath = getExePath(argv[0]);
	printf("exe path: %s\n",data->exePath.c_str());

	glow *gl = new glow();
	gl->initialize(GLOW_OPENGL_LEGACY, 0, 0, GLOW_FLAGS_NONE);
	int win = gl->createWindow("Text", GLOW_CENTER_HORIZONTAL, GLOW_CENTER_VERTICAL, 800, 640, GLOW_WINDOW_BASE);

	init(gl, win, data);

	gl->runLoop();

	return 0;
}

string getExePath(char* exe) {
	string path = exe;
	replace(path.begin(), path.end(), '\\', '/');
	int lsep = path.rfind('/');

	return path.substr(0, lsep+1);
}

void init(glow *gl, int win, appData *d) {
	gl->renderFunction(win, display, d);
	gl->idleFunction(win, idle, NULL);
	gl->resizeFunction(win, resize, NULL);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	string version, shadingVersion;
	gl->getGLVersions(&version, &shadingVersion);
	printf("Using OpenGL: %s, GLSL: %s\n", version.c_str(), shadingVersion.c_str());

	string text1 = "☺hello there, my name is 'text'. Nice to meet you!";
	string text2 = "\xe2\x98\xbaworld\u263a i am awesome. soooofreak'nawesome!";
	printf("%s %s\n", text1.c_str(), text2.c_str());

	GLOW_FontFace *face;
	unsigned char *textPx1, *textPx2;
	unsigned int textW1, textH1, textB1, textW2, textH2, textB2;
	unsigned char col[3] = {255, 126, 0};
	//gl->createFontFace(d->exePath + "../resrc/fonts/Arial.ttf", 96, &face);
	gl->createFontFace(d->exePath + "../resrc/fonts/Arial.ttf", 48, &face);
	//gl->renderStringToTexture(face, text1, false, &textW1, &textH1, &textB1, &textPx1);
	//gl->renderStringToTexture(face, text2, col, false, &textW2, &textH2, &textB2, &textPx2);

	gl->renderStringToTextureWithWrap(face, text1, 400, false, &textW1, &textH1, &textB1, &textPx1);
	gl->renderStringToTextureWithWrap(face, text2, 300, col, false, &textW2, &textH2, &textB2, &textPx2);

	glGenTextures(2, d->textures);
	glBindTexture(GL_TEXTURE_2D, d->textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, textW1, textH1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, textPx1);
	glBindTexture(GL_TEXTURE_2D, d->textures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textW2, textH2, 0, GL_RGBA, GL_UNSIGNED_BYTE, textPx2);
	glBindTexture(GL_TEXTURE_2D, 0);

	free(textPx1);
	free(textPx2);

	d->textSizes[0][0] = textW1;
	d->textSizes[0][1] = textH1;
	d->textSizes[0][2] = textB1;
	d->textSizes[1][0] = textW2;
	d->textSizes[1][1] = textH2;
	d->textSizes[1][2] = textB2;
	printf("Text texture sizes: %dx%d (%d), %dx%d (%d)\n", textW1, textH1, textB1, textW2, textH2, textB2);
}

void display(glow *gl, int win, unsigned long t, unsigned int dt, void *data) {
	appData *d = (appData*)data;
	//printf("render: t=%.3f, dt=%.3f\n", (float)t/1000.0, (float)dt/1000.0);

	float c = (float)(abs((int)(t % 4000) - 2000)) / 2000.0f;
	glClearColor(c, c, c, 1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	float tex1x1 = -1.0;
	float tex1x2 = -1.0 + (2.0 * (float)(d->textSizes[0][0]) / 800.0);
	float tex1y1 = 1.0 * (2.0 * (float)(d->textSizes[0][2]) / 640.0);
	float tex1y2 = tex1y1 - (2.0 * (float)(d->textSizes[0][1]) / 640.0);
	float tex2x1 =  1.0 - (2.0 * (float)(d->textSizes[1][0]) / 800.0);
	float tex2x2 =  1.0;
	float tex2y1 = 1.0 * (2.0 * (float)(d->textSizes[1][2]) / 640.0);
	float tex2y2 = tex2y1 - (2.0 * (float)(d->textSizes[1][1]) / 640.0);


	glBindTexture(GL_TEXTURE_2D, d->textures[0]);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(tex1x1, tex1y1);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(tex1x2, tex1y1);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(tex1x2, tex1y2);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(tex1x1, tex1y2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, d->textures[1]);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(tex2x1, tex2y1);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(tex2x2, tex2y1);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(tex2x2, tex2y2);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(tex2x1, tex2y2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);

	gl->swapBuffers(win);
}

void idle(glow *gl, int win, void *data) {
	gl->requestRenderFrame(win);
}

void resize(glow *gl, int win, unsigned int wW, unsigned int wH, unsigned int rW, unsigned int rH, void *data) {
	printf("window size: %ux%u (%ux%u)\n", wW, wH, rW, rH);
	glViewport(0, 0, rW, rH);
	gl->requestRenderFrame(win);
}
