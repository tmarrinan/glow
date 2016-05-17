#include "glow.h"


PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

// GLOW C++ Interface
void glow::initialize(unsigned int profile, unsigned int vmajor, unsigned int vminor, unsigned int hidpi) {
	glProfile = profile;
	glCoreVMajor = vmajor;
	glCoreVMinor = vminor;
	hiDPISupport = hidpi;

	mouseX = 0;
    mouseY = 0;

	timerId = 0;

	renderCallback = NULL;
	idleCallback = NULL;
	resizeCallback = NULL;
	keyDownCallback = NULL;
	keyUpCallback = NULL;
	mouseDownCallback = NULL;
	mouseUpCallback = NULL;
	mouseMoveCallback = NULL;
	scrollWheelCallback = NULL;

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

	capsmask = XkbKeysymToModifiers(display, XK_Caps_Lock);
}

void glow::createWindow(std::string title, int x, int y, unsigned int width, unsigned int height) {	
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
		exit(1);
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

	window = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, (CWBorderPixel | CWColormap | CWEventMask), &swa);
	if (!window) {
		fprintf(stderr, "Failed to create window\n");
		exit(1);
	}
	XFree(vi);

	XStoreName(display, window, title.c_str());

	ctx = NULL;
	if (glProfile == GLOW_OPENGL_CORE) {
		if (!glXCreateContextAttribsARB) {
			fprintf(stderr, "Failed to obtain OpenGL 3.0+ context\n");
			exit(1);
		}
		int glProfileAttribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB, glCoreVMajor,
			GLX_CONTEXT_MINOR_VERSION_ARB, glCoreVMinor,
			GLX_CONTEXT_FLAGS_ARB,         GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
			None
		};
		ctx = glXCreateContextAttribsARB(display, bestFbc, 0, True, glProfileAttribs);
	}
	else {
		ctx = glXCreateNewContext(display, bestFbc, GLX_RGBA_TYPE, 0, True);
	}
	XSync(display, False);
	if (!glXIsDirect(display, ctx)) {
		fprintf(stderr, "Warning: using indirect rendering context\n");
	}
	glXMakeCurrent(display, window, ctx);

	XMapWindow(display, window);

	Screen *screen = XDefaultScreenOfDisplay(display);
	int screenW = XWidthOfScreen(screen);
	int screenH = XHeightOfScreen(screen);

	if (x == GLOW_CENTER_HORIZONTAL) x = (screenW / 2) - (width / 2);
	if (y == GLOW_CENTER_VERTICAL) y = (screenH / 2) - (height / 2);
	prevX = x;
	prevY = y;
	prevW = width;
	prevH = height;
	fullscreen = false;
	XMoveWindow(display, window, x, y);
}

void glow::renderFunction(void (*callback)(unsigned long t, unsigned int dt, glow *gl)) {
	renderCallback = callback;
}

void glow::idleFunction(void (*callback)(glow *gl)) {
	idleCallback = callback;
}

void glow::resizeFunction(void (*callback)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl)) {
	resizeCallback = callback;
}

unsigned int glow::setTimeout(void (*callback)(unsigned int timeoutId, glow *gl), unsigned int wait) {
	int tId = timerId;
	struct sigevent se;
	struct itimerspec ts;

	GLOW_TimerData *data = (GLOW_TimerData*)malloc(sizeof(GLOW_TimerData));
	data->glow_ptr = this;
	data->timer_id = tId;

	se.sigev_notify = SIGEV_THREAD;
	se.sigev_value.sival_ptr = data;
	se.sigev_notify_function = timeoutTimerFired;// glow method?
	se.sigev_notify_attributes = NULL;

	ts.it_value.tv_sec = wait / 1000;
	ts.it_value.tv_nsec = (unsigned long)(wait % 1000) * (unsigned long)1000000;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;

	timeoutCallbacks[tId] = callback;

	timer_create(CLOCK_MONOTONIC, &se, &timeoutTimers[tId]);
	timer_settime(timeoutTimers[tId], 0, &ts, NULL);
	
	timerId = (timerId + 1) % GLOW_MAX_TIMERS;
	return tId;
}

void glow::cancelTimeout(unsigned int timeoutId) {
	timer_delete(timeoutTimers[timeoutId]);
}

void glow::timeoutTimerFired(union sigval arg) {
	GLOW_TimerData *data = (GLOW_TimerData*)arg.sival_ptr;

	XLockDisplay(data->glow_ptr->display);
	
	XClientMessageEvent timeoutEvent;
	timeoutEvent.type = ClientMessage;
	timeoutEvent.window = data->glow_ptr->window;
	timeoutEvent.message_type = data->glow_ptr->timeoutMessage;
	timeoutEvent.format = 32;
	timeoutEvent.data.l[0] = data->timer_id;
	XSendEvent(data->glow_ptr->display, data->glow_ptr->window, False, 0, (XEvent*)&timeoutEvent);
	XFlush(data->glow_ptr->display);

	XUnlockDisplay(data->glow_ptr->display);

	timer_delete(data->glow_ptr->timeoutTimers[data->timer_id]);
	free(data);
}

void glow::mouseDownListener(void (*callback)(unsigned short button, int x, int y, glow *gl)) {
	mouseDownCallback = callback;
}

void glow::mouseUpListener(void (*callback)(unsigned short button, int x, int y, glow *gl)) {
	mouseUpCallback = callback;
}

void glow::mouseMoveListener(void (*callback)(int x, int y, glow *gl)) {
	mouseMoveCallback = callback;
}

void glow::scrollWheelListener(void (*callback)(int dx, int dy, int x, int y, glow *gl)) {
	scrollWheelCallback = callback;
}

void glow::keyDownListener(void (*callback)(unsigned short key, int x, int y, glow *gl)) {
	keyDownCallback = callback;
}

void glow::keyUpListener(void (*callback)(unsigned short key, int x, int y, glow *gl)) {
	keyUpCallback = callback;
}

void glow::swapBuffers() {
	glXSwapBuffers(display, window);
}

void glow::requestRenderFrame() {
	requiresRender = true;
}

void glow::enableFullscreen() {
	if (fullscreen) return;

	fullscreen = true;

	XLockDisplay(display);
	XEvent fsEvent;
	fsEvent.type = ClientMessage;
	fsEvent.xclient.window = window;
	fsEvent.xclient.message_type = stateMessage;
	fsEvent.xclient.format = 32;
	fsEvent.xclient.data.l[0] = 1;
	fsEvent.xclient.data.l[1] = fullscreenMessage;
	fsEvent.xclient.data.l[2] = 0;
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &fsEvent);
	XUnlockDisplay(display);
}

void glow::disableFullscreen() {
	if (!fullscreen) return;

	fullscreen = false;

	XLockDisplay(display);
	XEvent fsEvent;
	fsEvent.type = ClientMessage;
	fsEvent.xclient.window = window;
	fsEvent.xclient.message_type = stateMessage;
	fsEvent.xclient.format = 32;
	fsEvent.xclient.data.l[0] = 0;
	fsEvent.xclient.data.l[1] = fullscreenMessage;
	fsEvent.xclient.data.l[2] = 0;
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &fsEvent);
	XUnlockDisplay(display);
}

void glow::setWindowGeometry(int x, int y, unsigned int width, unsigned int height) {
	bool wasFullscreen = fullscreen;
	fullscreen = false;	

	XLockDisplay(display);
	if (wasFullscreen) {
		XEvent fsEvent;
		fsEvent.type = ClientMessage;
		fsEvent.xclient.window = window;
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

	XMoveResizeWindow(display, window, x, y, width, height);
	XUnlockDisplay(display);
}

void glow::setWindowTitle(std::string title) {
	XLockDisplay(display);
	XStoreName(display, window, title.c_str());
	XUnlockDisplay(display);
}

void glow::runLoop() {
	Atom idleMessage = XInternAtom(display, "IDLE", False);
	Atom wmProtocols = XInternAtom(display, "WM_PROTOCOLS", False);
	Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &wmDeleteMessage, 1);

	long startTime;
    long prevTime;
	struct timeval tp;
	requiresRender = false;

	bool initWindowPlacement = false;

	unsigned short btn;
	int dx;
	int dy;
	int charcount;
	char buffer[20];
	int bufsize = 20;
	KeySym keysym;
	XComposeStatus compose;
	XkbStateRec kstate;

	XEvent event;
	bool isIdle = false;
	bool running = true;
	while (running) {
		do {
			XNextEvent(display, &event);
			if (event.xany.window != window) continue;
			XLockDisplay(display);		

			switch (event.type) {
				case MapNotify:
					requiresRender = true;
   					gettimeofday(&tp, NULL);
    				startTime = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    				prevTime = startTime;
					isIdle = true;
					break;
				case ConfigureNotify:
					if (!initWindowPlacement) {
						if (event.xconfigure.x == prevX && event.xconfigure.y == prevY && event.xconfigure.width == prevW && event.xconfigure.height == prevH) {
							initWindowPlacement = true;
						}
					}
					else {
						if (resizeCallback && (prevW != event.xconfigure.width || prevH != event.xconfigure.height)) {
							resizeCallback(event.xconfigure.width, event.xconfigure.height, event.xconfigure.width, event.xconfigure.height, this);
						}
						prevX = event.xconfigure.x;
						prevY = event.xconfigure.y;
						prevW = event.xconfigure.width;
						prevH = event.xconfigure.height;
					}
					break;
				case KeyPress:
					if (!keyDownCallback) break;

					charcount = XLookupString((XKeyEvent*)&event, buffer, bufsize, &keysym, &compose);
					if ((keysym >= XK_KP_Space && keysym <= XK_KP_9) || (keysym >= XK_space && keysym <= XK_asciitilde) || keysym == XK_BackSpace || keysym == XK_Delete || keysym == XK_Return || keysym == XK_Escape) {
						keyDownCallback(buffer[0], mouseX, mouseY, this);
					}
					else {
						if (keysym == XK_Caps_Lock) {
							XkbGetState(display, XkbUseCoreKbd, &kstate);
							if (kstate.locked_mods & capsmask) {
								keyDownCallback(specialKey(keysym), mouseX, mouseY, this);
							}
							else {
								keyUpCallback(specialKey(keysym), mouseX, mouseY, this);
							}
						}
						else {
							keyDownCallback(specialKey(keysym), mouseX, mouseY, this);
						}
					}
					break;
				case KeyRelease:
					if (!keyUpCallback) break;

					charcount = XLookupString((XKeyEvent*)&event, buffer, bufsize, &keysym, &compose);
					if ((keysym >= XK_KP_Space && keysym <= XK_KP_9) || (keysym >= XK_space && keysym <= XK_asciitilde) || keysym == XK_BackSpace || keysym == XK_Delete || keysym == XK_Return || keysym == XK_Escape) {
						keyUpCallback(buffer[0], mouseX, mouseY, this);
					}
					else {
						if (keysym != XK_Caps_Lock) {
							keyUpCallback(specialKey(keysym), mouseX, mouseY, this);
						}
					}
					break;
				case ButtonPress:
					if (!mouseDownCallback && !scrollWheelCallback) break;

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
					if (btn == GLOW_MOUSE_SCROLL && scrollWheelCallback)
						scrollWheelCallback(dx, dy, mouseX, mouseY, this); 
					else if (btn != GLOW_MOUSE_BUTTON_UNKNOWN && mouseDownCallback)
						mouseDownCallback(btn, mouseX, mouseY, this);
					break;
				case ButtonRelease:
					if (!mouseUpCallback) break;

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
					if (btn != GLOW_MOUSE_BUTTON_UNKNOWN) mouseUpCallback(btn, mouseX, mouseY, this);
					break;
				case MotionNotify:
					mouseX = event.xmotion.x;
					mouseY = event.xmotion.y;
					if (!mouseMoveCallback) break;

					mouseMoveCallback(mouseX, mouseY, this);
					break;
				case ClientMessage:
					if (event.xclient.message_type == wmProtocols && event.xclient.data.l[0] == wmDeleteMessage) {
						running = false;
					}
					else if (event.xclient.message_type == timeoutMessage) {						
						timeoutCallbacks[event.xclient.data.l[0]](event.xclient.data.l[0], this);
					}
					else if (event.xclient.message_type == idleMessage) {
						idleCallback(this);
						isIdle = true;
					}
					break;
				default:
					break;
			}
			XUnlockDisplay(display);
		} while (XPending(display));

		if (!running) break;

		if (isIdle && idleCallback) {
			XLockDisplay(display);
			isIdle = false;
			XClientMessageEvent idleEvent;
			idleEvent.type = ClientMessage;
			idleEvent.window = window;
			idleEvent.message_type = idleMessage;
			idleEvent.format = 32;
			idleEvent.data.l[0] = 0;
			XSendEvent(display, window, False, 0, (XEvent*)&idleEvent);
			XUnlockDisplay(display);
		}
		if (requiresRender && renderCallback) {
			struct timeval tp;
			gettimeofday(&tp, NULL);
			long now = tp.tv_sec * 1000 + tp.tv_usec / 1000;
			renderCallback(now - startTime, now - prevTime, this);
			prevTime = now;
			requiresRender = false;
		}
	}

	glXDestroyContext(display, ctx);
	XDestroyWindow(display, window);
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

void glow::getRenderedGlyphsFromString(GLOW_FontFace *face, std::string text, unsigned int *width, unsigned int *height, std::vector<GLOW_CharGlyph> *glyphs) {
	int i = 0;
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
	} while (i < text.length());

	*height = (3 * face->size) / 2;
}

void glow::renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, bool flipY, unsigned int *width, unsigned int *height, unsigned char **pixels) {
	int i, j, k;
	std::vector<GLOW_CharGlyph> glyphs;

	getRenderedGlyphsFromString(face, utf8Text, width, height, &glyphs);

	*width = (*width) + (4 - ((*width) % 4));
	*height = (*height) + (4 - ((*height) % 4));
	unsigned int bline = face->size / 2;

	int size = (*width) * (*height);
	*pixels = (unsigned char*)malloc(size * sizeof(unsigned char));
	memset(*pixels, 0, size);

	int x = 0;
	int pt, ix, iy, idx;
	for (i=0; i<glyphs.size(); i++) {
		for (j=0; j<glyphs[i].height; j++) {
			for (k=0; k<glyphs[i].width; k++) {
				pt = (*height) - (bline + glyphs[i].top);
				ix = glyphs[i].left + x + k;
				iy = pt + j;
				if (flipY) iy = ((*height) - iy);
				idx = (*width) * iy + ix;
				if (idx < 0 || idx >= size) continue;
				(*pixels)[idx] = glyphs[i].pixels[glyphs[i].width * j + k];
			}
		}
		x += glyphs[i].advanceX / 64;
		free(glyphs[i].pixels);
	}
}

void glow::renderStringToTexture(GLOW_FontFace *face, std::string utf8Text, unsigned char color[3], bool flipY, unsigned int *width, unsigned int *height, unsigned char **pixels) {
	int i, j, k;
	std::vector<GLOW_CharGlyph> glyphs;

	getRenderedGlyphsFromString(face, utf8Text, width, height, &glyphs);

	*width = (*width) + (4 - ((*width) % 4));
	*height = (*height) + (4 - ((*height) % 4));
	unsigned int bline = face->size / 2;

	int size = (*width) * (*height) * 4;
	*pixels = (unsigned char*)malloc(size * sizeof(unsigned char));
	memset(*pixels, 0, size);

	int x = 0;
	int pt, ix, iy, idx;
	for (i=0; i<glyphs.size(); i++) {
		for (j=0; j<glyphs[i].height; j++) {
			for (k=0; k<glyphs[i].width; k++) {
				pt = (*height) - (bline + glyphs[i].top);
				ix = glyphs[i].left + x + k;
				iy = pt + j;
				if (flipY) iy = ((*height) - iy);
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
	std::string version = (const char *)glGetString(GL_VERSION);

    int i;
    int end = 0;
    int dot = 0;
    for (i=0; i<version.length(); i++){
        if (version[i] == '.') {
            dot++;
            if (dot == 1) continue;
        }
        if (version[i] < 48 || version[i] > 57) {
            end = i;
            break;
        }
    }

    *glv = version.substr(0, end);
    *glslv = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
}

