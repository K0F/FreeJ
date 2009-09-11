//
//  CVFileInput.mm
//  freej
//
//  Created by xant on 2/16/09.
//  Copyright 2009 dyne.org. All rights reserved.
//

#import "CVFileInputController.h"
#import "CIAlphaFade.h"
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

@implementation CVFileInputController : CVLayerController

- (id)init
{
    isPlaying = NO;
        qtVisualContext = nil;
    return [super init];
}

- (void)dealloc
{
    if (qtMovie)
        [qtMovie release];
    if(qtVisualContext)
        QTVisualContextRelease(qtVisualContext);
    [super dealloc];
}

- (void)setupPixelBuffer
{
    OSStatus err;
    
    Context *ctx = [freej getContext];

    /* Create QT Visual context */
    
    // Pixel Buffer attributes
    CFMutableDictionaryRef pixelBufferOptions = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                                          &kCFTypeDictionaryKeyCallBacks,
                                                                          &kCFTypeDictionaryValueCallBacks);
    
    // the pixel format we want (freej require BGRA pixel format)
    SetNumberValue(pixelBufferOptions, kCVPixelBufferPixelFormatTypeKey, k32ARGBPixelFormat);
    
    // size
    SetNumberValue(pixelBufferOptions, kCVPixelBufferWidthKey, ctx->screen->geo.w);
    SetNumberValue(pixelBufferOptions, kCVPixelBufferHeightKey, ctx->screen->geo.h);
    
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
    CFRelease(pixelBufferOptions);
        err = QTOpenGLTextureContextCreate(kCFAllocatorDefault, glContext,
                    CGLGetPixelFormat(glContext), visualContextOptions, &qtVisualContext);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CFRelease(visualContextOptions);
    CGColorSpaceRelease(colorSpace);    
}

- (void)unloadMovie
{
    [lock lock];

    QTVisualContextTask(qtVisualContext);
    [qtMovie stop];
    [qtMovie release];
    qtMovie = NULL;

    [layerView clear];
    /* TODO - try a way to safely reset currentFrame and lastFrame 
     * (note that other threads can be trying to access them) 
     
    if (lastFrame)
        [lastFrame release];
    lastFrame = NULL;
    if (currentFrame)
        CVPixelBufferRelease(currentFrame);
    
     */

    [lock unlock];
}

- (BOOL)setQTMovie:(QTMovie*)inMovie
{    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    //Context *ctx = (Context *)[freej getContext];

    // no movie has been supplied... perhaps we are going to exit
    if (!inMovie)
        return NO;
    
    // if we own already a movie let's relase it before trying to open the new one
    if (qtMovie)
        [self unloadMovie];

    [lock lock];

    qtMovie = inMovie;

    if(qtMovie) { // ok the movie is here ... let's start the underlying QTMovie object
        OSStatus error;

        [self setupPixelBuffer];

        [qtMovie retain]; // we are going to need this for a while

        error = SetMovieVisualContext([qtMovie quickTimeMovie], qtVisualContext);
        [qtMovie gotoBeginning];
        //[qtMovie setMuted:YES]; // still no audio?
        
        NSArray *tracks = [qtMovie tracks];
        bool hasVideo = NO;
        for (NSUInteger i = 0; i < [tracks count]; i ++) {
            QTTrack *track = [tracks objectAtIndex:i];
            NSString *type = [track attributeForKey:QTTrackMediaTypeAttribute];
            if (![type isEqualToString:QTMediaTypeVideo]) {
                [track setEnabled:NO];
                DisposeMovieTrack([track quickTimeTrack]);
            } else {
                hasVideo = YES;
            }
        }
        if (!hasVideo) {
            qtMovie = nil;
            [lock unlock];
            return NO;
        }
        
        [qtMovie setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieLoopsAttribute];
        [self togglePlay:nil]; // start playing the movie
        movieDuration = [[[qtMovie movieAttributes] objectForKey:QTMovieDurationAttribute] QTTimeValue];
    
        QTTime posterTime = [qtMovie duration];
        posterTime.timeValue /= 2;
        NSImage *poster = [qtMovie frameImageAtTime:posterTime];

        if (layerView)
            [layerView setPosterImage:poster];
        //[lock unlock];
       
        // register the layer within the freej context
        if (!layer) {
            layer = new CVLayer(self);
            layer->init();
        }
    }

    [lock unlock];
    [pool release];
    return YES;
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
/*
    [qtMovie setCurrentTime:inTime];
    if(CVDisplayLinkIsRunning(displayLink))
        [self togglePlay:nil];
    [self updateCurrentFrame];
    [self display];
*/
}

- (IBAction)setMovieTime:(id)sender
{
    [self setTime:QTTimeFromString([sender stringValue])];
}

- (IBAction)togglePlay:(id)sender
{
    [lock lock];
    isPlaying = isPlaying?NO:YES;
    [lock unlock];
}

- (CVTexture *)getTexture
{
    return [super getTexture];
}

- (BOOL)getFrameForTime:(const CVTimeStamp *)timeStamp
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    CVOpenGLBufferRef newPixelBuffer;
    BOOL rv = NO;
    // we can care ourselves about thread safety when accessing the QTKit api
    [QTMovie enterQTKitOnThread];

    [lock lock];

    if (!qtMovie)
        return NO;
    
    QTTime now = [qtMovie currentTime];
    // TODO - check against real hosttime to skip frames instead of
    // slowing down playback
    now.timeValue+=(now.timeScale/layer->fps.fps);
    QTTime duration = [qtMovie duration];
    if (QTTimeCompare(now, duration) == NSOrderedAscending)
        [qtMovie setCurrentTime:now];
    else 
        [qtMovie gotoBeginning];
    if(qtVisualContext)
    {        
        QTVisualContextCopyImageForTime(qtVisualContext,
        NULL,
        NULL,
        &newPixelBuffer);
      
        // rendering (aka: applying filters) is now done in getTexture()
        // implemented in CVLayerView (our parent)
        
        rv = YES;
        if (currentFrame) 
            CVOpenGLTextureRelease(currentFrame);
        currentFrame = newPixelBuffer;
        newFrame = YES;

    } 
    
    [lock unlock];
    [QTMovie exitQTKitOnThread];   
    MoviesTask([qtMovie quickTimeMovie], 0);
    [pool release];
    return rv;
}


- (CVReturn)renderFrame
{
    NSAutoreleasePool *pool = nil;
    
    CVReturn rv = kCVReturnError;

    pool =[[NSAutoreleasePool alloc] init];

    if(qtMovie && [self getFrameForTime:nil])
    {
       
        // render preview if necessary
        [self renderPreview];
        rv = kCVReturnSuccess;
    } else {
        rv = kCVReturnError;
    }
    if (layer) 
        layer->vbuffer = currentFrame;
    [pool release];
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

//@synthesize qtMovie;

@end
