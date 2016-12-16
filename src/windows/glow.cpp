#include "glow.h"

PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;
PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = NULL;

// GLOW C++ Interface
void glow::initialize(unsigned int profile, unsigned int vmajor, unsigned int vminor, unsigned int flags) {
	glProfile = profile;
	glCoreVMajor = vmajor;
	glCoreVMinor = vminor;

	mouseX = 0;
	mouseY = 0;

	timerId = 0;
	
	idleTimerId = GLOW_MAX_TIMERS + 1;
	
	IDLE_MESSAGE = 1;
	TIMER_MESSAGE = 2;
	
	windowCount = 0;
	
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlHandler, TRUE)) {
		fprintf(stderr, "Warning: could not set console control handler\n");
	}

	// initialize freetype text rendering
	if(FT_Init_FreeType(&ft)) fprintf(stderr, "Error: could not init freetype library\n");
	offsetsFromUTF8[0] = 0x00000000UL;
	offsetsFromUTF8[1] = 0x00003080UL;
	offsetsFromUTF8[2] = 0x000E2080UL;
	offsetsFromUTF8[3] = 0x03C82080UL;
}

int glow::createWindow(std::string title, int x, int y, unsigned int width, unsigned int height, unsigned int windowtype) {	
	if (windowList.size() >= GLOW_MAX_WINDOWS)
		return -1;
	
	wchar_t *glClass = L"GLClass";
	std::wstring wtitle = std::wstring(title.begin(), title.end());
	HINSTANCE hinst = GetModuleHandle(NULL);

	int wid = windowList.size();
	bool hiDPISupport = windowtype & GLOW_WINDOW_HIDPI;
	bool borderless = windowtype & GLOW_WINDOW_BORDERLESS;
	
	WNDCLASSEX ex;
	ex.cbSize = sizeof(WNDCLASSEX);
	ex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	ex.lpfnWndProc = wndProc;
	ex.cbClsExtra = 0;
	ex.cbWndExtra = 0;
	ex.hInstance = hinst;
	ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ex.hCursor = LoadCursor(NULL, IDC_ARROW);
	ex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);//NULL;
	ex.lpszMenuName = NULL;
	ex.lpszClassName = glClass;
	ex.hIconSm = NULL;
	RegisterClassEx(&ex);

	if (x == GLOW_CENTER_HORIZONTAL) x = (GetSystemMetrics(SM_CXSCREEN) / 2) - (width / 2);
	if (y == GLOW_CENTER_VERTICAL) y = (GetSystemMetrics(SM_CYSCREEN) / 2) - (height / 2);
	prevX.push_back(x);
	prevY.push_back(y);
	prevW.push_back(width);
	prevH.push_back(height);
	fullscreen.push_back(false);
	requiresRender.push_back(false);
	isIdle.push_back(false);
	winIsOpen.push_back(false);
	startTime.push_back(0);
	prevTime.push_back(0);
	
	HWND mainwindow;
	RECT wRect;
	wRect.left = x;
	wRect.top = y;
	wRect.right = x+width;
	wRect.bottom = y+height;
	if (borderless) {
		AdjustWindowRectEx(&wRect, WS_POPUP, false, WS_EX_APPWINDOW);
		mainwindow = CreateWindowEx(WS_EX_APPWINDOW, glClass, wtitle.c_str(), WS_POPUP, wRect.left, wRect.top, wRect.right-wRect.left, wRect.bottom-wRect.top, NULL, NULL, hinst, this);
	}
	else {
		AdjustWindowRectEx(&wRect, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, false, NULL);
		mainwindow = CreateWindowEx(NULL, glClass, wtitle.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, wRect.left, wRect.top, wRect.right-wRect.left, wRect.bottom-wRect.top, NULL, NULL, hinst, this);
	}
	
	return wid;
}

	
int glow::windowCreated(HWND mainwindow) {
	HDC display = GetDC(mainwindow);
	if (display == NULL) {
		fprintf(stderr, "ERROR getting window device context\n");
		return -1;
	}

	int indexPixelFormat = 0;
	unsigned int numPixelFormats;
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	
	indexPixelFormat = ChoosePixelFormat(display, &pfd);
	SetPixelFormat(display, indexPixelFormat, &pfd);

	HGLRC glContext = wglCreateContext(display);
	if (glContext == NULL) {
		fprintf(stderr, "ERROR creating OpenGL rendering context\n");
		return -1;
	}
	if (wglMakeCurrent(display, glContext) == 0) {
		fprintf(stderr, "ERROR making OpenGL rendering context current\n");
		return -1;
	}

	if (glProfile == GLOW_OPENGL_CORE) {
		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

		if (wglGetExtensionsStringARB == NULL) {
			fprintf(stderr, "ERROR OpenGL 3.0+ not supported on this system\n");
			return -1;
		}
		std::string ext = wglGetExtensionsStringARB(display);
		if (ext.find("WGL_ARB_create_context") == std::string::npos) {
			fprintf(stderr, "ERROR OpenGL 3.0+ not supported on this system\n");
			return -1;
		}

		wglMakeCurrent(display, NULL);
		wglDeleteContext(glContext);

		const int iPixelFormatAttribList[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0
		};
		int iContextAttribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, glCoreVMajor,
			WGL_CONTEXT_MINOR_VERSION_ARB, glCoreVMinor,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		wglChoosePixelFormatARB(display, iPixelFormatAttribList, NULL,1, &indexPixelFormat, &numPixelFormats);
		SetPixelFormat(display, indexPixelFormat, &pfd);

		glContext = wglCreateContextAttribsARB(display, 0, iContextAttribs);
		if (glContext == NULL) {
			fprintf(stderr, "ERROR creating OpenGL rendering context\n");
			return -1;
		}
		if (wglMakeCurrent(display, glContext) == 0) {
			fprintf(stderr, "ERROR making OpenGL rendering context current\n");
			return -1;
		}
	}
	
	displayList.push_back(display);
	glCtxList.push_back(glContext);

	renderCallback.push_back(NULL);
	idleCallback.push_back(NULL);
	resizeCallback.push_back(NULL);
	mouseDownCallback.push_back(NULL);
	mouseUpCallback.push_back(NULL);
	mouseMoveCallback.push_back(NULL);
	scrollWheelCallback.push_back(NULL);
	keyDownCallback.push_back(NULL);
	keyUpCallback.push_back(NULL);

	renderData.push_back(NULL);
	idleData.push_back(NULL);
	resizeData.push_back(NULL);
	mouseDownData.push_back(NULL);
	mouseUpData.push_back(NULL);
	mouseMoveData.push_back(NULL);
	scrollWheelData.push_back(NULL);
	keyDownData.push_back(NULL);
	keyUpData.push_back(NULL);
	
	int wid = windowList.size();
	requiresRender[wid] = true;
	startTime[wid] = GetTickCount64();
	prevTime[wid] = startTime[wid];
	isIdle[wid] = true;
	winIsOpen[wid] = true;
	windowList.push_back(mainwindow);
	
	ShowWindow(mainwindow, SW_SHOWNORMAL);
	UpdateWindow(mainwindow);
	
	windowCount++;
	
	return 0;
}

void glow::setActiveWindow(int winId) {
	wglMakeCurrent(displayList[winId], glCtxList[winId]);
}

void glow::renderFunction(int winId, void (*callback)(glow *gl, int wid, unsigned long t, unsigned int dt, void *data), void *data) {
	renderCallback[winId] = callback;
	renderData[winId] = data;
}

void glow::idleFunction(int winId, void (*callback)(glow *gl, int wid, void *data), void *data) {
	idleCallback[winId] = callback;
	idleData[winId] = data;
}

void glow::resizeFunction(int winId, void (*callback)(glow *gl, int wid, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data), void *data) {
	resizeCallback[winId] = callback;
	resizeData[winId] = data;
}

int glow::setTimeout(int winId, void (*callback)(glow *gl, int wid, int timeoutId, void *data), unsigned int wait, void *data) {
	if (!winIsOpen[winId])
		return -1;
	
	int tId = timerId;

	timeoutCallbacks[tId] = callback;
	timeoutData[tId] = data;
	SetTimer(windowList[winId], tId+IDT_TIMER1, wait, (TIMERPROC)timeoutTimerFired);
	timerId = (timerId + 1) % GLOW_MAX_TIMERS;
	
	return tId;
}

void glow::cancelTimeout(int winId, int timeoutId) {
	if (winIsOpen[winId])
		KillTimer(windowList[winId], timeoutId+IDT_TIMER1);
}

VOID CALLBACK glow::timeoutTimerFired(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime) {
	glow *self = (glow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	KillTimer(hwnd, idTimer);
	PostMessage(hwnd, WM_USER, self->TIMER_MESSAGE, idTimer-IDT_TIMER1);
}

void glow::mouseDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data) {
	mouseDownCallback[winId] = callback;
	mouseDownData[winId] = data;
}

void glow::mouseUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data) {
	mouseUpCallback[winId] = callback;
	mouseDownData[winId] = data;
}

void glow::mouseMoveListener(int winId, void (*callback)(glow *gl, int wid, int x, int y, void *data), void *data) {
	mouseMoveCallback[winId] = callback;
	mouseMoveData[winId] = data;
}

void glow::scrollWheelListener(int winId, void (*callback)(glow *gl, int wid, int dx, int dy, int x, int y, void *data), void *data) {
	scrollWheelCallback[winId] = callback;
	scrollWheelData[winId] = data;
}

void glow::keyDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data) {
	keyDownCallback[winId] = callback;
	keyDownData[winId] = data;
}

void glow::keyUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data) {
	keyUpCallback[winId] = callback;
	keyUpData[winId] = data;
}

void glow::swapBuffers(int winId) {
	SwapBuffers(displayList[winId]);
}

bool glow::requestRenderFrame(int winId) {
	if (winIsOpen[winId]) {
		requiresRender[winId] = true;
		return true;
	}
	return false;
}

void glow::enableFullscreen(int winId) {
	if (fullscreen[winId]) return;

	fullscreen[winId] = true;
	RECT rect;
	GetWindowRect(windowList[winId], &rect);
	prevX[winId] = rect.left;
	prevY[winId] = rect.top;
	prevW[winId] = rect.right - prevX[winId];
	prevH[winId] = rect.bottom - prevY[winId];
	SetWindowLongPtr(windowList[winId], GWL_STYLE, WS_POPUP | WS_VISIBLE);
	SetWindowPos(windowList[winId], HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
}

void glow::disableFullscreen(int winId) {
	if (!fullscreen[winId]) return;

	fullscreen[winId] = false;
	SetWindowLongPtr(windowList[winId], GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	SetWindowPos(windowList[winId], HWND_TOP, prevX[winId], prevY[winId], prevW[winId], prevH[winId], SWP_SHOWWINDOW);
}

void glow::setWindowGeometry(int winId, int x, int y, unsigned int width, unsigned int height) {
	fullscreen[winId] = false;
	if (x == GLOW_CENTER_HORIZONTAL) x = (GetSystemMetrics(SM_CXSCREEN) / 2) - (width / 2);
	if (y == GLOW_CENTER_VERTICAL) y = (GetSystemMetrics(SM_CYSCREEN) / 2) - (height / 2);
	
	RECT wRect;
	wRect.left = x;
	wRect.top = y;
	wRect.right = x+width;
	wRect.bottom = y+height;
	AdjustWindowRectEx(&wRect, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, false, NULL);
	SetWindowLongPtr(windowList[winId], GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	SetWindowPos(windowList[winId], HWND_TOP, wRect.left, wRect.top, wRect.right-wRect.left, wRect.bottom-wRect.top, SWP_SHOWWINDOW);
}

void glow::setWindowTitle(int winId, std::string title) {
	std::wstring wtitle = std::wstring(title.begin(), title.end());
	SetWindowText(windowList[winId], wtitle.c_str());
}

void glow::runLoop() {
	MSG message;
	int i;
	bool running = true;

	while (GetMessage(&message, NULL, 0, 0) > 0) {
		do {
			if (message.message == WM_QUIT) {
				running = false;
				break;
			}
			TranslateMessage(&message);
			DispatchMessage(&message);
		} while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE));

		if (!running) break;

		for (i=0; i<windowList.size(); i++) {
			if (isIdle[i] && idleCallback[i]) {
				isIdle[i] = false;
				SetTimer(windowList[i], idleTimerId+IDT_TIMER1, USER_TIMER_MINIMUM, (TIMERPROC)idleTimerFired);
			}
			if (requiresRender[i] && renderCallback[i]) {
				ULONGLONG now = GetTickCount64();
				wglMakeCurrent(displayList[i], glCtxList[i]);
				renderCallback[i](this, i, (unsigned long)(now - startTime[i]), (unsigned int)(now - prevTime[i]), renderData[i]);
				prevTime[i] = now;
				requiresRender[i] = false;
			}
		}
	}
}

VOID CALLBACK glow::idleTimerFired(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime) {
	glow *self = (glow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	PostMessage(hwnd, WM_USER, self->IDLE_MESSAGE, 0);
}

LRESULT CALLBACK glow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	glow *self = (glow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	unsigned short key;
	short rw, rh, scroll;
	
	int i;
	int winId = -1;
	if (msg != WM_CREATE && self != NULL) {
		for (i=0; i<self->windowList.size(); i++) {
			if (hwnd == self->windowList[i]) winId = i;
		}	
		if (winId < 0) return 0;
	}

	switch (msg) {
		case WM_CREATE:
			self = (glow*)(((CREATESTRUCT*)lParam)->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (long)self);
			self->windowCreated(hwnd);
			break;
		case WM_CLOSE:
			wglDeleteContext(self->glCtxList[winId]);
			DestroyWindow(self->windowList[winId]);
			break;
		case WM_DESTROY:
			if (self->windowCount == 1)
				PostQuitMessage(0);
			self->glCtxList[winId] = NULL;
			self->windowList[winId] = NULL;
			self->windowCount--;
			break;
		case WM_QUERYENDSESSION:
			printf("user ending session\n");
			break;
		case WM_SIZE:
			rw = (short)LOWORD(lParam);
			rh = (short)HIWORD(lParam);
			if (!self->resizeCallback[winId]) break;

			self->resizeCallback[winId](self, winId, rw, rh, rw, rh, self->resizeData[winId]);
			break;
		case WM_MOVE:
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (!self->keyDownCallback[winId] && !self->keyUpCallback[winId]) break;

			key = translateKey(wParam, lParam);
			if (key == GLOW_KEY_CAPS_LOCK && !(GetKeyState(VK_CAPITAL) & 0x0001)) {
				if (self->keyUpCallback[winId]) self->keyUpCallback[winId](self, winId, key, self->mouseX, self->mouseY, self->keyUpData[winId]);
			}
			else {
				if (self->keyDownCallback[winId]) self->keyDownCallback[winId](self, winId, key, self->mouseX, self->mouseY, self->keyDownData[winId]);
			}
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (!self->keyUpCallback[winId]) break;

			key = translateKey(wParam, lParam);
			if (key != GLOW_KEY_CAPS_LOCK)
				self->keyUpCallback[winId](self, winId, key, self->mouseX, self->mouseY, self->keyUpData[winId]);
			break;
		case WM_LBUTTONDOWN:
			if (!self->mouseDownCallback[winId]) break;

			self->mouseDownCallback[winId](self, winId, GLOW_MOUSE_BUTTON_LEFT, self->mouseX, self->mouseY, self->mouseDownData[winId]);
			break;
		case WM_RBUTTONDOWN:
			if (!self->mouseDownCallback[winId]) break;

			self->mouseDownCallback[winId](self, winId, GLOW_MOUSE_BUTTON_RIGHT, self->mouseX, self->mouseY, self->mouseDownData[winId]);
			break;
		case WM_LBUTTONUP:
			if (!self->mouseUpCallback[winId]) break;

			self->mouseUpCallback[winId](self, winId, GLOW_MOUSE_BUTTON_LEFT, self->mouseX, self->mouseY, self->mouseUpData[winId]);
			break;
		case WM_RBUTTONUP:
			if (!self->mouseUpCallback[winId]) break;

			self->mouseUpCallback[winId](self, winId, GLOW_MOUSE_BUTTON_RIGHT, self->mouseX, self->mouseY, self->mouseUpData[winId]);
			break;
		case WM_MOUSEWHEEL:
			if (!self->scrollWheelCallback[winId]) break;

			scroll = (short)HIWORD(wParam);
			self->scrollWheelCallback[winId](self, winId, 0, scroll, self->mouseX, self->mouseY, self->scrollWheelData[winId]);
			break;
		case WM_MOUSEHWHEEL:
			if (!self->scrollWheelCallback[winId]) break;

			scroll = (short)HIWORD(wParam);
			self->scrollWheelCallback[winId](self, winId, scroll, 0, self->mouseX, self->mouseY, self->scrollWheelData[winId]);
			break;
		case WM_MOUSEMOVE:
			self->mouseX = (short)LOWORD(lParam);
			self->mouseY = (short)HIWORD(lParam);

			if (!self->mouseMoveCallback[winId]) break;

			self->mouseMoveCallback[winId](self, winId, self->mouseX, self->mouseY, self->mouseMoveData[winId]);
			break;
		case WM_USER:
			if (wParam == self->IDLE_MESSAGE) {
				self->idleCallback[winId](self, winId, self->idleData[winId]);
				self->isIdle[winId] = true;
			}
			else if (wParam == self->TIMER_MESSAGE) {
				self->timeoutCallbacks[lParam](self, winId, lParam, self->timeoutData[lParam]);
			}
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

BOOL glow::ctrlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
		case CTRL_C_EVENT:
			PostQuitMessage(0);
			return TRUE;
		case CTRL_CLOSE_EVENT:
			PostQuitMessage(0);
			return TRUE;
		default:
			return FALSE;
	}
}

unsigned short glow::translateKey(WPARAM vk, LPARAM lParam) {
	unsigned short key;
	bool capslock;
	bool shift;
	
	switch (vk) {
		case VK_BACK:
			key = 8;
			break;
		case VK_TAB:
			key = 9;
			break;
		case VK_RETURN:
			key = 13;
			break;
		case VK_ESCAPE:
			key = 27;
			break;
		case VK_SPACE:
			key = 32;
			break;
		case VK_DELETE:
			key = 127;
			break;
		case 0x30:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 41;
			else                                key = vk;
			break;
		case 0x31:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 33;
			else                                key = vk;
			break;
		case 0x32:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 64;
			else                                key = vk;
			break;
		case 0x33:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 35;
			else                                key = vk;
			break;
		case 0x34:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 36;
			else                                key = vk;
			break;
		case 0x35:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 37;
			else                                key = vk;
			break;
		case 0x36:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 94;
			else                                key = vk;
			break;
		case 0x37:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 38;
			else                                key = vk;
			break;
		case 0x38:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 42;
			else                                key = vk;
			break;
		case 0x39:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 40;
			else                                key = vk;
			break;
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F:
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:
		case 0x58:
		case 0x59:
		case 0x5A:
			if (GetKeyState(VK_CAPITAL) & 0x0001) capslock = true;
			else                                  capslock = false;
			if (GetKeyState(VK_SHIFT) & 0x8000)   shift = true;
			else                                  shift = false;
			if (capslock ^ shift) // xor
				key = (unsigned short)vk;
			else
				key = (unsigned short)vk + 32;
			break;
		case VK_OEM_PLUS:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 43;
			else                                key = 61;
			break;
		case VK_OEM_COMMA:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 60;
			else                                key = 44;
			break;
		case VK_OEM_MINUS:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 95;
			else                                key = 45;
			break;
		case VK_OEM_PERIOD:
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 62;
			else                                key = 46;
			break;
		case VK_OEM_1: // ';', ':'
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 58;
			else                                key = 59;
			break;
		case VK_OEM_2: // '/', '?'
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 63;
			else                                key = 47;
			break;
		case VK_OEM_3: // '`', '~'
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 126;
			else                                key = 96;
			break;
		case VK_OEM_4: // '[', '{'
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 123;
			else                                key = 91;
			break;
		case VK_OEM_5: // '\', '|'
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 124;
			else                                key = 92;
			break;
		case VK_OEM_6: // ']', '}'
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 125;
			else                                key = 93;
			break;
		case VK_OEM_7: // ''', '"'
			if (GetKeyState(VK_SHIFT) & 0x8000) key = 34;
			else                                key = 39;
			break;
		default:
			key = specialKey(vk, lParam);
			break;
	}
	return key;
}

unsigned short glow::specialKey(WPARAM vk, LPARAM lParam) {
	unsigned short key;

	WPARAM newVk;
	UINT scancode = (lParam & 0x00ff0000) >> 16;
	int extended = (lParam & 0x01000000) != 0;
	switch (vk) {
		case VK_SHIFT:
			newVk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
			break;
		case VK_CONTROL:
			newVk = extended ? VK_RCONTROL : VK_LCONTROL;
			break;
		case VK_MENU:
			newVk = extended ? VK_RMENU : VK_LMENU;
			break;
		default:
			newVk = vk;
			break;
	}
    
	switch (newVk) {
        case VK_LSHIFT:
            key = GLOW_KEY_LEFT_SHIFT;
            break;
		case VK_RSHIFT:
            key = GLOW_KEY_RIGHT_SHIFT;
            break;
		case VK_LCONTROL:
            key = GLOW_KEY_LEFT_CONTROL;
            break;
		case VK_RCONTROL:
            key = GLOW_KEY_RIGHT_CONTROL;
            break;
		case VK_LMENU:
            key = GLOW_KEY_LEFT_ALT;
            break;
		case VK_RMENU:
            key = GLOW_KEY_RIGHT_ALT;
            break;
		case VK_LWIN:
            key = GLOW_KEY_LEFT_COMMAND;
            break;
		case VK_RWIN:
            key = GLOW_KEY_RIGHT_COMMAND;
            break;
        case VK_CAPITAL:
            key = GLOW_KEY_CAPS_LOCK;
            break;
        case VK_F1:
            key = GLOW_KEY_F1;
            break;
        case VK_F2:
            key = GLOW_KEY_F2;
            break;
        case VK_F3:
            key = GLOW_KEY_F3;
            break;
        case VK_F4:
            key = GLOW_KEY_F4;
            break;
        case VK_F5:
            key = GLOW_KEY_F5;
            break;
        case VK_F6:
            key = GLOW_KEY_F6;
            break;
        case VK_F7:
            key = GLOW_KEY_F7;
            break;
        case VK_F8:
            key = GLOW_KEY_F8;
            break;
        case VK_F9:
            key = GLOW_KEY_F9;
            break;
        case VK_F10:
            key = GLOW_KEY_F10;
            break;
        case VK_F11:
            key = GLOW_KEY_F11;
            break;
        case VK_F12:
            key = GLOW_KEY_F12;
            break;
        case VK_LEFT:
            key = GLOW_KEY_LEFT_ARROW;
            break;
        case VK_RIGHT:
            key = GLOW_KEY_RIGHT_ARROW;
            break;
        case VK_DOWN:
            key = GLOW_KEY_DOWN_ARROW;
            break;
        case VK_UP:
            key = GLOW_KEY_UP_ARROW;
            break;
        default:
			printf("unknown: %lu\n", newVk);
            key = GLOW_KEY_NONE;
            break;
    }
	
    return key;
}

void glow::createFontFace(std::string fontfile, unsigned int size, GLOW_FontFace **facePtr) {
	*facePtr = (GLOW_FontFace*)malloc(sizeof(GLOW_FontFace));
	if(FT_New_Face(ft, fontfile.c_str(), 0, &((*facePtr)->face))) {
		fprintf(stderr, "Error: could not open font\n");
		return;
	}
	(*facePtr)->size = size;

	FT_Select_Charmap((*facePtr)->face, FT_ENCODING_UNICODE);
	FT_Set_Pixel_Sizes((*facePtr)->face, 0, (*facePtr)->size);
}

void glow::destroyFontFace(GLOW_FontFace **facePtr) {
	free(*facePtr);
}

void glow::setFontSize(GLOW_FontFace *face, unsigned int size) {
	face->size = size;
	FT_Set_Pixel_Sizes(face->face, 0, face->size);
}

void glow::convertUTF8toUTF32 (unsigned char *source, uint16_t bytes, uint32_t* target) {
	uint32_t ch = 0;

	switch (bytes) {
		case 4: ch += *source++; ch <<= 6;
		case 3: ch += *source++; ch <<= 6;
		case 2: ch += *source++; ch <<= 6;
		case 1: ch += *source++;
	}
	ch -= offsetsFromUTF8[bytes-1];

	*target = ch;
}

void glow::getRenderedGlyphsFromString(GLOW_FontFace *face, std::string text, unsigned int *width, unsigned int *height, unsigned int *baseline, std::vector<GLOW_CharGlyph> *glyphs) {
	int i = 0;
	int top = 0;
	int bottom = 0;

	*width = 0;
	unsigned char *unicode = (unsigned char*)text.c_str();
	do {
		uint32_t charcode = 0;
		if ((text[i] & 0x80) == 0) {          // 1 byte
			charcode = text[i];
			i += 1;
		}
		else if ((text[i] & 0xE0) == 0xC0) {  // 2 bytes
			convertUTF8toUTF32(unicode + i, 2, &charcode);
			i += 2;
		}
		else if ((text[i] & 0xF0) == 0xE0) {  // 3 bytes
			convertUTF8toUTF32(unicode + i, 3, &charcode);
			i += 3;
		}
		else if ((text[i] & 0xF8) == 0xF0) {  // 4 bytes
			convertUTF8toUTF32(unicode + i, 4, &charcode);
			i += 4;
		}
		else {
			i += 1;
			continue;
		}

		uint32_t glyph_index = FT_Get_Char_Index(face->face, charcode);
		if (FT_Load_Glyph(face->face, glyph_index, FT_LOAD_RENDER))
			continue;

		glyphs->push_back(GLOW_CharGlyph());
		int c = glyphs->size() - 1;
		(*glyphs)[c].width = face->face->glyph->bitmap.width;
		(*glyphs)[c].height = face->face->glyph->bitmap.rows;
		(*glyphs)[c].pixels = (unsigned char *)malloc((*glyphs)[c].width * (*glyphs)[c].height);
		memcpy((*glyphs)[c].pixels, face->face->glyph->bitmap.buffer, (*glyphs)[c].width * (*glyphs)[c].height);
		(*glyphs)[c].left = face->face->glyph->bitmap_left;
		(*glyphs)[c].top = face->face->glyph->bitmap_top;
		(*glyphs)[c].advanceX = face->face->glyph->advance.x;

		*width += (*glyphs)[c].advanceX / 64;
		if ((*glyphs)[c].top > top) {
			top = (*glyphs)[c].top;
			*baseline = top;
		}
		if (((int)((*glyphs)[c].top) - (int)((*glyphs)[c].height)) < bottom) {
			bottom = (int)((*glyphs)[c].top) - (int)((*glyphs)[c].height);
		}
	} while (i < text.length());

	*height = top - bottom;
}

void glow::renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels) {
	int i, j, k;
	std::vector<GLOW_CharGlyph> glyphs;

	getRenderedGlyphsFromString(face, utf8Text, width, height, baseline, &glyphs);

	*width = (*width) + (4 - ((*width) % 4));
	*height = (*height) + (4 - ((*height) % 4));

	int size = (*width) * (*height);
	*pixels = (unsigned char*)malloc(size * sizeof(unsigned char));
	memset(*pixels, 0, size);

	int x = 0;
	int pt, ix, iy, idx;
	for (i=0; i<glyphs.size(); i++) {
		for (j=0; j<glyphs[i].height; j++) {
			pt = (*baseline) - glyphs[i].top;
			iy = pt + j;
			if (flipY) iy = ((*height) - iy);
			for (k=0; k<glyphs[i].width; k++) {
				ix = glyphs[i].left + x + k;
				idx = (*width) * iy + ix;
				if (idx < 0 || idx >= size) continue;
				(*pixels)[idx] = glyphs[i].pixels[glyphs[i].width * j + k];
			}
		}
		x += glyphs[i].advanceX / 64;
		free(glyphs[i].pixels);
	}
}

void glow::renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, unsigned char color[3], bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels) {
	int i, j, k;
	std::vector<GLOW_CharGlyph> glyphs;

	getRenderedGlyphsFromString(face, utf8Text, width, height, baseline, &glyphs);

	*width = (*width) + (4 - ((*width) % 4));
	*height = (*height) + (4 - ((*height) % 4));
	unsigned int bline = *baseline;

	int size = (*width) * (*height) * 4;
	*pixels = (unsigned char*)malloc(size * sizeof(unsigned char));
	memset(*pixels, 0, size);

	int x = 0;
	int pt, ix, iy, idx;
	for (i=0; i<glyphs.size(); i++) {
		for (j=0; j<glyphs[i].height; j++) {
			pt = (*baseline) - glyphs[i].top;
			iy = pt + j;
			if (flipY) iy = ((*height) - iy);
			for (k=0; k<glyphs[i].width; k++) {
				ix = glyphs[i].left + x + k;
				idx = (4 * (*width) * iy) + (4 * ix);
				if (idx < 0 || idx >= size) continue;
				(*pixels)[idx + 0] = color[0];
				(*pixels)[idx + 1] = color[1];
				(*pixels)[idx + 2] = color[2];
				(*pixels)[idx + 3] = glyphs[i].pixels[glyphs[i].width * j + k];
			}
		}
		x += glyphs[i].advanceX / 64;
		free(glyphs[i].pixels);
	}
}

void glow::getGLVersions(std::string *glv, std::string *glslv) {
	std::string *v[2] = {glv, glslv};
	GLenum type[2] = { GL_VERSION, GL_SHADING_LANGUAGE_VERSION };

    unsigned int i, j;
	for (i = 0; i < 2; i++) {
		std::string version = (const char*)glGetString(type[i]);
		int end = version.length();
		int dot = 0;
		for (j=0; j<version.length(); j++){
			if (version[j] == '.') {
				dot++;
				if (dot == 1) continue;
			}
			if (version[j] < 48 || version[j] > 57) {
				end = j;
				break;
			}
		}
		*(v[i]) = version.substr(0, end);
	}
}

