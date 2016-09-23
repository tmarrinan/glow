#ifndef __GLOW_MAC_H__
#define __GLOW_MAC_H__

#include <OpenGL/gl.h>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
typedef NSAutoreleasePool nsAutoreleasePool;
typedef NSWindow nsWindow;
typedef NSOpenGLContext nsOpenGLContext;
typedef NSOpenGLPixelFormatAttribute nsOpenGLPixelFormatAttribute;
#else
typedef void nsAutoreleasePool;
typedef void nsWindow;
typedef void nsOpenGLContext;
typedef uint32_t nsOpenGLPixelFormatAttribute;
#endif

#include <iostream>
#include <string>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glowEvents.h"

#define GLOW_OPENGL_LEGACY 0
#define GLOW_OPENGL_CORE 1
#define GLOW_BASE_WINDOW 0
#define GLOW_HIDPI_WINDOW 1
#define GLOW_BORDERLESS_WINDOW 2
#define GLOW_CENTER_HORIZONTAL INT_MAX
#define GLOW_CENTER_VERTICAL INT_MAX

typedef struct GLOW_FontFace {
	FT_Face face;
	unsigned int size;
} GLOW_FontFace;

typedef struct GLOW_CharGlyph {
	unsigned int width;
	unsigned int height;
	unsigned char *pixels;
	int left;
	int top;
	int advanceX;
} GLOW_CharGlyph;

class glow {
private:
	nsAutoreleasePool *pool;
	nsWindow *mainwindow;
	nsOpenGLContext *glContext;

	nsOpenGLPixelFormatAttribute glProfileAttrib;
	bool hiDPISupport;
	bool borderless;

	bool windowRequiresResize;
	int requestedWindowX;
	int requestedWindowY;
	unsigned int requestedWindowW;
	unsigned int requestedWindowH;

	FT_Library ft;

	uint32_t offsetsFromUTF8[4];

	void convertUTF8toUTF32 (unsigned char *source, uint16_t bytes, uint32_t* target);
	void getRenderedGlyphsFromString(GLOW_FontFace *face, std::string text, unsigned int *width, unsigned int *height, std::vector<GLOW_CharGlyph> *glyphs);

public:
	glow() {};
	~glow() {};
 
	void initialize(unsigned int profile, unsigned int vmajor, unsigned int vminor, unsigned int windowtype);
	void createWindow(std::string title, int x, int y, unsigned int width, unsigned int height);

	void renderFunction(void (*callback)(unsigned long t, unsigned int dt, glow *gl));
	void idleFunction(void (*callback)(glow *gl));
	void resizeFunction(void (*callback)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl));
	unsigned int setTimeout(void (*callback)(unsigned int timeoutId, glow *gl), unsigned int wait);
	void cancelTimeout(unsigned int timeoutId);

	void mouseDownListener(void (*callback)(unsigned short button, int x, int y, glow *gl));
	void mouseUpListener(void (*callback)(unsigned short button, int x, int y, glow *gl));
	void mouseMoveListener(void (*callback)(int x, int y, glow *gl));
	void scrollWheelListener(void (*callback)(int dx, int dy, int x, int y, glow *gl));
	void keyDownListener(void (*callback)(unsigned short key, int x, int y, glow *gl));
	void keyUpListener(void (*callback)(unsigned short key, int x, int y, glow *gl));

	void swapBuffers();
	void requestRenderFrame();
	void enableFullscreen();
	void disableFullscreen();
	void setWindowGeometry(int x, int y, unsigned int width, unsigned int height);
	void exitFullScreenFinished();
	void setWindowTitle(std::string title);
	void runLoop();

	void createFontFace(std::string fontfile, unsigned int size, GLOW_FontFace **facePtr);
	void destroyFontFace(GLOW_FontFace **facePtr);
	void setFontSize(GLOW_FontFace *face, unsigned int size);
	void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, bool flipY, unsigned int *width, unsigned int *height, unsigned char **pixels);
	void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, unsigned char color[3], bool flipY, unsigned int *width, unsigned int *height, unsigned char **pixels);

	void getGLVersions(std::string *glv, std::string *glslv);
};

#endif // __GLOW_H__
