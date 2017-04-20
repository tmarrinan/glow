#include "glow.h"


// GLOW C++ Interface
void glow::initialize(unsigned int profile, unsigned int vmajor, unsigned int vminor, unsigned int flags) {
	glProfile = profile;
	glCoreVMajor = vmajor;
	glCoreVMinor = vminor;

	mouseX = 0;
	mouseY = 0;

	timerId = 0;

	if (XInitThreads() == 0) {
		fprintf(stderr, "Failed to initialize X\n");
	}
	
	display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "Failed to open X display\n");
		exit(1);
	}

	int glxMajor, glxMinor;
	if (!glXQueryVersion(display, &glxMajor, &glxMinor) || (glxMajor == 1 && glxMinor < 3) || glxMajor < 1) {
		fprintf(stderr, "Requires GLX >= 1.3, only found %d.%d\n", glxMajor, glxMinor);
		exit(1);
	}

	stateMessage = XInternAtom(display, "_NET_WM_STATE", False);
	fullscreenMessage = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
	timeoutMessage = XInternAtom(display, "TIMEOUT", False);

	// initialize freetype text rendering
	if(FT_Init_FreeType(&ft)) fprintf(stderr, "Error: could not init freetype library\n");
	offsetsFromUTF8[0] = 0x00000000UL;
	offsetsFromUTF8[1] = 0x00003080UL;
	offsetsFromUTF8[2] = 0x000E2080UL;
	offsetsFromUTF8[3] = 0x03C82080UL;

	shiftmask = XkbKeysymToModifiers(display, XK_Shift_L);
	ctrlmask = XkbKeysymToModifiers(display, XK_Control_L);
	altmask = XkbKeysymToModifiers(display, XK_Alt_L);
	cmdmask = XkbKeysymToModifiers(display, XK_Super_L);
	capsmask = XkbKeysymToModifiers(display, XK_Caps_Lock);
}

int glow::createWindow(std::string title, int x, int y, unsigned int width, unsigned int height, unsigned int windowType) {	
	if (windowList.size() >= GLOW_MAX_WINDOWS)
		return -1;
	
	int visualAttribs[] = {
		GLX_X_RENDERABLE,  True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_RED_SIZE,      8,
		GLX_GREEN_SIZE,    8,
		GLX_BLUE_SIZE,     8,
		GLX_ALPHA_SIZE,    8,
		GLX_DEPTH_SIZE,    24,
		GLX_STENCIL_SIZE,  8,
		GLX_DOUBLEBUFFER,  True,
		None
	};
	int fbcount;
	GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), visualAttribs, &fbcount);
	if (!fbc) {
		fprintf(stderr, "Failed to retreive a framebuffer config\n");
	}
	int bestFbcIdx = -1;
	int worstFbcIdx = -1;
	int bestNumSamp = -1;
	int worstNumSamp = 999;
	int i;
	for (i=0; i<fbcount; i++) {
		XVisualInfo *cvi = glXGetVisualFromFBConfig(display, fbc[i]);
		if (cvi) {
			int sampBufs, samples;
			glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &sampBufs);
			glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);
			if ((sampBufs && samples > bestNumSamp) || bestFbcIdx < 0) {
				bestFbcIdx = i;
				bestNumSamp = samples;
			}
			if ((!sampBufs || samples < worstNumSamp) || bestFbcIdx < 0) {
				worstFbcIdx = i;
				worstNumSamp = samples;
			}
		}
		XFree(cvi);
	}
	if (bestFbcIdx < 0) {
		fprintf(stderr, "Failed to find a suitable framebuffer configuration\n");
		return -1;
	}
	GLXFBConfig bestFbc = fbc[bestFbcIdx];
	XFree(fbc);
	XVisualInfo *vi = glXGetVisualFromFBConfig(display, bestFbc);

	XSetWindowAttributes swa;
	Colormap cmap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.background_pixmap = None;
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask;

	Window mainwindow = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, (CWBorderPixel | CWColormap | CWEventMask), &swa);
	if (!mainwindow) {
		fprintf(stderr, "Failed to create window\n");
		return -1;
	}
	XFree(vi);

	int wid = windowList.size();
	bool hiDPISupport = windowType & GLOW_WINDOW_HIDPI;
	bool borderless = windowType & GLOW_WINDOW_BORDERLESS;

	if (borderless) {
		Atom wmHints = XInternAtom(display, "_MOTIF_WM_HINTS", False);
		struct {
			unsigned long flags;
			unsigned long functions;
			unsigned long decorations;
			long input_mode;
			unsigned long status; 
		} hints;
		hints.flags = 2;
		hints.decorations = 0;
		XChangeProperty(display, mainwindow, wmHints, wmHints, 32, PropModeReplace, (unsigned char*) &hints, sizeof(hints) / sizeof(long));
	}
	
	XStoreName(display, mainwindow, title.c_str());

	GLXContext glContext = NULL;
	if (glProfile == GLOW_OPENGL_CORE) {
		if (!glXCreateContextAttribsARB) {
			fprintf(stderr, "Failed to obtain OpenGL 3.0+ context\n");
			return -1;
		}
		int glProfileAttribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB, glCoreVMajor,
			GLX_CONTEXT_MINOR_VERSION_ARB, glCoreVMinor,
			GLX_CONTEXT_FLAGS_ARB,         GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
			None
		};
		glContext = glXCreateContextAttribsARB(display, bestFbc, 0, True, glProfileAttribs);
	}
	else {
		glContext = glXCreateNewContext(display, bestFbc, GLX_RGBA_TYPE, 0, True);
	}
	XSync(display, False);
	if (!glXIsDirect(display, glContext)) {
		fprintf(stderr, "Warning: using indirect rendering context\n");
	}
	glXMakeCurrent(display, mainwindow, glContext);

	XMapWindow(display, mainwindow);

	Screen *screen = XDefaultScreenOfDisplay(display);
	int screenW = XWidthOfScreen(screen);
	int screenH = XHeightOfScreen(screen);

	if (x == GLOW_CENTER_HORIZONTAL) x = (screenW / 2) - (width / 2);
	if (y == GLOW_CENTER_VERTICAL) y = (screenH / 2) - (height / 2);
	prevX.push_back(x);
	prevY.push_back(y);
	prevW.push_back(width);
	prevH.push_back(height);
	fullscreen.push_back(false);
	requiresRender.push_back(false);
	initWindowPlacement.push_back(false);
	isIdle.push_back(false);
	winIsOpen.push_back(2); // 0 => closed, 1=> open, 2 => opening
	XMoveWindow(display, mainwindow, x, y);
	
	windowList.push_back(mainwindow);
	glCtxList.push_back(glContext);

	startTime.push_back(0);
	prevTime.push_back(0);

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

	return wid;
}

void glow::setActiveWindow(int winId) {
	glXMakeCurrent(display, windowList[winId], glCtxList[winId]);
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
	struct sigevent se;
	struct itimerspec ts;

	GLOW_TimerData *tdata = (GLOW_TimerData*)malloc(sizeof(GLOW_TimerData));
	tdata->glow_ptr = this;
	tdata->win_id = winId;
	tdata->timer_id = tId;

	se.sigev_notify = SIGEV_THREAD;
	se.sigev_value.sival_ptr = tdata;
	se.sigev_notify_function = timeoutTimerFired;
	se.sigev_notify_attributes = NULL;

	ts.it_value.tv_sec = wait / 1000;
	ts.it_value.tv_nsec = (unsigned long)(wait % 1000) * (unsigned long)1000000;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;

	timeoutCallbacks[tId] = callback;
	timeoutData[tId] = data;

	timer_create(CLOCK_MONOTONIC, &se, &timeoutTimers[tId]);
	timer_settime(timeoutTimers[tId], 0, &ts, NULL);
	
	timerId = (timerId + 1) % GLOW_MAX_TIMERS;
	return tId;
}

void glow::cancelTimeout(int winId, int timeoutId) {
	if (!winIsOpen[winId])
		return;

	timer_delete(timeoutTimers[timeoutId]);
}

void glow::timeoutTimerFired(union sigval arg) {
	GLOW_TimerData *data = (GLOW_TimerData*)arg.sival_ptr;
	if (data->glow_ptr->winIsOpen[data->win_id]) {
		XLockDisplay(data->glow_ptr->display);
	
		XClientMessageEvent timeoutEvent;
		timeoutEvent.type = ClientMessage;
		timeoutEvent.window = data->glow_ptr->windowList[data->win_id];
		timeoutEvent.message_type = data->glow_ptr->timeoutMessage;
		timeoutEvent.format = 32;
		timeoutEvent.data.l[0] = data->timer_id;
		XSendEvent(data->glow_ptr->display, data->glow_ptr->windowList[data->win_id], False, 0, (XEvent*)&timeoutEvent);
		XFlush(data->glow_ptr->display);

		XUnlockDisplay(data->glow_ptr->display);
	}

	timer_delete(data->glow_ptr->timeoutTimers[data->timer_id]);
	free(data);
}

void glow::mouseDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data) {
	mouseDownCallback[winId] = callback;
	mouseDownData[winId] = data;
}

void glow::mouseUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data) {
	mouseUpCallback[winId] = callback;
	mouseUpData[winId] = data;
}

void glow::mouseMoveListener(int winId, void (*callback)(glow *gl, int wid, int x, int y, void *data), void *data) {
	mouseMoveCallback[winId] = callback;
	mouseMoveData[winId] = data;
}

void glow::scrollWheelListener(int winId, void (*callback)(glow *gl, int wid, int dx, int dy, int x, int y, void *data), void *data) {
	scrollWheelCallback[winId] = callback;
	scrollWheelData[winId] = data;
}

void glow::keyDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, unsigned short modifiers, int x, int y, void *data), void *data) {
	keyDownCallback[winId] = callback;
	keyDownData[winId] = data;
}

void glow::keyUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, unsigned short modifiers, int x, int y, void *data), void *data) {
	keyUpCallback[winId] = callback;
	keyUpData[winId] = data;
}

void glow::swapBuffers(int winId) {
	glXSwapBuffers(display, windowList[winId]);
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

	XLockDisplay(display);
	XEvent fsEvent;
	fsEvent.type = ClientMessage;
	fsEvent.xclient.window = windowList[winId];
	fsEvent.xclient.message_type = stateMessage;
	fsEvent.xclient.format = 32;
	fsEvent.xclient.data.l[0] = 1;
	fsEvent.xclient.data.l[1] = fullscreenMessage;
	fsEvent.xclient.data.l[2] = 0;
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &fsEvent);
	XUnlockDisplay(display);
}

void glow::disableFullscreen(int winId) {
	if (!fullscreen[winId]) return;

	fullscreen[winId] = false;

	XLockDisplay(display);
	XEvent fsEvent;
	fsEvent.type = ClientMessage;
	fsEvent.xclient.window = windowList[winId];
	fsEvent.xclient.message_type = stateMessage;
	fsEvent.xclient.format = 32;
	fsEvent.xclient.data.l[0] = 0;
	fsEvent.xclient.data.l[1] = fullscreenMessage;
	fsEvent.xclient.data.l[2] = 0;
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &fsEvent);
	XUnlockDisplay(display);
}

void glow::setWindowGeometry(int winId, int x, int y, unsigned int width, unsigned int height) {
	bool wasFullscreen = fullscreen[winId];
	fullscreen[winId] = false;	

	XLockDisplay(display);
	if (wasFullscreen) {
		XEvent fsEvent;
		fsEvent.type = ClientMessage;
		fsEvent.xclient.window = windowList[winId];
		fsEvent.xclient.message_type = stateMessage;
		fsEvent.xclient.format = 32;
		fsEvent.xclient.data.l[0] = 0;
		fsEvent.xclient.data.l[1] = fullscreenMessage;
		fsEvent.xclient.data.l[2] = 0;
		XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &fsEvent);
	}

	Screen *screen = XDefaultScreenOfDisplay(display);
	int screenW = XWidthOfScreen(screen);
	int screenH = XHeightOfScreen(screen);

	if (x == GLOW_CENTER_HORIZONTAL) x = (screenW / 2) - (width / 2);
	if (y == GLOW_CENTER_VERTICAL) y = (screenH / 2) - (height / 2);

	XMoveResizeWindow(display, windowList[winId], x, y, width, height);
	XUnlockDisplay(display);
}

void glow::setWindowTitle(int winId, std::string title) {
	XLockDisplay(display);
	XStoreName(display, windowList[winId], title.c_str());
	XUnlockDisplay(display);
}

void glow::runLoop() {
	Atom idleMessage = XInternAtom(display, "IDLE", False);
	Atom wmProtocols = XInternAtom(display, "WM_PROTOCOLS", False);
	Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);

	struct timeval tp;
	
	int i;
	unsigned short btn;
	int dx;
	int dy;
	int modIdx;
	KeySym keysym;
	XComposeStatus compose;
	XkbStateRec kstate;

	XEvent event;
	int windowCount = 0;
	bool running = true;
	while (running) {
		do {
			XNextEvent(display, &event);
			int winId = -1;
			Window eWin = event.xany.window;
			for (i=0; i<windowList.size(); i++) {
				if (eWin == windowList[i]) winId = i;
			}	
			if (winId < 0) continue;

			XLockDisplay(display);

			XkbGetState(display, XkbUseCoreKbd, &kstate);
			bool shift = kstate.base_mods & shiftmask;
			bool control = kstate.base_mods & ctrlmask;
			bool alt = kstate.base_mods & altmask;
			bool command = kstate.base_mods & cmdmask;
			bool caps = kstate.locked_mods & capsmask;
			bool func = 0;

			unsigned short mod = (func << 5) | (caps << 4) | (command << 3) | (alt << 2) | (control << 1) | (shift);

			switch (event.type) {
				case MapNotify:
					requiresRender[winId] = true;
   					gettimeofday(&tp, NULL);
    				startTime[winId] = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    				prevTime[winId] = startTime[winId];
					isIdle[winId] = true;
					winIsOpen[winId] = 1;
					windowCount++;
					XSetWMProtocols(display, windowList[winId], &wmDeleteMessage, 1);
					break;
				case ConfigureNotify:
					if (!initWindowPlacement[winId]) {
						initWindowPlacement[winId] = true;
					}
					else {
						if (resizeCallback[winId] && (prevW[winId] != event.xconfigure.width || prevH[winId] != event.xconfigure.height)) {
							resizeCallback[winId](this, winId, event.xconfigure.width, event.xconfigure.height, event.xconfigure.width, event.xconfigure.height, resizeData[winId]);
						}
						prevX[winId] = event.xconfigure.x;
						prevY[winId] = event.xconfigure.y;
						prevW[winId] = event.xconfigure.width;
						prevH[winId] = event.xconfigure.height;
					}
					break;
				case KeyPress:
					if (!keyDownCallback[winId]) break;

					modIdx = (shift & ShiftMask) | (caps % LockMask);
					keysym = XLookupKeysym((XKeyEvent*)&event, modIdx);
					if (keysym == XK_ISO_Left_Tab) keysym = XK_Tab;
					if ((keysym >= XK_space && keysym <= XK_asciitilde) || keysym == XK_Tab || keysym == XK_BackSpace || keysym == XK_Delete || keysym == XK_Return || keysym == XK_Escape) {
						keyDownCallback[winId](this, winId, keysym & 0xFF, mod, mouseX, mouseY, keyDownData[winId]);
					}
					else {
						if (keysym == XK_Caps_Lock) {
							if (caps) {
								keyDownCallback[winId](this, winId, specialKey(keysym), mod, mouseX, mouseY, keyDownData[winId]);
							}
							else {
								keyUpCallback[winId](this, winId, specialKey(keysym), mod, mouseX, mouseY, keyDownData[winId]);
							}
						}
						else {
							keyDownCallback[winId](this, winId, specialKey(keysym), mod, mouseX, mouseY, keyDownData[winId]);
						}
					}
					break;
				case KeyRelease:
					if (!keyUpCallback[winId]) break;

					modIdx = (shift & ShiftMask) | (caps % LockMask);
					keysym = XLookupKeysym((XKeyEvent*)&event, modIdx);
					if (keysym == XK_ISO_Left_Tab) keysym = XK_Tab;
					if ((keysym >= XK_space && keysym <= XK_asciitilde) || keysym == XK_Tab || keysym == XK_BackSpace || keysym == XK_Delete || keysym == XK_Return || keysym == XK_Escape) {
						keyUpCallback[winId](this, winId, keysym & 0xFF, mod, mouseX, mouseY, keyDownData[winId]);
					}
					else {
						if (keysym != XK_Caps_Lock) {
							keyUpCallback[winId](this, winId, specialKey(keysym), mod, mouseX, mouseY, keyUpData[winId]);
						}
					}
					break;
				case ButtonPress:
					if (!mouseDownCallback[winId] && !scrollWheelCallback[winId]) break;

					switch (event.xbutton.button) {
						case Button1:
							btn = GLOW_MOUSE_BUTTON_LEFT;
							break;
						case Button3:
							btn = GLOW_MOUSE_BUTTON_RIGHT;
							break;
						case Button4:
							btn = GLOW_MOUSE_SCROLL;
							dx = 0;
							dy = 120;
							break;
						case Button5:
							btn = GLOW_MOUSE_SCROLL;
							dx = 0;
							dy = -120;
							break;
						case Button6:
							btn = GLOW_MOUSE_SCROLL;
							dx = -120;
							dy = 0;
							break;
						case Button7:
							btn = GLOW_MOUSE_SCROLL;
							dx = 120;
							dy = 0;
							break;
						default:
							btn = GLOW_MOUSE_BUTTON_UNKNOWN;
							break;
					}
					if (btn == GLOW_MOUSE_SCROLL && scrollWheelCallback[winId])
						scrollWheelCallback[winId](this, winId, dx, dy, mouseX, mouseY, scrollWheelData[winId]); 
					else if (btn != GLOW_MOUSE_BUTTON_UNKNOWN && mouseDownCallback[winId])
						mouseDownCallback[winId](this, winId, btn, mouseX, mouseY, mouseDownData[winId]);
					break;
				case ButtonRelease:
					if (!mouseUpCallback[winId]) break;

					switch (event.xbutton.button) {
						case Button1:
							btn = GLOW_MOUSE_BUTTON_LEFT;
							break;
						case Button3:
							btn = GLOW_MOUSE_BUTTON_RIGHT;
							break;
						default:
							btn = GLOW_MOUSE_BUTTON_UNKNOWN;
							break;
					}
					if (btn != GLOW_MOUSE_BUTTON_UNKNOWN) mouseUpCallback[winId](this, winId, btn, mouseX, mouseY, mouseUpData[winId]);
					break;
				case MotionNotify:
					mouseX = event.xmotion.x;
					mouseY = event.xmotion.y;
					if (!mouseMoveCallback[winId]) break;

					mouseMoveCallback[winId](this, winId, mouseX, mouseY, mouseMoveData[winId]);
					break;
				case ClientMessage:
					if (event.xclient.message_type == wmProtocols && event.xclient.data.l[0] == wmDeleteMessage) {
						if (windowCount == 1)
							running = false;
						isIdle[winId] = false;
						requiresRender[winId] = false;
						glXDestroyContext(display, glCtxList[winId]);
						XDestroyWindow(display, windowList[winId]);
						winIsOpen[winId] = 0;
						windowCount--;
					}
					else if (event.xclient.message_type == timeoutMessage) {						
						timeoutCallbacks[event.xclient.data.l[0]](this, winId, event.xclient.data.l[0], timeoutData[event.xclient.data.l[0]]);
					}
					else if (event.xclient.message_type == idleMessage) {
						idleCallback[winId](this, winId, idleData[winId]);
						isIdle[winId] = true;
					}
					break;
				default:
					break;
			}
			XUnlockDisplay(display);
		} while (XPending(display));

		if (!running) break;

		for (i=0; i<windowList.size(); i++) {
			if (isIdle[i] && idleCallback[i]) {
				XLockDisplay(display);
				isIdle[i] = false;
				XClientMessageEvent idleEvent;
				idleEvent.type = ClientMessage;
				idleEvent.window = windowList[i];
				idleEvent.message_type = idleMessage;
				idleEvent.format = 32;
				idleEvent.data.l[0] = 0;
				XSendEvent(display, windowList[i], False, 0, (XEvent*)&idleEvent);
				XUnlockDisplay(display);
			}
			if (requiresRender[i] && renderCallback[i]) {
				struct timeval tp;
				gettimeofday(&tp, NULL);
				long now = tp.tv_sec * 1000 + tp.tv_usec / 1000;
				glXMakeCurrent(display, windowList[i], glCtxList[i]);
				renderCallback[i](this, i, now - startTime[i], now - prevTime[i], renderData[i]);
				prevTime[i] = now;
				requiresRender[i] = false;
			}
		}
	}

	XCloseDisplay(display);
}

unsigned short glow::specialKey(KeySym code) {
	unsigned short key;
    switch (code) {
        case XK_Shift_L:
            key = GLOW_KEY_LEFT_SHIFT;
            break;
		case XK_Shift_R:
            key = GLOW_KEY_RIGHT_SHIFT;
            break;
		case XK_Control_L:
            key = GLOW_KEY_LEFT_CONTROL;
            break;
		case XK_Control_R:
            key = GLOW_KEY_RIGHT_CONTROL;
            break;
		case XK_Alt_L:
            key = GLOW_KEY_LEFT_ALT;
            break;
		case XK_Alt_R:
            key = GLOW_KEY_RIGHT_ALT;
            break;
		case XK_Super_L:
            key = GLOW_KEY_LEFT_COMMAND;
            break;
		case XK_Super_R:
            key = GLOW_KEY_RIGHT_COMMAND;
            break;
        case XK_Caps_Lock:
            key = GLOW_KEY_CAPS_LOCK;
            break;
        case XK_F1:
            key = GLOW_KEY_F1;
            break;
        case XK_F2:
            key = GLOW_KEY_F2;
            break;
        case XK_F3:
            key = GLOW_KEY_F3;
            break;
        case XK_F4:
            key = GLOW_KEY_F4;
            break;
        case XK_F5:
            key = GLOW_KEY_F5;
            break;
        case XK_F6:
            key = GLOW_KEY_F6;
            break;
        case XK_F7:
            key = GLOW_KEY_F7;
            break;
        case XK_F8:
            key = GLOW_KEY_F8;
            break;
        case XK_F9:
            key = GLOW_KEY_F9;
            break;
        case XK_F10:
            key = GLOW_KEY_F10;
            break;
        case XK_F11:
            key = GLOW_KEY_F11;
            break;
        case XK_F12:
            key = GLOW_KEY_F12;
            break;
        case XK_Left:
            key = GLOW_KEY_LEFT_ARROW;
            break;
        case XK_Right:
            key = GLOW_KEY_RIGHT_ARROW;
            break;
        case XK_Down:
            key = GLOW_KEY_DOWN_ARROW;
            break;
        case XK_Up:
            key = GLOW_KEY_UP_ARROW;
            break;
        default:
			printf("unknown: %lu\n", code);
            key = GLOW_KEY_NONE;
            break;
    }
    return key;
}

void glow::createFontFace(std::string fontFile, unsigned int size, GLOW_FontFace **facePtr) {
	*facePtr = (GLOW_FontFace*)malloc(sizeof(GLOW_FontFace));
	if(FT_New_Face(ft, fontFile.c_str(), 0, &((*facePtr)->face))) {
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
		(*glyphs)[c].charcode = charcode;
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

	int r = *width % 4;
	if (r != 0)
		*width = *width + 4 - r;
	r = *height % 4;
	if (r != 0)
		*height = *height + 4 - r;

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

	int r = *width % 4;
	if (r != 0)
		*width = *width + 4 - r;
	r = *height % 4;
	if (r != 0)
		*height = *height + 4 - r;

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

void glow::renderStringToTextureWithWrap(GLOW_FontFace *face, std::string utf8Text, int wrapWidth, bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels) {
	int i, j, k;
	std::vector<GLOW_CharGlyph> glyphs;

	unsigned int totalW, totalH, bline;
	getRenderedGlyphsFromString(face, utf8Text, &totalW, &totalH, &bline, &glyphs);

	int lines, x, lineHeight, space;
	bool hyphen;
	std::vector<int> lineBreaks;	
	x = 0;
	lines = 1;
	lineHeight = (int)((double)face->size * 1.1);
	space = -1;
	hyphen = false;
	i = 0;
	while (i<glyphs.size()) {
		if (glyphs[i].charcode == ' ' || glyphs[i].charcode == '\t' || glyphs[i].charcode == '\n' || glyphs[i].charcode == '\r') {
			space = i;
		}
		else if (glyphs[i].charcode == '-') {
			space = i;
			hyphen = true;
		}
		if (x + (glyphs[i].width) > wrapWidth) {
			x = 0;
			if (space >= 0) {
				if (hyphen) {
					space++;
				}
				else {
					free(glyphs[space].pixels);
					glyphs.erase(glyphs.begin() + space);
				}
			}
			i = space >= 0 ? space : i;
			lineBreaks.push_back(i);
			lines++;
			space = -1;
			hyphen = false;
		}
		else {
			x += glyphs[i].advanceX / 64;
			i++;
		}
	}
	lineBreaks.push_back(glyphs.size());

	int r = wrapWidth % 4;
	if (r == 0)
		*width = wrapWidth;
	else
		*width = wrapWidth + 4 - r;
	r = (lines * lineHeight) % 4;
	if (r == 0)
		*height = lines * lineHeight;
	else
		*height = (lines * lineHeight) + 4 - r;
	
	int size = (*width) * (*height);
	*pixels = (unsigned char*)malloc(size * sizeof(unsigned char));
	memset(*pixels, 0, size);

	lines = 0;
	x = 0;
	int l, pt, ix, iy, idx;
	int start = 0;
	for (l=0; l<lineBreaks.size(); l++) {
		x = 0;
		for (i=start; i<lineBreaks[l]; i++) {
			for (j=0; j<glyphs[i].height; j++) {
				pt = bline - glyphs[i].top + (lines * lineHeight);
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
		start = lineBreaks[l];
		lines++;
	}
}

void glow::renderStringToTextureWithWrap(GLOW_FontFace *face, std::string utf8Text, int wrapWidth, unsigned char color[3], bool flipY, unsigned int *width, unsigned int *height, unsigned int *baseline, unsigned char **pixels) {
	int i, j, k;
	std::vector<GLOW_CharGlyph> glyphs;

	unsigned int totalW, totalH, bline;
	getRenderedGlyphsFromString(face, utf8Text, &totalW, &totalH, &bline, &glyphs);

	int lines, x, lineHeight, space;
	bool hyphen;
	std::vector<int> lineBreaks;	
	x = 0;
	lines = 1;
	lineHeight = (int)((double)face->size * 1.1);
	space = -1;
	hyphen = false;
	i = 0;
	while (i<glyphs.size()) {
		if (glyphs[i].charcode == ' ' || glyphs[i].charcode == '\t' || glyphs[i].charcode == '\n' || glyphs[i].charcode == '\r') {
			space = i;
		}
		else if (glyphs[i].charcode == '-') {
			space = i;
			hyphen = true;
		}
		if (x + (glyphs[i].width) > wrapWidth) {
			x = 0;
			if (space >= 0) {
				if (hyphen) {
					space++;
				}
				else {
					free(glyphs[space].pixels);
					glyphs.erase(glyphs.begin() + space);
				}
			}
			i = space >= 0 ? space : i;
			lineBreaks.push_back(i);
			lines++;
			space = -1;
			hyphen = false;
		}
		else {
			x += glyphs[i].advanceX / 64;
			i++;
		}
	}
	lineBreaks.push_back(glyphs.size());

	int r = wrapWidth % 4;
	if (r == 0)
		*width = wrapWidth;
	else
		*width = wrapWidth + 4 - r;
	r = (lines * lineHeight) % 4;
	if (r == 0)
		*height = lines * lineHeight;
	else
		*height = (lines * lineHeight) + 4 - r;
	
	int size = (*width) * (*height) * 4;
	*pixels = (unsigned char*)malloc(size * sizeof(unsigned char));
	memset(*pixels, 0, size);

	lines = 0;
	x = 0;
	int l, pt, ix, iy, idx;
	int start = 0;
	for (l=0; l<lineBreaks.size(); l++) {
		x = 0;
		for (i=start; i<lineBreaks[l]; i++) {
			for (j=0; j<glyphs[i].height; j++) {
				pt = bline - glyphs[i].top + (lines * lineHeight);
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
		start = lineBreaks[l];
		lines++;
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

