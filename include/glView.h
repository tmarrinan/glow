#include <sys/time.h>
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

#import "glow.h"

#define MAX_TIMERS 128

@class NSOpenGLContext;

@interface glView : NSView

- (id) init;
- (id) initWithFrame:(NSRect)rect glowPtr:(glow*)gl ctx:(NSOpenGLContext*)ctx;
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

- (unsigned int) setTimeout:(void (*)(unsigned int timeoutId, glow *gl))callback wait:(unsigned int)wait;
- (void) cancelTimeout:(unsigned int)timeoutId;
- (void) idleTimerFired:(NSTimer*)sender;
- (void) timeoutTimerFired:(NSTimer*)sender;
- (void) swapBuffers;
- (void) requestRenderFrame;

- (unsigned short) specialKey:(unsigned short)code;

- (void) renderFunction:(void (*)(unsigned long t, unsigned int dt, glow *gl))callback;
- (void) idleFunction:(void (*)(glow *gl))callback;
- (void) resizeFunction:(void (*)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl))callback;

- (void) mouseDownListener:(void (*)(unsigned short button, int x, int y, glow *gl))callback;
- (void) mouseUpListener:(void (*)(unsigned short button, int x, int y, glow *gl))callback;
- (void) mouseMoveListener:(void (*)(int x, int y, glow *gl))callback;
- (void) scrollWheelListener:(void (*)(int dx, int dy, int x, int y, glow *gl))callback;
- (void) keyDownListener:(void (*)(unsigned short key, int x, int y, glow *gl))callback;
- (void) keyUpListener:(void (*)(unsigned short key, int x, int y, glow *gl))callback;

@end