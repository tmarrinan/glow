#ifndef __GLOW_WINDOWS_H__
#define __GLOW_WINDOWS_H__

#ifdef GLOW_EXPORTS
#define GLOW_API __declspec(dllexport)
#else
#define GLOW_API __declspec(dllimport)
#endif

#include <windows.h>
#include <epoxy/gl.h>
#include <epoxy/wgl.h>

#include <iostream>
#include <string>
#include <vector>
#include <inttypes.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "glowEvents.h"

#ifndef IDT_TIMER1
#define IDT_TIMER1 1001
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

typedef void(*timer_func)(glow *gl, int wid, int timeoutId, void *data);


class glow {
private:
	std::vector<HDC> displayList;
	std::vector<HWND> windowList;
	std::vector<HGLRC> glCtxList;

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
	std::vector<ULONGLONG> startTime;
	std::vector<ULONGLONG> prevTime;
	unsigned int windowCount;
	unsigned int capsmask;
	std::vector<bool> requiresRender;
	std::vector<bool> isIdle;
	std::vector<bool> winIsOpen;
	unsigned int timerId;
	UINT_PTR idleTimerId;
	WPARAM IDLE_MESSAGE;
	WPARAM TIMER_MESSAGE;
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

	static VOID CALLBACK timeoutTimerFired(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);
	static VOID CALLBACK idleTimerFired(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);
	static VOID CALLBACK inactiveTimerFired(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);
	static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static BOOL ctrlHandler(DWORD fdwCtrlType);
	static unsigned short translateKey(WPARAM vk, LPARAM lParam);
	static unsigned short specialKey(WPARAM vk, LPARAM lParam);
	int windowCreated(HWND mainwindow);
	void convertUTF8toUTF32 (unsigned char *source, uint16_t bytes, uint32_t* target);
	void getRenderedGlyphsFromString(GLOW_FontFace *face, std::string text, unsigned int *width, unsigned int *height, unsigned int *baseline, std::vector<GLOW_CharGlyph> *glyphs);

public:
	GLOW_API glow() {};
	GLOW_API ~glow() {};

	
	GLOW_API void initialize(unsigned int profile, unsigned int vmajor, unsigned int vminor, unsigned int flags);
	GLOW_API int createWindow(std::string title, int x, int y, unsigned int width, unsigned int height, unsigned int windowtype);
	GLOW_API void setActiveWindow(int winId);

	GLOW_API void renderFunction(int winId, void (*callback)(glow *gl, int wid, unsigned long t, unsigned int dt, void *data), void *data);
	GLOW_API void idleFunction(int winId, void (*callback)(glow *gl, int wid, void *data), void *data);
	GLOW_API void resizeFunction(int winId, void (*callback)(glow *gl, int wid, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data), void *data);
	GLOW_API int setTimeout(int winId, void (*callback)(glow *gl, int wid, int timeoutId, void *data), unsigned int wait, void *data);
	GLOW_API void cancelTimeout(int winId, int timeoutId);

	GLOW_API void mouseDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data);
	GLOW_API void mouseUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data);
	GLOW_API void mouseMoveListener(int winId, void (*callback)(glow *gl, int wid, int x, int y, void *data), void *data);
	GLOW_API void scrollWheelListener(int winId, void (*callback)(glow *gl, int wid, int dx, int dy, int x, int y, void *data), void *data);
	GLOW_API void keyDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data);
	GLOW_API void keyUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data);

	GLOW_API void swapBuffers(int winId);
	GLOW_API bool requestRenderFrame(int winId);
	GLOW_API void enableFullscreen(int winId);
	GLOW_API void disableFullscreen(int winId);
	GLOW_API void setWindowGeometry(int winId, int x, int y, unsigned int width, unsigned int height);
	GLOW_API void setWindowTitle(int winId, std::string title);
	GLOW_API void runLoop();

	GLOW_API void createFontFace(std::string fontfile, unsigned int size, GLOW_FontFace **facePtr);
	GLOW_API void destroyFontFace(GLOW_FontFace **facePtr);
	GLOW_API void setFontSize(GLOW_FontFace *face, unsigned int size);
	GLOW_API void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels);
	GLOW_API void renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, unsigned char color[3], bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels);

	GLOW_API void getGLVersions(std::string *glv, std::string *glslv);	
};

#endif // __GLOW_WINDOWS_H__

