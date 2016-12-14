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

#define GLOW_MAX_WINDOWS 32

#define GLOW_OPENGL_LEGACY 0
#define GLOW_OPENGL_CORE 1
#define GLOW_FLAGS_NONE 0
#define GLOW_FLAGS_HIDE_DOCK 1
#define GLOW_WINDOW_BASE 0
#define GLOW_WINDOW_HIDPI 1
#define GLOW_WINDOW_BORDERLESS 2
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
	std::vector<nsWindow*> windowList;
	std::vector<nsOpenGLContext*> glCtxList;
	std::vector<bool> winIsOpen;

	nsOpenGLPixelFormatAttribute glProfileAttrib;

	bool windowRequiresResize;
	int requestedWindowX;
	int requestedWindowY;
	unsigned int requestedWindowW;
	unsigned int requestedWindowH;

	FT_Library ft;

	uint32_t offsetsFromUTF8[4];

	void convertUTF8toUTF32 (unsigned char *source, uint16_t bytes, uint32_t* target);
	void getRenderedGlyphsFromString(GLOW_FontFace *face, std::string text, unsigned int *width, unsigned int *height, unsigned int *baseline, std::vector<GLOW_CharGlyph> *glyphs);

public:
	glow() {};
	~glow() {};
 
	void initialize(unsigned int profile, unsigned int vmajor, unsigned int vminor, unsigned int flags);
	int createWindow(std::string title, int x, int y, unsigned int width, unsigned int height, unsigned int windowtype);
	void setActiveWindow(int winId);

	void renderFunction(int winId, void (*callback)(glow *gl, int wid, unsigned long t, unsigned int dt, void *data), void *data);
	void idleFunction(int winId, void (*callback)(glow *gl, int wid, void *data), void *data);
	void resizeFunction(int winId, void (*callback)(glow *gl, int wid, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data), void *data);
	unsigned int setTimeout(void (*callback)(glow *gl, unsigned int timeoutId, void *data), unsigned int wait, void *data);
	void cancelTimeout(unsigned int timeoutId);

	void mouseDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data);
	void mouseUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data);
	void mouseMoveListener(int winId, void (*callback)(glow *gl, int wid, int x, int y, void *data), void *data);
	void scrollWheelListener(int winId, void (*callback)(glow *gl, int wid, int dx, int dy, int x, int y, void *data), void *data);
	void keyDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data);
	void keyUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data);

	void swapBuffers(int winId);
	bool requestRenderFrame(int winId);
	void enableFullscreen(int winId);
	void disableFullscreen(int winId);
	void windowClosed(nsWindow *window);
	void exitFullScreenFinished(int winId);
	void setWindowGeometry(int winId, int x, int y, unsigned int width, unsigned int height);
	void setWindowTitle(int winId, std::string title);
	void runLoop();

	void createFontFace(std::string fontfile, unsigned int size, GLOW_FontFace **facePtr);
	void destroyFontFace(GLOW_FontFace **facePtr);
	void setFontSize(GLOW_FontFace *face, unsigned int size);
	void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels);
	void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, unsigned char color[3], bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels);

	void getGLVersions(std::string *glv, std::string *glslv);
};

#endif // __GLOW_H__
