#ifndef __GLOW_LINUX_H__
#define __GLOW_LINUX_H__

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glowEvents.h"

#define GLOW_OPENGL_LEGACY 0
#define GLOW_OPENGL_3_2_CORE 1
#define GLOW_BASE_WINDOW 0
#define GLOW_HIDPI_WINDOW 1
#define GLOW_CENTER_HORIZONTAL INT_MAX
#define GLOW_CENTER_VERTICAL INT_MAX

typedef struct GLOW_FontFace {
	FT_Face face;
	unsigned int size;
} GLOW_FontFace;

typedef struct charGlyph {
	unsigned int width;
	unsigned int height;
	unsigned char *pixels;
	int left;
	int top;
	int advanceX;
} charGlyph;

extern PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;

class glow {
private:
	Display *display;
	Window window;
	GLXContext ctx;
	unsigned int glProfile;
	unsigned int hiDPISupport;
	int prevX;
	int prevY;
	int prevW;
	int prevH;
	int mouseX;
    int mouseY;
	unsigned int capsmask;
	bool requiresRender;

	void (*renderCallback)(unsigned long t, unsigned int dt, glow *gl);
	void (*idleCallback)(glow *gl);
	void (*resizeCallback)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl);
	void (*keyDownCallback)(unsigned short key, int x, int y, glow *gl);
	void (*keyUpCallback)(unsigned short key, int x, int y, glow *gl);

	FT_Library ft;

	uint32_t offsetsFromUTF8[4];

	unsigned short specialKey(KeySym code);
	void convertUTF8toUTF32 (unsigned char *source, uint16_t bytes, uint32_t* target);
	void getRenderedGlyphsFromString(GLOW_FontFace *face, std::string text, unsigned int *width, unsigned int *height, std::vector<charGlyph> *glyphs);
public:
	glow() {};
	~glow() {};

	
	void createWindow(std::string title, int x, int y, unsigned int width, unsigned int height);
	void initialize(unsigned int profile, unsigned int hidpi);

	void renderFunction(void (*callback)(unsigned long t, unsigned int dt, glow *gl));
	void idleFunction(void (*callback)(glow *gl));
	void resizeFunction(void (*callback)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl));
	unsigned int setTiminiteout(void (*callback)(unsigned int timeoutId, glow *gl), unsigned int wait);
	void cancelTimeout(unsigned int timeoutId);

	void mouseDownListener(void (*callback)(unsigned short button, int x, int y, glow *gl));
	void mouseUpListener(void (*callback)(unsigned short button, int x, int y, glow *gl));
	void mouseMoveListener(void (*callback)(int x, int y, glow *gl));
	void scrollWheelListener(void (*callback)(int dx, int dy, int x, int y, glow *gl));
	void keyDownListener(void (*callback)(unsigned short key, int x, int y, glow *gl));
	void keyUpListener(void (*callback)(unsigned short key, int x, int y, glow *gl));

	void swapBuffers();
	void requestRenderFrame();
	void runLoop();

	void createFontFace(std::string fontfile, unsigned int size, GLOW_FontFace **facePtr);
	void destroyFontFace(GLOW_FontFace **facePtr);
	void setFontSize(GLOW_FontFace *face, unsigned int size);
	void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, bool flipY, unsigned int *width, unsigned int *height, unsigned char **pixels);
	void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, unsigned char color[3], bool flipY, unsigned int *width, unsigned int *height, unsigned char **pixels);

	void getGLVersions(std::string *glv, std::string *glslv);	
};

#endif // __GLOW_LINUX_H__

