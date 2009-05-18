//
//  CVFileInput.mm
//  freej
//
//  Created by xant on 2/16/09.
//  Copyright 2009 dyne.org. All rights reserved.
//

#import "CIAlphaFade.h"
#import "CVFileInput.h"
#include <math.h>

/* Utility to set a SInt32 value in a CFDictionary
*/
static OSStatus SetNumberValue(CFMutableDictionaryRef inDict,
                        CFStringRef inKey,
                        SInt32 inValue)
{
    CFNumberRef number;

    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inValue);
    if (NULL == number) return coreFoundationUnknownErr;

    CFDictionarySetValue(inDict, inKey, number);

    CFRelease(number);

    return noErr;
}

static CVReturn renderCallback(CVDisplayLinkRef displayLink, 
                                                const CVTimeStamp *inNow, 
                                                const CVTimeStamp *inOutputTime, 
                                                CVOptionFlags flagsIn, 
                                                CVOptionFlags *flagsOut, 
                                                void *displayLinkContext)
{
    CVReturn ret = [(CVFileInput*)displayLinkContext _renderTime:inOutputTime];
    return ret;
}

@implementation CVFileInput : CVLayerView

- (void)dealloc
{
    if (qtMovie)
        [qtMovie release];
    if(qtVisualContext)
        QTVisualContextRelease(qtVisualContext);
    [super dealloc];
}

/*
- (void) setLayer:(CVLayer *)lay
{
    [super setLayer:lay];
    if (!lay && qtMoview)
        [self setQTMovie:nil];
}
*/

- (void)unloadMovie
{
    NSRect        frame = [self frame];
    [lock lock];
    //delete layer;
    //layer = NULL;
    QTVisualContextTask(qtVisualContext);
    [qtMovie stop];
    [qtMovie release];
    qtMovie = NULL;
    //SetMovieVisualContext([qtMovie quickTimeMovie], NULL);
    //[previewImage release];
    //if (posterImage)
      //  [posterImage release];
    if (lastFrame)
        [lastFrame release];
    lastFrame = NULL;
    if (currentFrame)
        CVPixelBufferRelease(currentFrame);
    
    posterImage = NULL;
    //QTVisualContextRelease(qtVisualContext);
    //qtVisualContext = NULL;
    //CVOpenGLTextureRelease(currentFrame);

    //[QTMovie exitQTKitOnThread];
    [[self openGLContext] makeCurrentContext];    
    // clean the OpenGL context
    glClearColor(0.0, 0.0, 0.0, 0.0);         
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    [lock unlock];
}

- (void)setQTMovie:(QTMovie*)inMovie
{    
    OSStatus                err;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    Context *ctx = (Context *)[freej getContext];

    [lock lock];
    // if we own already a movie let's relase it before trying to open the new one
    if (qtMovie) {
        [self unloadMovie];
    }
    // no movie has been supplied... perhaps we are going to exit
    if (!inMovie) {
        //[lock unlock];
        return;
    }
    qtMovie = inMovie;
    [qtMovie retain]; // we are going to need this for a while
    if (!qtVisualContext)
    {
        /* Create QT Visual context */

        // Pixel Buffer attributes
        CFMutableDictionaryRef pixelBufferOptions = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                       &kCFTypeDictionaryKeyCallBacks,
                                                       &kCFTypeDictionaryValueCallBacks);

        // the pixel format we want (freej require BGRA pixel format
        SetNumberValue(pixelBufferOptions, kCVPixelBufferPixelFormatTypeKey, k32ARGBPixelFormat);

        // size
        SetNumberValue(pixelBufferOptions, kCVPixelBufferWidthKey, ctx->screen->w);
        SetNumberValue(pixelBufferOptions, kCVPixelBufferHeightKey, ctx->screen->h);

        // alignment
        SetNumberValue(pixelBufferOptions, kCVPixelBufferBytesPerRowAlignmentKey, 1);
        // QT Visual Context attributes
        CFMutableDictionaryRef visualContextOptions = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                         &kCFTypeDictionaryKeyCallBacks,
                                                         &kCFTypeDictionaryValueCallBacks);
        // set the pixel buffer attributes for the visual context
        CFDictionarySetValue(visualContextOptions,
                             kQTVisualContextPixelBufferAttributesKey,
                             pixelBufferOptions);

        err = QTOpenGLTextureContextCreate(kCFAllocatorDefault, (CGLContextObj)[[self openGLContext] CGLContextObj],
            (CGLPixelFormatObj)[[NSOpenGLView defaultPixelFormat] CGLPixelFormatObj], visualContextOptions, &qtVisualContext);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        
        CGColorSpaceRelease(colorSpace);
        
    }
    if(qtMovie) { // ok the movie is here ... let's start the underlying QTMovie object
        OSStatus error;
        
        error = SetMovieVisualContext([qtMovie quickTimeMovie], qtVisualContext);
        [qtMovie gotoBeginning];
        [qtMovie setMuted:YES]; // still no audio?
        [qtMovie setIdling:NO];

        movieDuration = [[[qtMovie movieAttributes] objectForKey:QTMovieDurationAttribute] QTTimeValue];
    
        Context *ctx = (Context *)[freej getContext];
        QTTime posterTime = [qtMovie duration];
        posterTime.timeValue /= 2;
        NSImage *poster = [qtMovie frameImageAtTime:posterTime];
        NSData  * tiffData = [poster TIFFRepresentation];
        NSBitmapImageRep * bitmap;
        bitmap = [NSBitmapImageRep imageRepWithData:tiffData];

        CIImage *posterInputImage = [[CIImage alloc] initWithBitmapImageRep:bitmap];
        // scale the frame to fit the preview
        NSAffineTransform *scaleTransform = [NSAffineTransform transform];
        NSRect bounds = [self bounds];
        NSRect frame = [self frame];
        float scaleFactor = frame.size.width/ctx->screen->w;
        [scaleTransform scaleBy:scaleFactor];
    
        [scaleFilter setValue:scaleTransform forKey:@"inputTransform"];
        [scaleFilter setValue:posterInputImage forKey:@"inputImage"];

        posterImage = [scaleFilter valueForKey:@"outputImage"];
        // [posterImage retain];

        CGRect  imageRect = CGRectMake(NSMinX(bounds), NSMinY(bounds),
            NSWidth(bounds), NSHeight(bounds));
        [lock lock];
        [ciContext drawImage:posterImage
            atPoint: imageRect.origin
            fromRect: imageRect];
        [[self openGLContext] makeCurrentContext];
        [[self openGLContext] flushBuffer];
        [lock unlock];
        [posterInputImage release];            
       
        // register the layer within the freej context
        if (!layer) {
            layer = new CVLayer((NSObject *)self);
            layer->init(ctx);
            layer->buffer = (void *)pixelBuffer; // give freej a fake buffer ... that's not going to be used anyway
        }
    }

    [lock unlock];
    [pool release];
    if (!layer->running)
        layer->start();
}

- (QTTime)currentTime
{
    return [qtMovie currentTime];
}

- (QTTime)movieDuration
{
    return movieDuration;
}

- (void)setTime:(QTTime)inTime
{
   // [qtMovie setCurrentTime:inTime];
  //  if(CVDisplayLinkIsRunning(displayLink))
        //[self togglePlay:nil];
    //[self updateCurrentFrame];
   // [self display];
}

- (IBAction)setMovieTime:(id)sender
{
    [self setTime:QTTimeFromString([sender stringValue])];
}

- (IBAction)togglePlay:(id)sender
{
    //[lock lock];
    //if(CVDisplayLinkIsRunning(displayLink))
    //{
    //    CVDisplayLinkStop(displayLink);
     //   [qtMovie stop];
   // } else {
        [qtMovie play];
    //    CVDisplayLinkStart(displayLink);
   // }
    //[lock unlock];
}

- (CVTexture *)getTexture
{
    [self _renderTime:nil];
    return [super getTexture];
}

- (BOOL)getFrameForTime:(const CVTimeStamp *)timeStamp
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    BOOL rv = NO;
    [lock lock];
    if(qtVisualContext && QTVisualContextIsNewImageAvailable(qtVisualContext, NULL))
    {        
        if (currentFrame) 
            CVOpenGLTextureRelease(currentFrame);
        QTVisualContextCopyImageForTime(qtVisualContext,
        NULL,
        NULL,
        &currentFrame);
      
        // rendering (aka: applying filters) is now done in getTexture()
        // implemented in CVLayerView (our parent)
        newFrame = YES;
        rv = YES;
    } 
    [lock unlock];
    /*
    layer->fps.calc();
    layer->fps.delay();
    */
    [pool release];
    return rv;
}


- (CVReturn)_renderTime:(const CVTimeStamp *)timeStamp
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    CVReturn rv = kCVReturnError;

    if(qtMovie && [self getFrameForTime:timeStamp])
    {
       
        // render preview if necessary
        [self renderPreview];
        rv = kCVReturnSuccess;
    } else {
        rv = kCVReturnError;
    }
    [pool release];
    MoviesTask([qtMovie quickTimeMovie], 0);
    return rv;
}

- (void)task
{
    QTVisualContextTask(qtVisualContext);
}

- (bool)stepBackward
{
    [qtMovie stepBackward];
    return true;
}

- (bool)setpForward
{
    [qtMovie stepForward];
    return true;
}

- (IBAction)openFile:(id)sender
{
     func("doOpen");    
     NSOpenPanel *tvarNSOpenPanelObj    = [NSOpenPanel openPanel];
     NSInteger tvarNSInteger    = [tvarNSOpenPanelObj runModalForTypes:nil];
     if(tvarNSInteger == NSOKButton){
         func("openScript we have an OK button");    
     } else if(tvarNSInteger == NSCancelButton) {
         func("openScript we have a Cancel button");
         return;
     } else {
         error("doOpen tvarInt not equal 1 or zero = %3d",tvarNSInteger);
         return;
     } // end if     

     NSString * tvarDirectory = [tvarNSOpenPanelObj directory];
     func("openScript directory = %@",tvarDirectory);

     NSString * tvarFilename = [tvarNSOpenPanelObj filename];
     func("openScript filename = %@",tvarFilename);
 
    if (tvarFilename) {
        //if(CVDisplayLinkIsRunning(displayLink)) 
        //    [self togglePlay:nil];
    
        QTMovie *movie = [[QTMovie alloc] initWithFile:tvarFilename error:nil];
        [self setQTMovie:movie];
        [movie setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieLoopsAttribute];
        //[movie gotoBeginning];
        [self togglePlay:nil];

    }
}

@end
