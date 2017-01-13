#include <sys/time.h>
#import <Cocoa/Cocoa.h>
#import <epoxy/gl.h>

#import "glow.h"

#define GLOW_MAX_TIMERS 128

@class NSOpenGLContext;

@interface glView : NSView

- (id) init;
- (id) initWithFrame:(NSRect)rect glowPtr:(glow*)gl ctx:(NSOpenGLContext*)ctx winId:(int)id;
- (void) drawRect:(NSRect)bounds;
- (void) viewDidMoveToWindow;
- (void) windowResized:(NSNotification *)notification;
- (BOOL) isOpaque;
- (BOOL) canBecomeKeyView;
- (BOOL) acceptsFirstResponder;

- (void) mouseDown:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;
- (void) mouseUp:(NSEvent*)event;
- (void) rightMouseUp:(NSEvent*)event;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDragged:(NSEvent*)event;
- (void) rightMouseDragged:(NSEvent*)event;
- (void) scrollWheel:(NSEvent*)event;
- (void) keyDown:(NSEvent*)event;
- (void) keyUp:(NSEvent*)event;
- (void) flagsChanged:(NSEvent*)event;

- (int) setTimeout:(void (*)(glow *gl, int wid, int timeoutId, void *data))callback wait:(unsigned int)wait data:(void*)data;
- (void) cancelTimeout:(int)timeoutId;
- (void) idleTimerFired:(NSTimer*)sender;
- (void) timeoutTimerFired:(NSTimer*)sender;
- (void) setAsActiveContext;
- (void) swapBuffers;
- (void) requestRenderFrame;

- (unsigned short) specialKey:(unsigned short)code;

- (void) renderFunction:(void (*)(glow *gl, int wid, unsigned long t, unsigned int dt, void *data))callback data:(void*)data;
- (void) idleFunction:(void (*)(glow *gl, int wid, void *data))callback data:(void*)data;
- (void) resizeFunction:(void (*)(glow *gl, int wid, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data))callback data:(void*)data;

- (void) mouseDownListener:(void (*)(glow *gl, int wid, unsigned short button, int x, int y, void *data))callback data:(void*)data;
- (void) mouseUpListener:(void (*)(glow *gl, int wid, unsigned short button, int x, int y, void *data))callback data:(void*)data;
- (void) mouseMoveListener:(void (*)(glow *gl, int wid, int x, int y, void *data))callback data:(void*)data;
- (void) scrollWheelListener:(void (*)(glow *gl, int wid, int dx, int dy, int x, int y, void *data))callback data:(void*)data;
- (void) keyDownListener:(void (*)(glow *gl, int wid, unsigned short key, int x, int y, void *data))callback data:(void*)data;
- (void) keyUpListener:(void (*)(glow *gl, int wid, unsigned short key, int x, int y, void *data))callback data:(void*)data;

@end
