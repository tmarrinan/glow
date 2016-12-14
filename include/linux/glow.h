#ifndef __GLOW_LINUX_H__
#define __GLOW_LINUX_H__

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XInput2.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glowEvents.h"

#ifndef Button6
#define Button6 6
#endif
#ifndef Button7
#define Button7 7
#endif

#define GLOW_MAX_TIMERS 128
#define GLOW_MAX_WINDOWS 32

#define GLOW_OPENGL_LEGACY 0
#define GLOW_OPENGL_CORE 1
#define GLOW_FLAGS_NONE 0
#define GLOW_WINDOW_BASE 0
#define GLOW_WINDOW_HIDPI 1
#define GLOW_WINDOW_BORDERLESS 2
#define GLOW_CENTER_HORIZONTAL INT_MAX
#define GLOW_CENTER_VERTICAL INT_MAX

class glow;

typedef struct GLOW_TimerData {
	glow *glow_ptr;
	unsigned int timer_id;
} GLOW_TimerData;

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

typedef void (*timer_func)(glow *gl, unsigned int timeoutId, void *data);

extern PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;

class glow {
private:
	Display *display;
	std::vector<Window> windowList;
	std::vector<GLXContext> glCtxList;

	unsigned int glProfile;
	unsigned int glCoreVMajor;
	unsigned int glCoreVMinor;
	std::vector<int> prevX;
	std::vector<int> prevY;
	std::vector<int> prevW;
	std::vector<int> prevH;
	std::vector<bool> fullscreen;
	int mouseX;
    int mouseY;
	std::vector<long> startTime;
	std::vector<long> prevTime;
	unsigned int capsmask;
	std::vector<bool> requiresRender;
	std::vector<bool> initWindowPlacement;
	std::vector<bool> isIdle;
	std::vector<bool> winIsOpen;
	Atom stateMessage;
	Atom fullscreenMessage;
	Atom timeoutMessage;
	unsigned int timerId;
	timer_t timeoutTimers[GLOW_MAX_TIMERS];
	timer_func timeoutCallbacks[GLOW_MAX_TIMERS];
	void *timeoutData[GLOW_MAX_TIMERS];

	std::vector<void (*)(glow *gl, int wid, unsigned long t, unsigned int dt, void *data)> renderCallback;
	std::vector<void (*)(glow *gl, int wid, void *data)> idleCallback;
	std::vector<void (*)(glow *gl, int wid, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data)> resizeCallback;
	std::vector<void (*)(glow *gl, int wid, unsigned short button, int x, int y, void *data)> mouseDownCallback;
	std::vector<void (*)(glow *gl, int wid, unsigned short button, int x, int y, void *data)> mouseUpCallback;
	std::vector<void (*)(glow *gl, int wid, int x, int y, void *data)> mouseMoveCallback;
	std::vector<void (*)(glow *gl, int wid, int dx, int dy, int x, int y, void *data)> scrollWheelCallback;
	std::vector<void (*)(glow *gl, int wid, unsigned short key, int x, int y, void *data)> keyDownCallback;
	std::vector<void (*)(glow *gl, int wid, unsigned short key, int x, int y, void *data)> keyUpCallback;

	std::vector<void*> renderData;
	std::vector<void*> idleData;
	std::vector<void*> resizeData;
	std::vector<void*> mouseDownData;
	std::vector<void*> mouseUpData;
	std::vector<void*> mouseMoveData;
	std::vector<void*> scrollWheelData;
	std::vector<void*> keyDownData;
	std::vector<void*> keyUpData;

	FT_Library ft;

	uint32_t offsetsFromUTF8[4];

	static void timeoutTimerFired(union sigval arg);
	unsigned short specialKey(KeySym code);
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

#endif // __GLOW_LINUX_H__

