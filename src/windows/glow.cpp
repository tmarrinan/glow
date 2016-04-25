#include "glow.h"


// GLOW C++ Interface
void glow::initialize(unsigned int profile, unsigned int hidpi) {
	glProfile = profile;
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

	// initialize freetype text rendering
	if(FT_Init_FreeType(&ft)) fprintf(stderr, "Error: could not init freetype library\n");
	offsetsFromUTF8[0] = 0x00000000UL;
	offsetsFromUTF8[1] = 0x00003080UL;
	offsetsFromUTF8[2] = 0x000E2080UL;
	offsetsFromUTF8[3] = 0x03C82080UL;
}

void glow::createWindow(std::string title, int x, int y, unsigned int width, unsigned int height) {	
	
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
	/*
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
	*/
	timerId = (timerId + 1) % GLOW_MAX_TIMERS;
	return tId;
}

void glow::cancelTimeout(unsigned int timeoutId) {
	
}

void glow::timeoutTimerFired(union sigval arg) {
	
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
	
}

void glow::requestRenderFrame() {
	requiresRender = true;
}

void glow::runLoop() {
	
}

unsigned short glow::specialKey(KeySym code) {
	unsigned short key;
    /*
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
	*/
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
