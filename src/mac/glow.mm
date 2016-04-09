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

// GLOW C++ Wrapped Interface
void glow::initialize(unsigned int profile, unsigned int hidpi) {
	pool = [[NSAutoreleasePool alloc] init];

	GLDelegate *glDelegate = [[GLDelegate alloc] init];
	[[NSApplication sharedApplication] setDelegate:glDelegate];

	glProfileAttrib = (profile == GLOW_OPENGL_3_2_CORE) ? NSOpenGLProfileVersion3_2Core : NSOpenGLProfileVersionLegacy;
	hiDPISupport = hidpi;

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

	// initialize freetype text rendering
	if(FT_Init_FreeType(&ft)) fprintf(stderr, "Error: could not init freetype library\n");
	offsetsFromUTF8[0] = 0x00000000UL;
	offsetsFromUTF8[1] = 0x00003080UL;
	offsetsFromUTF8[2] = 0x000E2080UL;
	offsetsFromUTF8[3] = 0x03C82080UL;
}

void glow::createWindow(std::string title, int x, int y, unsigned int width, unsigned int height) {
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
		NSOpenGLPFASampleBuffers, 0,
		0,
    };
	NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttrs];
	glContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:NULL];

	glView* view = [[glView alloc] initWithFrame:NSMakeRect(0, 0, width, height) glowPtr:this ctx:glContext];
	if (hiDPISupport == GLOW_HIDPI_WINDOW) [view setWantsBestResolutionOpenGLSurface:YES];
	else                                   [view setWantsBestResolutionOpenGLSurface:NO];

	int styleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	NSString* appTitle = [NSString stringWithUTF8String:title.c_str()];
	mainwindow = [[[NSWindow alloc] initWithContentRect:[view frame] styleMask:styleMask backing:NSBackingStoreBuffered defer:NO] autorelease];
	[mainwindow cascadeTopLeftFromPoint:NSMakePoint(20,20)];
	[mainwindow setTitle:appTitle];
	[mainwindow setContentView:view];
	[mainwindow setFrameOrigin:NSMakePoint(x, (int)screen.size.height - y - height)];
	[mainwindow makeKeyAndOrderFront:nil];
	[mainwindow setAcceptsMouseMovedEvents:YES];

	[glContext makeCurrentContext];
	[glContext setView:view];
}

void glow::renderFunction(void (*callback)(unsigned long t, unsigned int dt, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view renderFunction:callback];
}

void glow::idleFunction(void (*callback)(glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view idleFunction:callback];
}

void glow::resizeFunction(void (*callback)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view resizeFunction:callback];
}

unsigned int glow::setTimeout(void (*callback)(unsigned int timeoutId, glow *gl), unsigned int wait) {
	glView* view = (glView*)[glContext view];
	unsigned int t = [view setTimeout:callback wait:wait];
	return t;
}

void glow::cancelTimeout(unsigned int timeoutId) {
	glView* view = (glView*)[glContext view];
	[view cancelTimeout:timeoutId];
}

void glow::mouseDownListener(void (*callback)(unsigned short button, int x, int y, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view mouseDownListener:callback];
}

void glow::mouseUpListener(void (*callback)(unsigned short button, int x, int y, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view mouseUpListener:callback];
}

void glow::mouseMoveListener(void (*callback)(int x, int y, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view mouseMoveListener:callback];
}

void glow::scrollWheelListener(void (*callback)(int dx, int dy, int x, int y, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view scrollWheelListener:callback];
}

void glow::keyDownListener(void (*callback)(unsigned short key, int x, int y, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view keyDownListener:callback];
}

void glow::keyUpListener(void (*callback)(unsigned short key, int x, int y, glow *gl)) {
	glView* view = (glView*)[glContext view];
	[view keyUpListener:callback];
}

void glow::swapBuffers() {
	glView* view = (glView*)[glContext view];
	[view swapBuffers];
}

void glow::requestRenderFrame() {
	glView* view = (glView*)[glContext view];
	[view requestRenderFrame];
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

void glow::getRenderedGlyphsFromString(GLOW_FontFace *face, std::string text, unsigned int *width, unsigned int *height, std::vector<charGlyph> *glyphs) {
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

		glyphs->push_back(charGlyph());
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
	std::vector<charGlyph> glyphs;

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
	std::vector<charGlyph> glyphs;

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
