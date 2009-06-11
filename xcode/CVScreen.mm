/*  FreeJ
 *  (c) Copyright 2009 Denis Roio aka jaromil <jaromil@dyne.org>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published 
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <config.h>

#include <stdlib.h>

#include <jutils.h>
#define __cocoa
#import <QuartzCore/CIKernel.h>
#import <QTKit/QTMovie.h>
#import "CFreej.h"
#import "CVLayer.h"
#import "CVScreen.h"
#import "CVTexture.h"

#define _BGRA2ARGB(__buf, __size) \
    {\
        long *__bgra = (long *)__buf;\
        for (int __i = 0; __i < __size; __i++)\
            __bgra[__i] = ntohl(__bgra[__i]);\
    }

static CVReturn renderCallback(CVDisplayLinkRef displayLink, 
                                                const CVTimeStamp *inNow, 
                                                const CVTimeStamp *inOutputTime, 
                                                CVOptionFlags flagsIn, 
                                                CVOptionFlags *flagsOut, 
                                                void *displayLinkContext)
{    
    CVReturn ret = [(CVScreenView*)displayLinkContext outputFrame:inNow->hostTime];
    return ret;
}


@implementation CVScreenView : NSOpenGLView
/*
- (void)windowChangedSize:(NSNotification*)inNotification
{
//    NSRect frame = [self frame];
//    [self setSizeWidth:frame.size.width Height:frame.size.height];    
}
*/

- (void)awakeFromNib
{
    [self init];
}

- (void)start
{

    Context *ctx = [freej getContext];
    CVScreen *screen = (CVScreen *)ctx->screen;
    screen->set_view(self);
    CVDisplayLinkStart(displayLink);
}

- (id)init
{
    needsReshape = YES;
    outFrame = NULL;
    lastFrame = NULL;
    exportedFrame = NULL;
    lock = [[NSRecursiveLock alloc] init];
    [lock retain];
    [self setNeedsDisplay:NO];
    [freej start];
    Context *ctx = (Context *)[freej getContext];
    fjScreen = (CVScreen *)ctx->screen;
    
    CVReturn err = CVPixelBufferCreate (
           NULL,
           fjScreen->w,
           fjScreen->h,
           k32ARGBPixelFormat,
           NULL,
           &pixelBuffer
    );
    if (err) {
        // TODO - Error messages
        return nil;
    }
    CVPixelBufferLockBaseAddress(pixelBuffer, NULL);
    exportBuffer = CVPixelBufferGetBaseAddress(pixelBuffer);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    exportCGContextRef = CGBitmapContextCreate (NULL,
                                     ctx->screen->w,
                                     ctx->screen->h,
                                     8,      // bits per component
                                     ctx->screen->w*4,
                                     colorSpace,
                                     kCGImageAlphaPremultipliedLast);
        
    if (exportCGContextRef == NULL)
        NSLog(@"Context not created!");
    exportContext = [[CIContext contextWithCGContext:exportCGContextRef 
            options:[NSDictionary dictionaryWithObject: (NSString*) kCGColorSpaceGenericRGB 
                forKey:  kCIContextOutputColorSpace]] retain];
    CGColorSpaceRelease( colorSpace );
    return self;
}

- (void)dealloc
{
    CVPixelBufferUnlockBaseAddress(pixelBuffer, NULL);
    CVOpenGLTextureRelease(pixelBuffer);
    [ciContext release];
    [currentContext release];
    if (outFrame)
        [outFrame release];
    if (lastFrame)
        [lastFrame release];
    [lock release];
    [exportContext release];
    CGContextRelease( exportCGContextRef );
    [super dealloc];
}

- (void)prepareOpenGL
{
    CVReturn                ret;

    // Create display link 
    CGOpenGLDisplayMask    totalDisplayMask = 0;
    int            virtualScreen;
    GLint        displayMask;
    NSOpenGLPixelFormat    *openGLPixelFormat = [self pixelFormat];
    // we start with our view on the main display
    // build up list of displays from OpenGL's pixel format
    viewDisplayID = (CGDirectDisplayID)[[[[[self window] screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue];  
    for (virtualScreen = 0; virtualScreen < [openGLPixelFormat  numberOfVirtualScreens]; virtualScreen++)
    {
        [openGLPixelFormat getValues:&displayMask forAttribute:NSOpenGLPFAScreenMask forVirtualScreen:virtualScreen];
        totalDisplayMask |= displayMask;
    }
    ret = CVDisplayLinkCreateWithCGDisplay(viewDisplayID, &displayLink);
    
    currentContext = [[self openGLContext] retain];
    
    // Create CGColorSpaceRef 
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        
    // Create CIContext 
    
    ciContext = [[CIContext contextWithCGLContext:(CGLContextObj)[currentContext CGLContextObj]
                pixelFormat:(CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj]
                options:[NSDictionary dictionaryWithObjectsAndKeys:
                (id)colorSpace,kCIContextOutputColorSpace,
                (id)colorSpace,kCIContextWorkingColorSpace,nil]] retain];
    CGColorSpaceRelease(colorSpace);

    
    //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowChangedSize:) name:NSWindowDidResizeNotification object:nil];

    rateCalc = [[FrameRate alloc] init];
    [rateCalc retain];
    
    GLint params[] = { 1 };
    CGLSetParameter( CGLGetCurrentContext(), kCGLCPSwapInterval, params );
     
    // Set up display link callbacks 
    CVDisplayLinkSetOutputCallback(displayLink, renderCallback, self);
    
    // start asking for frames
    [self start];
}

- (void) update
{
    if( kCGLNoError != CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]) )
        return;
    [super update];
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
}

- (void)drawRect:(NSRect)theRect
{
    NSRect        frame = [self frame];
    NSRect        bounds = [self bounds];    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    if( kCGLNoError != CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]) )
            return;

    [currentContext makeCurrentContext];

    if(needsReshape)    // if the view has been resized, reset the OpenGL coordinate system
    {
        GLfloat     minX, minY, maxX, maxY;

        minX = NSMinX(bounds);
        minY = NSMinY(bounds);
        maxX = NSMaxX(bounds);
        maxY = NSMaxY(bounds);

        //[self update]; 

        if(NSIsEmptyRect([self visibleRect])) 
        {
            glViewport(0, 0, 1, 1);
        } else {
            glViewport(0, 0,  frame.size.width ,frame.size.height);
        }
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(minX, maxX, minY, maxY, -1.0, 1.0);
        
        glDisable(GL_DITHER);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_FOG);
        //glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glPixelZoom(1.0,1.0);
        
        // clean the OpenGL context 
        glClearColor(0.0, 0.0, 0.0, 0.0);         
        glClear(GL_COLOR_BUFFER_BIT);

        needsReshape = NO;
    }
    // flush our output to the screen - this will render with the next beamsync
    [[self openGLContext] flushBuffer];
    //[super drawRect:theRect];
    [self setNeedsDisplay:NO];
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    [pool release];
}

- (void *)getSurface
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [lock lock];
    Context *ctx = (Context *)[freej getContext];
    
    if (lastFrame) {
        NSRect bounds = [self bounds];
        
        CGRect rect = CGRectMake(0,0, ctx->screen->w, ctx->screen->h);
        [exportContext render:lastFrame 
             toBitmap:exportBuffer
             rowBytes:ctx->screen->w*4
               bounds:rect 
               format:kCIFormatARGB8 
           colorSpace:NULL];
    }
    [lock unlock];
    [pool release];
    return exportBuffer;
}

- (CVReturn)outputFrame:(uint64_t)timestamp
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    //CVTexture *textureToRelease = nil;
    NSRect        bounds = [self bounds];

    Context *ctx = (Context *)[freej getContext];

    ctx->cafudda(0.0);

    if (rateCalc) {
        [rateCalc tick:timestamp];
        fpsString = [[NSString alloc] initWithFormat:@"%0.1lf", [rateCalc rate]];
        [fpsString autorelease];
        [showFps setStringValue:fpsString];
    } 

    if( kCGLNoError != CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]) )
        return kCVReturnError;
    
    if (outFrame) {
        CGRect  cg = CGRectMake(NSMinX(bounds), NSMinY(bounds),
                    NSWidth(bounds), NSHeight(bounds));
        [ciContext drawImage: outFrame
            atPoint: cg.origin  fromRect: cg];
        if (lastFrame)
            [lastFrame release];
        lastFrame = outFrame;
        outFrame = NULL;
    } else {
        needsReshape = YES;
    }
    
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);    

    // XXX - we force rendering at this stace instead of delaying it setting the needsDisplay flag
    // to avoid stopping displaying new frames on the output screen while system animations are in progress
    // (for example, when opening a fileselction panel the CVScreen rendering would stop while the animation is in progress
    // because both the actions would happen in the main application thread. Forcing rendering now makes it happen in the 
    // CVScreen thread
    //[self setNeedsDisplay:YES]; // this will delay rendering to be done  the application main thread
    [self drawRect:NSZeroRect]; // this directly render the frame out in this thread

    [pool release];
    return kCVReturnSuccess;
}

- (void)drawLayer:(Layer *)layer
{
    //NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    CIFilter *blendFilter = nil;
    CVTexture *texture = nil;

    if (layer->type == Layer::GL_COCOA) {

        CVLayer *cvLayer = (CVLayer *)layer;
        texture = cvLayer->gl_texture();

        NSString *blendMode = ((CVLayer *)layer)->blendMode;
        if (blendMode)
            blendFilter = [CIFilter filterWithName:blendMode];
        else
            blendFilter = [CIFilter filterWithName:@"CIOverlayBlendMode"]; 
    } else { // freej 'not-cocoa' layer type
    
        CVPixelBufferRef pixelBufferOut;
        //_BGRA2ARGB(layer->buffer, layer->geo.w*layer->geo.h); // XXX - expensive conversion
        CVReturn cvRet = CVPixelBufferCreateWithBytes (
           NULL,
           layer->geo.w,
           layer->geo.h,
           k32ARGBPixelFormat,
           layer->buffer,
           layer->geo.w*4,
           NULL,
           NULL,
           NULL,
           &pixelBufferOut
        );
        if (cvRet != noErr) {
            // TODO - Error Messages
        } 
        CIImage *inputImage = [CIImage imageWithCVImageBuffer:pixelBufferOut];
        texture = [[CVTexture alloc] initWithCIImage:inputImage pixelBuffer:pixelBufferOut];
        //CVPixelBufferRelease(pixelBufferOut);
        blendFilter = [CIFilter filterWithName:@"CIOverlayBlendMode"];
    }
    [blendFilter setDefaults];
    if (texture) {
        if (!outFrame) {
            outFrame = [[texture image] retain];
        } else {
            [blendFilter setValue:outFrame forKey:@"inputBackgroundImage"];
            [blendFilter setValue:[texture image] forKey:@"inputImage"];
            CIImage *temp = [blendFilter valueForKey:@"outputImage"];
            [outFrame autorelease];
            outFrame = [temp retain];
        }
        [texture autorelease];
        //[textures addObject:texture];
    }
}

- (void)setSizeWidth:(int)w Height:(int)h
{
    //[lock lock];
    if (w != fjScreen->w || h != fjScreen->h) {

            CVPixelBufferRelease(pixelBuffer);
            CVReturn err = CVOpenGLBufferCreate (NULL, fjScreen->w, fjScreen->h, NULL, &pixelBuffer);
            if (err != noErr) {
                // TODO - Error Messages
            }
            CVPixelBufferRetain(pixelBuffer);
            //pixelBuffer = realloc(pixelBuffer, w*h*4);
            fjScreen->w = w;
            fjScreen->h = h;
            needsReshape = YES;

    }
    //[lock unlock];
}

- (IBAction)toggleFullScreen:(id)sender
{    
    CGDirectDisplayID currentDisplayID = (CGDirectDisplayID)[[[[[self window] screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue];  

    if (fullScreen) {
        CGDisplaySwitchToMode(currentDisplayID, savedMode);
        SetSystemUIMode(kUIModeNormal, kUIOptionAutoShowMenuBar);
        [self retain];
        NSWindow *fullScreenWindow = [self window];
        [self removeFromSuperview];
        [myWindow setContentView:self];
        [myWindow setFrame:[self frame] display:YES];
        [myWindow release];
        [self release];
        [fullScreenWindow release];
        fullScreen = NO;
        needsReshape = YES;
    } else {
        CFDictionaryRef newMode = CGDisplayBestModeForParameters(currentDisplayID, 32, fjScreen->w, fjScreen->h, 0);
        NSAssert(newMode, @"Couldn't find display mode");
        
        savedMode = CGDisplayCurrentMode(currentDisplayID);
        CGDisplaySwitchToMode(currentDisplayID, newMode);
        
        SetSystemUIMode(kUIModeAllSuppressed, kUIOptionAutoShowMenuBar);
        NSScreen *screen = [[self window] screen];
        NSWindow *window = [[NSWindow alloc] initWithContentRect:[screen frame]
                                styleMask:NSBorderlessWindowMask
                                backing:NSBackingStoreBuffered
                                defer:NO
                                screen:screen];
        myWindow = [[self window] retain];
        [self retain];
        [self removeFromSuperview];
        [window setContentView:self];
        [window setFrameOrigin:[screen frame].origin];
        [self release];
        [window makeKeyAndOrderFront:sender];
        [NSCursor setHiddenUntilMouseMoves:YES];
        fullScreen = YES;
    }
    [self drawRect:NSZeroRect];
}

- (bool)isOpaque
{
    return YES;
}

//
// buildQTKitMovie
//
// Build a QTKit movie from a series of image frames
//
//

-(void)buildQTKitMovie
{

    /*  
      NOTES ABOUT CREATING A NEW ("EMPTY") MOVIE AND ADDING IMAGE FRAMES TO IT

      In order to compose a new movie from a series of image frames with QTKit
      it is of course necessary to first create an "empty" movie to which these
      frames can be added. Actually, the real requirements (in QuickTime terminology)
      for such an "empty" movie are that it contain a writable data reference. A
      movie with a writable data reference can then accept the addition of image 
      frames via the -addImage method.

      Prior to QuickTime 7.2.1, QTKit did not provide a QTMovie method for creating a 
      QTMovie with a writable data reference. In this case, we can use the native 
      QuickTime API CreateMovieStorage() to create a QuickTime movie with a writable 
      data reference (in our example below we use a data reference to a file). We then 
      use the QTKit movieWithQuickTimeMovie: method to instantiate a QTMovie from this 
      native QuickTime movie. 

      Finally, images are added to the movie as movie frames using -addImage.

      NEW IN QUICKTIME 7.2.1

      QuickTime 7.2.1 now provides a new method:

      - (id)initToWritableFile:(NSString *)filename error:(NSError **)errorPtr;

      to create a QTMovie with a writable data reference. This eliminates the need to
      use the native QuickTime API CreateMovieStorage() as described above.

      The code below checks first to see if this new method initToWritableFile: is 
      available, and if so it will use it rather than use the native API.
    */
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    QTMovie *mMovie;
    // Check first if the new QuickTime 7.2.1 initToWritableFile: method is available
    if ([[[[QTMovie alloc] init] retain] respondsToSelector:@selector(initToWritableFile:error:)] == YES)
    {

        // generate a name for our movie file
        NSString *tempName = [NSString stringWithCString:tmpnam(nil) 
                                encoding:[NSString defaultCStringEncoding]];
        if (nil == tempName) goto bail;
        
        // Create a QTMovie with a writable data reference
        mMovie = [[QTMovie alloc] initToWritableFile:tempName error:NULL];
    }
    else    
    {    
        // The QuickTime 7.2.1 initToWritableFile: method is not available, so use the native 
        // QuickTime API CreateMovieStorage() to create a QuickTime movie with a writable 
        // data reference
    
        //OSErr err;
        // create a native QuickTime movie
        //Movie qtMovie = [self quicktimeMovieFromTempFile:&mDataHandlerRef error:&err];
        //if (nil == qtMovie) goto bail;
        
        // instantiate a QTMovie from our native QuickTime movie
        //mMovie = [QTMovie movieWithQuickTimeMovie:qtMovie disposeWhenDone:YES error:nil];
        //if (!mMovie || err) goto bail;
    }


  // mark the movie as editable
  [mMovie setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieEditableAttribute];
  
  // keep it around until we are done with it...
  [mMovie retain];

  [pool release];
    
bail:

  return;
}

- (double)rate
{
    if (rateCalc) 
        return [rateCalc rate];
    return 0;
}

- (void)reset
{
    needsReshape = YES;
    [self drawRect:NSZeroRect];
}

@synthesize fullScreen;

@end

CVScreen::CVScreen()
  : ViewPort() {

  bpp = 32;
  view = NULL;
}

CVScreen::~CVScreen() {
  func("%s",__PRETTY_FUNCTION__);

}


bool CVScreen::init(int w, int h) {

  this->w = w;
  this->h = h;
  bpp = 32;
  size = w*h*(bpp>>3);
  pitch = w*(bpp>>3);


  return true;
}


void *CVScreen::coords(int x, int y) {
    //func("method coords(%i,%i) invoked", x, y);
    // if you are trying to get a cropped part of the layer
    // use the .pitch parameter for a pre-calculated stride
    // that is: number of bytes for one full line
    return ( x + (w*y) + (uint32_t*)get_surface() );
}

void *CVScreen::get_surface() {
  if (view)
    return [view getSurface];
  return NULL;
}

void CVScreen::set_view(CVScreenView *v)
{
    view = v;
    [view setSizeWidth:w Height:h];
}

CVScreenView *CVScreen::get_view(void)
{
    return view;
}

void CVScreen::blit(Layer *lay)
{
    if (view)
        [view drawLayer:lay];
}

void CVScreen::show() {
    //do nothing
    /*
    if (view) 
        [view setNeedsDisplay:YES];
    */
}

