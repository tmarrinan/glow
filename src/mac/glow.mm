#include "glow.h"

#import <Cocoa/Cocoa.h>
#import "mac/glView.h"

// Application Delegate (quit app when window is closed)
@interface GLDelegate : NSObject <NSApplicationDelegate>

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app;

@end

@implementation GLDelegate
-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
    return YES;
}
@end

// Window Delegate (gets window notification messages)
@interface WindowDelegate : NSObject <NSWindowDelegate>

-(void) windowDidExitFullScreen:(NSNotification *)notification;

@end

@implementation WindowDelegate {
	glow *glowPtr;
	int winId;
}
-(void) init:(glow *)ptr winId:(int)wid {
	glowPtr = ptr;
	winId = wid;
}
-(void) windowDidExitFullScreen:(NSNotification *)notification {
	glowPtr->exitFullScreenFinished(winId);
}
@end


// GLOW C++ Wrapped Interface
void glow::initialize(unsigned int profile, unsigned int vmajor, unsigned int vminor, unsigned int flags) {
	pool = [[NSAutoreleasePool alloc] init];

	GLDelegate *glDelegate = [[GLDelegate alloc] init];
	[[NSApplication sharedApplication] setDelegate:glDelegate];

	if (profile == GLOW_OPENGL_CORE) {
		if ((vmajor == 3 && vminor > 2) || vmajor > 3) {
			glProfileAttrib = NSOpenGLProfileVersion4_1Core;
		}
		else {
			glProfileAttrib = NSOpenGLProfileVersion3_2Core;
		}
	}
	else {
		glProfileAttrib = NSOpenGLProfileVersionLegacy;
	}
	windowRequiresResize = false;

	bool hideDock = flags & GLOW_FLAGS_HIDE_DOCK;

	// setup app menu
	NSMenu *menu=[[NSMenu alloc] initWithTitle:@"AMainMenu"];
	NSMenuItem* item;
	NSMenu* subMenu;
	item=[[NSMenuItem alloc] initWithTitle:@"Apple" action:NULL keyEquivalent:@""];
	[menu addItem:item];
	subMenu=[[NSMenu alloc] initWithTitle:@"Apple"];
	[menu setSubmenu:subMenu forItem:item];
	[item release];
	item=[[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
	[subMenu addItem:item];
	[item release];
	[subMenu release];
	[NSApp setMenu:menu];

	[NSApp activateIgnoringOtherApps:YES];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	if (hideDock) [NSMenu setMenuBarVisible:NO];
	
	// initialize freetype text rendering
	if(FT_Init_FreeType(&ft)) fprintf(stderr, "Error: could not init freetype library\n");
	offsetsFromUTF8[0] = 0x00000000UL;
	offsetsFromUTF8[1] = 0x00003080UL;
	offsetsFromUTF8[2] = 0x000E2080UL;
	offsetsFromUTF8[3] = 0x03C82080UL;
}

int glow::createWindow(std::string title, int x, int y, unsigned int width, unsigned int height, unsigned int windowtype) {
	NSRect screen = [[NSScreen mainScreen] frame];
	if (x == GLOW_CENTER_HORIZONTAL) x = ((int)screen.size.width / 2) - (width / 2);
	if (y == GLOW_CENTER_VERTICAL) y = ((int)screen.size.height / 2) - (height / 2);
	NSOpenGLPixelFormatAttribute pixelAttrs[] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAOpenGLProfile, glProfileAttrib,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFASampleBuffers, 1,
		NSOpenGLPFASamples, 4,
		0,
    };
	NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttrs];
	NSOpenGLContext* glContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:NULL];

	int wid = windowList.size();
	bool hiDPISupport = windowtype & GLOW_WINDOW_HIDPI;
	bool borderless = windowtype & GLOW_WINDOW_BORDERLESS;

	glView* view = [[glView alloc] initWithFrame:NSMakeRect(0, 0, width, height) glowPtr:this ctx:glContext winId:wid];
	if (hiDPISupport) [view setWantsBestResolutionOpenGLSurface:YES];
	else              [view setWantsBestResolutionOpenGLSurface:NO];

	int styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
	if (borderless) styleMask = NSWindowStyleMaskBorderless;
	NSString* appTitle = [NSString stringWithUTF8String:title.c_str()];
	WindowDelegate *winDelegate = [[WindowDelegate alloc] init];
	[winDelegate init:this winId:wid];
	NSWindow* mainwindow = [[[NSWindow alloc] initWithContentRect:[view frame] styleMask:styleMask backing:NSBackingStoreBuffered defer:NO] autorelease];
	[mainwindow setDelegate:winDelegate];
	[mainwindow cascadeTopLeftFromPoint:NSMakePoint(20,20)];
	[mainwindow setTitle:appTitle];
	[mainwindow setContentView:view];
	[mainwindow setFrameOrigin:NSMakePoint(x, (int)screen.size.height - y - height)];
	[mainwindow makeKeyAndOrderFront:nil];
	[mainwindow setAcceptsMouseMovedEvents:YES];

	[glContext makeCurrentContext];
	[glContext setView:view];

	windowList.push_back(mainwindow);
	glCtxList.push_back(glContext);

	return wid;
}

void glow::setActiveWindow(int winId) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view setAsActiveContext];
}

void glow::renderFunction(int winId, void (*callback)(glow *gl, int wid, unsigned long t, unsigned int dt, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view renderFunction:callback data:data];
}

void glow::idleFunction(int winId, void (*callback)(glow *gl, int wid, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view idleFunction:callback data:data];
}

void glow::resizeFunction(int winId, void (*callback)(glow *gl, int wid, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view resizeFunction:callback data:data];
}

unsigned int glow::setTimeout(void (*callback)(glow *gl, unsigned int timeoutId, void *data), unsigned int wait, void *data) {
	glView* view = (glView*)[glCtxList[0] view];
	unsigned int t = [view setTimeout:callback wait:wait data:data];
	return t;
}

void glow::cancelTimeout(unsigned int timeoutId) {
	glView* view = (glView*)[glCtxList[0] view];
	[view cancelTimeout:timeoutId];
}

void glow::mouseDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view mouseDownListener:callback data:data];
}

void glow::mouseUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short button, int x, int y, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view mouseUpListener:callback data:data];
}

void glow::mouseMoveListener(int winId, void (*callback)(glow *gl, int wid, int x, int y, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view mouseMoveListener:callback data:data];
}

void glow::scrollWheelListener(int winId, void (*callback)(glow *gl, int wid, int dx, int dy, int x, int y, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view scrollWheelListener:callback data:data];
}

void glow::keyDownListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view keyDownListener:callback data:data];
}

void glow::keyUpListener(int winId, void (*callback)(glow *gl, int wid, unsigned short key, int x, int y, void *data), void *data) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view keyUpListener:callback data:data];
}

void glow::swapBuffers(int winId) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view swapBuffers];
}

void glow::requestRenderFrame(int winId) {
	glView* view = (glView*)[glCtxList[winId] view];
	[view requestRenderFrame];
}

void glow::enableFullscreen(int winId) {
	if ([windowList[winId] styleMask] & NSWindowStyleMaskFullScreen) return;

	[windowList[winId] toggleFullScreen:nil];
}

void glow::disableFullscreen(int winId) {
	if (!([windowList[winId] styleMask] & NSWindowStyleMaskFullScreen)) return;

	[windowList[winId] toggleFullScreen:nil];
}

void glow::setWindowGeometry(int winId, int x, int y, unsigned int width, unsigned int height) {
	NSRect screen = [[NSScreen mainScreen] frame];
	if (x == GLOW_CENTER_HORIZONTAL) x = ((int)screen.size.width / 2) - (width / 2);
	if (y == GLOW_CENTER_VERTICAL) y = ((int)screen.size.height / 2) - (height / 2);

	NSRect newFrame = [windowList[winId] frameRectForContentRect:NSMakeRect(x, (int)screen.size.height - y - height, width, height)];
	if (([windowList[winId] styleMask] & NSWindowStyleMaskFullScreen)) {
		windowRequiresResize = true;
		requestedWindowX = x;
		requestedWindowY = y;
		requestedWindowW = width;
		requestedWindowH = height;
		[windowList[winId] toggleFullScreen:nil];
	}
	else {
		[windowList[winId] setFrame:newFrame display:YES animate:YES];
	}
}

void glow::setWindowTitle(int winId, std::string title) {
	NSString* appTitle = [NSString stringWithUTF8String:title.c_str()];
	[windowList[winId] setTitle:appTitle];
}

void glow::exitFullScreenFinished(int winId) {
	if (windowRequiresResize) {
		windowRequiresResize = false;

		NSRect screen = [[NSScreen mainScreen] frame];
		NSRect newFrame = [windowList[winId] frameRectForContentRect:NSMakeRect(requestedWindowX, (int)screen.size.height - requestedWindowY - requestedWindowH, requestedWindowW, requestedWindowH)];
		[windowList[winId] setFrame:newFrame display:YES animate:YES];
	}
}

void glow::runLoop() {
	[NSApp run];

	[pool drain];
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
		int end = 0;
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
	/*std::string version = (const char *)glGetString(GL_VERSION);

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
	*/
}
