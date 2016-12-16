#import "mac/glView.h"


@implementation glView {
    glow *glowPtr;
    int winId;
    NSOpenGLContext* glContext;
    
    void (* renderCallback)(glow *gl, int wid, unsigned long t, unsigned int dt, void *data);
    void (* idleCallback)(glow *gl, int wid, void *data);
    void (* resizeCallback)(glow *gl, int wid, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data);
    void (* mouseDownCallback)(glow *gl, int wid, unsigned short button, int x, int y, void *data);
    void (* mouseUpCallback)(glow *gl, int wid, unsigned short button, int x, int y, void *data);
    void (* mouseMoveCallback)(glow *gl, int wid, int x, int y, void *data);
    void (* scrollWheelCallback)(glow *gl, int wid, int dx, int dy, int x, int y, void *data);
    void (* keyDownCallback)(glow *gl, int wid, unsigned short key, int x, int y, void *data);
    void (* keyUpCallback)(glow *gl, int wid, unsigned short key, int x, int y, void *data);

    void *renderData;
    void *idleData;
    void *resizeData;
    void *mouseDownData;
    void *mouseUpData;
    void *mouseMoveData;
    void *scrollWheelData;
    void *keyDownData;
    void *keyUpData;

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
    void (* timeoutCallbacks[GLOW_MAX_TIMERS])(glow *gl, int wid, int timeoutId, void *data);
    void *timeoutData[GLOW_MAX_TIMERS];
}

- (id) init {
    [NSException raise:@"NotImplementedError" format:@""];
    return nil;
}

- (id) initWithFrame:(NSRect)rect glowPtr:(glow*)gl ctx:(NSOpenGLContext*)ctx winId:(int)wid {
    self = [super initWithFrame:rect];
    if (self == nil)
        return nil;

    glowPtr = gl;
    glContext = ctx;
    winId = wid;

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

    for(int i=0; i<GLOW_MAX_TIMERS; i++) {
        timeoutTimers[i] = nil;
        timeoutCallbacks[i] = NULL;
        timeoutData[i] = NULL;
    }

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

    mouseDownCallback(glowPtr, winId, GLOW_MOUSE_BUTTON_LEFT, mouseX, mouseY, mouseDownData);
}

- (void) rightMouseDown:(NSEvent*)event {
    if (!mouseDownCallback) return;
    
    mouseDownCallback(glowPtr, winId, GLOW_MOUSE_BUTTON_RIGHT, mouseX, mouseY, mouseDownData);
}

- (void) mouseUp:(NSEvent*)event {
    if (!mouseUpCallback) return;

    mouseUpCallback(glowPtr, winId, GLOW_MOUSE_BUTTON_LEFT, mouseX, mouseY, mouseUpData);
}

- (void) rightMouseUp:(NSEvent*)event {
    if (!mouseUpCallback) return;
    
    mouseUpCallback(glowPtr, winId, GLOW_MOUSE_BUTTON_RIGHT, mouseX, mouseY, mouseUpData);
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

    mouseMoveCallback(glowPtr, winId, mouseX, mouseY, mouseMoveData);
}

- (void) mouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void) rightMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void) scrollWheel:(NSEvent*)event; {
    if (!scrollWheelCallback) return;

    scrollWheelCallback(glowPtr, winId, [event scrollingDeltaX], [event scrollingDeltaY], mouseX, mouseY, scrollWheelData);
}

- (void) keyDown:(NSEvent*)event {
    if (!keyDownCallback) return;

    unsigned char key = [[event characters] UTF8String][0];
    if (key < 128) {
        keyDownCallback(glowPtr, winId, key, mouseX, mouseY, keyDownData);
    }
    else {
        keyDownCallback(glowPtr, winId, [self specialKey:[event keyCode]], mouseX, mouseY, keyDownData);
    }
}

- (void) keyUp:(NSEvent*)event {
    if (!keyUpCallback) return;

    unsigned char key = [[event characters] UTF8String][0];
    if (key < 128) {
        keyUpCallback(glowPtr, winId, key, mouseX, mouseY, keyUpData);
    }
    else {
        keyUpCallback(glowPtr, winId, [self specialKey:[event keyCode]], mouseX, mouseY, keyUpData);
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
        keyDownCallback(glowPtr, winId, [self specialKey:[event keyCode]], mouseX, mouseY, keyDownData);
    else
        keyUpCallback(glowPtr, winId, [self specialKey:[event keyCode]], mouseX, mouseY, keyUpData);
}

- (void) drawRect:(NSRect)bounds {
    if (!renderCallback) return;

    struct timeval tp;
    gettimeofday(&tp, NULL);
    long now = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    [glContext makeCurrentContext];
    renderCallback(glowPtr, winId, now - startTime, now - prevTime, renderData);
    
    prevTime = now;

    if (!idleCallback) return;

    NSTimer *renderTimer = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(idleTimerFired:) userInfo:nil repeats:NO];
    [[NSRunLoop currentRunLoop] addTimer:renderTimer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:renderTimer forMode:NSEventTrackingRunLoopMode];
}

- (void) idleTimerFired:(NSTimer*)sender {
    idleCallback(glowPtr, winId, idleData);
    sender = nil;
}

- (void) timeoutTimerFired:(NSTimer*)sender {
    NSNumber *tId = [sender userInfo];
    int timeoutId = [tId intValue];
    timeoutCallbacks[timeoutId](glowPtr, winId, timeoutId, timeoutData[timeoutId]);
    timeoutTimers[timeoutId] = nil;
}

- (void) setAsActiveContext {
    [glContext makeCurrentContext];
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

    [glContext makeCurrentContext];
    [glContext update];

    resizeCallback(glowPtr, winId, wsize.width, wsize.height, bsize.width, bsize.height, resizeData);
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

- (void) renderFunction:(void (*)(glow *gl, int winId, unsigned long t, unsigned int dt, void *data))callback data:(void*)data {
    renderCallback = callback;
    renderData = data;
}

- (void) idleFunction:(void (*)(glow *gl, int winId, void *data))callback data:(void*)data {
    idleCallback = callback;
    idleData = data;
}

- (int) setTimeout:(void (*)(glow *gl, int wid, int timeoutId, void *data))callback wait:(unsigned int)wait data:(void*)data {
    id tId = [NSNumber numberWithInteger: timerId];
    NSTimer *timeoutTimer = [NSTimer timerWithTimeInterval:(double)wait/1000.0 target:self selector:@selector(timeoutTimerFired:) userInfo:tId repeats:NO];
    [[NSRunLoop currentRunLoop] addTimer:timeoutTimer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:timeoutTimer forMode:NSEventTrackingRunLoopMode];

    timeoutTimers[timerId] = timeoutTimer;
    timeoutCallbacks[timerId] = callback;
    timeoutData[timerId] = data;
    timerId = (timerId + 1) % GLOW_MAX_TIMERS;

    return [tId intValue];
}

- (void) cancelTimeout:(int)timeoutId {
    if (timeoutTimers[timeoutId] != nil) {
        [timeoutTimers[timeoutId] invalidate];
        timeoutTimers[timeoutId] = nil;
    }
}

- (void) resizeFunction:(void (*)(glow *gl, int winId, unsigned int windowW, unsigned int windowH, unsigned int renderW, unsigned int renderH, void *data))callback data:(void*)data {
    resizeCallback = callback;
    resizeData = data;
}

- (void) mouseDownListener:(void (*)(glow *gl, int winId, unsigned short button, int x, int y, void *data))callback data:(void*)data {
    mouseDownCallback = callback;
    mouseDownData = data;
}

- (void) mouseUpListener:(void (*)(glow *gl, int winId, unsigned short button, int x, int y, void *data))callback data:(void*)data {
    mouseUpCallback = callback;
    mouseUpData = data;
}

- (void) mouseMoveListener:(void (*)(glow *gl, int winId, int x, int y, void *data))callback data:(void*)data {
    mouseMoveCallback = callback;
    mouseMoveData = data;
}

- (void) scrollWheelListener:(void (*)(glow *gl, int winId, int dx, int dy, int x, int y, void *data))callback data:(void*)data {
    scrollWheelCallback = callback;
    scrollWheelData = data;
}

- (void) keyDownListener:(void (*)(glow *gl, int winId, unsigned short key, int x, int y, void *data))callback data:(void*)data {
    keyDownCallback = callback;
    keyDownData = data;
}

- (void) keyUpListener:(void (*)(glow *gl, int winId, unsigned short key, int x, int y, void *data))callback data:(void*)data {
    keyUpCallback = callback;
    keyUpData = data;
}

@end
