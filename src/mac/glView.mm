#import "mac/glView.h"


@implementation glView {
    glow *glowPtr;
    NSOpenGLContext* glContext;
    
    void (* renderCallback)(unsigned long t, unsigned int dt, glow *gl);
    void (* idleCallback)(glow *gl);
    void (* resizeCallback)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl);
    void (* mouseDownCallback)(unsigned short button, int x, int y, glow *gl);
    void (* mouseUpCallback)(unsigned short button, int x, int y, glow *gl);
    void (* mouseMoveCallback)(int x, int y, glow *gl);
    void (* scrollWheelCallback)(int dx, int dy, int x, int y, glow *gl);
    void (* keyDownCallback)(unsigned short key, int x, int y, glow *gl);
    void (* keyUpCallback)(unsigned short key, int x, int y, glow *gl);

    bool leftShift;
    bool rightShift;
    bool leftAlt;
    bool rightAlt;
    bool leftCtrl;
    bool rightCtrl;
    bool leftCmd;
    bool rightCmd;
    bool function;
    bool capsLock;

    int mouseX;
    int mouseY;

    long startTime;
    long prevTime;

    double dpiScale;

    unsigned int timerId;
    NSTimer *timeoutTimers[GLOW_MAX_TIMERS];
    void (* timeoutCallbacks[GLOW_MAX_TIMERS])(unsigned int timeoutId, glow *gl);
}

- (id) init {
    [NSException raise:@"NotImplementedError" format:@""];
    return nil;
}

- (id) initWithFrame:(NSRect)rect glowPtr:(glow*)gl ctx:(NSOpenGLContext*)ctx {
    self = [super initWithFrame:rect];
    if (self == nil)
        return nil;

    glowPtr = gl;
    glContext = ctx;

    GLint swapInt = 1;
    [glContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

    renderCallback = nil;
    idleCallback = nil;
    resizeCallback = nil;
    mouseDownCallback = nil;
    mouseUpCallback = nil;
    mouseMoveCallback = nil;
    scrollWheelCallback = nil;
    keyDownCallback = nil;
    keyUpCallback = nil;

    leftShift = false;
    rightShift = false;
    leftAlt = false;
    rightAlt = false;
    leftCtrl = false;
    rightCtrl = false;
    leftCmd = false;
    rightCmd = false;
    function = false;
    capsLock = false;

    mouseX = 0;
    mouseY = 0;

    struct timeval tp;
    gettimeofday(&tp, NULL);
    startTime = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    prevTime = startTime;

    dpiScale = 1.0;

    timerId = 0;

    return self;
}

- (BOOL) isOpaque {
    return YES;
}

- (BOOL) canBecomeKeyView {
    return YES;
}

- (BOOL) acceptsFirstResponder {
    return YES;
}

- (void) mouseDown:(NSEvent*)event {
    if (!mouseDownCallback) return;

    mouseDownCallback(GLOW_MOUSE_BUTTON_LEFT, mouseX, mouseY, glowPtr);
}

- (void) rightMouseDown:(NSEvent*)event {
    if (!mouseDownCallback) return;
    
    mouseDownCallback(GLOW_MOUSE_BUTTON_RIGHT, mouseX, mouseY, glowPtr);
}

- (void) mouseUp:(NSEvent*)event {
    if (!mouseUpCallback) return;

    mouseUpCallback(GLOW_MOUSE_BUTTON_LEFT, mouseX, mouseY, glowPtr);
}

- (void) rightMouseUp:(NSEvent*)event {
    if (!mouseUpCallback) return;
    
    mouseUpCallback(GLOW_MOUSE_BUTTON_RIGHT, mouseX, mouseY, glowPtr);
}

- (void) mouseMoved:(NSEvent*)event {
    NSPoint mousePos = [event locationInWindow];
    NSSize windowSize = [self frame].size;
    mouseX = mousePos.x;
    mouseY = windowSize.height - mousePos.y;
    if (mouseX < 0) mouseX = 0;
    if (mouseY < 0) mouseY = 0;
    if (mouseX > windowSize.width - 1) mouseX = windowSize.width - 1;
    if (mouseY > windowSize.height - 1) mouseY = windowSize.height - 1;

    if (!mouseMoveCallback) return;

    mouseMoveCallback(mouseX, mouseY, glowPtr);
}

- (void) mouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void) rightMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void) scrollWheel:(NSEvent*)event; {
    if (!scrollWheelCallback) return;

    scrollWheelCallback([event scrollingDeltaX], [event scrollingDeltaY], mouseX, mouseY, glowPtr);
}

- (void) keyDown:(NSEvent*)event {
    if (!keyDownCallback) return;

    unsigned char key = [[event characters] UTF8String][0];
    if (key < 128) {
        keyDownCallback(key, mouseX, mouseY, glowPtr);
    }
    else {
        keyDownCallback([self specialKey:[event keyCode]], mouseX, mouseY, glowPtr);
    }
}

- (void) keyUp:(NSEvent*)event {
    if (!keyUpCallback) return;

    unsigned char key = [[event characters] UTF8String][0];
    if (key < 128) {
        keyUpCallback(key, mouseX, mouseY, glowPtr);
    }
    else {
        keyUpCallback([self specialKey:[event keyCode]], mouseX, mouseY, glowPtr);
    }
}

- (void) flagsChanged:(NSEvent*)event {
    if (!keyDownCallback) return;

    bool keydown;
    switch ([event keyCode]) {
        case 54:
            rightCmd = !rightCmd;
            keydown = rightCmd;
            break;
        case 55:
            leftCmd = !leftCmd;
            keydown = leftCmd;
            break;
        case 56:
            leftShift = !leftShift;
            keydown = leftShift;
            break;
        case 57:
            capsLock = !capsLock;
            keydown = capsLock;
        case 58:
            leftAlt = !leftAlt;
            keydown = leftAlt;
            break;
        case 59:
            leftCtrl = !leftCtrl;
            keydown = leftCtrl;
            break;
        case 60:
            rightShift = !rightShift;
            keydown = rightShift;
            break;
        case 61:
            rightAlt = !rightAlt;
            keydown = rightAlt;
            break;
        case 62:
            rightCtrl = !rightCtrl;
            keydown = rightCtrl;
            break;
        case 63:
            function = !function;
            keydown = function;
            break;
        default:
            return;
    }

    if (keydown)
        keyDownCallback([self specialKey:[event keyCode]], mouseX, mouseY, glowPtr);
    else
        keyUpCallback([self specialKey:[event keyCode]], mouseX, mouseY, glowPtr);
}

- (void) drawRect:(NSRect)bounds {
    if (!renderCallback) return;

    struct timeval tp;
    gettimeofday(&tp, NULL);
    long now = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    renderCallback(now - startTime, now - prevTime, glowPtr);
    
    prevTime = now;

    if (!idleCallback) return;

    NSTimer *renderTimer = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(idleTimerFired:) userInfo:nil repeats:NO];
    [[NSRunLoop currentRunLoop] addTimer:renderTimer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:renderTimer forMode:NSEventTrackingRunLoopMode];
}

- (void) idleTimerFired:(NSTimer*)sender {
    idleCallback(glowPtr);
}

- (void) timeoutTimerFired:(NSTimer*)sender {
    NSNumber *tId = [sender userInfo];
    unsigned int timeoutId = [tId intValue];
    timeoutCallbacks[timeoutId](timeoutId, glowPtr);
}

- (void) swapBuffers {
    [glContext flushBuffer];
}

- (void) requestRenderFrame {
    [self setNeedsDisplay:YES];
    [self display];
}

- (void) viewDidMoveToWindow {
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowResized:) name:NSWindowDidResizeNotification object:[self window]];
}

- (void) windowResized:(NSNotification *)notification {
    if (!resizeCallback) return;

    NSSize wsize = [self frame].size;
    NSSize bsize = [self convertSizeToBacking:wsize];

    [glContext update];

    resizeCallback(wsize.width, wsize.height, bsize.width, bsize.height, glowPtr);
}

- (unsigned short) specialKey:(unsigned short)code {
    unsigned short key;
    switch (code) {
        case 54:
            key = GLOW_KEY_RIGHT_COMMAND;
            break;
        case 55:
            key = GLOW_KEY_LEFT_COMMAND;
            break;
        case 56:
            key = GLOW_KEY_LEFT_SHIFT;
            break;
        case 57:
            key = GLOW_KEY_CAPS_LOCK;
            break;
        case 58:
            key = GLOW_KEY_LEFT_ALT;
            break;
        case 59:
            key = GLOW_KEY_LEFT_CONTROL;
            break;
        case 60:
            key = GLOW_KEY_RIGHT_SHIFT;
            break;
        case 61:
            key = GLOW_KEY_RIGHT_ALT;
            break;
        case 62:
            key = GLOW_KEY_RIGHT_CONTROL;
            break;
        case 63:
            key = GLOW_KEY_FUNCTION;
            break;
        case 96:
            key = GLOW_KEY_F5;
            break;
        case 97:
            key = GLOW_KEY_F6;
            break;
        case 98:
            key = GLOW_KEY_F7;
            break;
        case 99:
            key = GLOW_KEY_F3;
            break;
        case 100:
            key = GLOW_KEY_F8;
            break;
        case 101:
            key = GLOW_KEY_F9;
            break;
        case 103:
            key = GLOW_KEY_F11;
            break;
        case 109:
            key = GLOW_KEY_F10;
            break;
        case 111:
            key = GLOW_KEY_F12;
            break;
        case 118:
            key = GLOW_KEY_F4;
            break;
        case 120:
            key = GLOW_KEY_F2;
            break;
        case 122:
            key = GLOW_KEY_F1;
            break;
        case 123:
            key = GLOW_KEY_LEFT_ARROW;
            break;
        case 124:
            key = GLOW_KEY_RIGHT_ARROW;
            break;
        case 125:
            key = GLOW_KEY_DOWN_ARROW;
            break;
        case 126:
            key = GLOW_KEY_UP_ARROW;
            break;
        default:
            key = GLOW_KEY_NONE;
            break;
    }
    return key;
}

- (void) renderFunction:(void (*)(unsigned long t, unsigned int dt, glow *gl))callback {
    renderCallback = callback;
}

- (void) idleFunction:(void (*)(glow *gl))callback {
    idleCallback = callback;
}

- (unsigned int) setTimeout:(void (*)(unsigned int timeoutId, glow *gl))callback wait:(unsigned int)wait {
    id tId = [NSNumber numberWithInteger: timerId];
    NSTimer *timeoutTimer = [NSTimer timerWithTimeInterval:(double)wait/1000.0 target:self selector:@selector(timeoutTimerFired:) userInfo:tId repeats:NO];
    [[NSRunLoop currentRunLoop] addTimer:timeoutTimer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:timeoutTimer forMode:NSEventTrackingRunLoopMode];

    timeoutTimers[timerId] = timeoutTimer;
    timeoutCallbacks[timerId] = callback;
    timerId = (timerId + 1) % GLOW_MAX_TIMERS;
    return [tId intValue];
}

- (void) cancelTimeout:(unsigned int)timeoutId {
    [timeoutTimers[timeoutId] invalidate];
}

- (void) resizeFunction:(void (*)(unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, glow *gl))callback {
    resizeCallback = callback;
}

- (void) mouseDownListener:(void (*)(unsigned short button, int x, int y, glow *gl))callback {
    mouseDownCallback = callback;
}

- (void) mouseUpListener:(void (*)(unsigned short button, int x, int y, glow *gl))callback {
    mouseUpCallback = callback;
}

- (void) mouseMoveListener:(void (*)(int x, int y, glow *gl))callback {
    mouseMoveCallback = callback;
}

- (void) scrollWheelListener:(void (*)(int dx, int dy, int x, int y, glow *gl))callback {
    scrollWheelCallback = callback;
}

- (void) keyDownListener:(void (*)(unsigned short key, int x, int y, glow *gl))callback {
    keyDownCallback = callback;
}

- (void) keyUpListener:(void (*)(unsigned short key, int x, int y, glow *gl))callback {
    keyUpCallback = callback;
}

@end
