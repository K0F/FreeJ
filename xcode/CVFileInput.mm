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

@implementation CVFileInput : CVLayerView

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
    
    Context *ctx = (Context *)[freej getContext];

    /* Create QT Visual context */
    
    // Pixel Buffer attributes
    CFMutableDictionaryRef pixelBufferOptions = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                                          &kCFTypeDictionaryKeyCallBacks,
                                                                          &kCFTypeDictionaryValueCallBacks);
    
    // the pixel format we want (freej require BGRA pixel format)
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
    CFRelease(pixelBufferOptions);
    err = QTOpenGLTextureContextCreate(kCFAllocatorDefault, (CGLContextObj)[[self openGLContext] CGLContextObj],
                                       (CGLPixelFormatObj)[[NSOpenGLView defaultPixelFormat] CGLPixelFormatObj], visualContextOptions, &qtVisualContext);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CFRelease(visualContextOptions);
    CGColorSpaceRelease(colorSpace);    
}

- (void)unloadMovie
{
    NSRect        frame = [self frame];
    [lock lock];

    QTVisualContextTask(qtVisualContext);
    [qtMovie stop];
    [qtMovie release];
    qtMovie = NULL;

    /* TODO - try a way to safely reset currentFrame and lastFrame 
     * (note that other threads can be trying to access them) 
     
    if (lastFrame)
        [lastFrame release];
    lastFrame = NULL;
    if (currentFrame)
        CVPixelBufferRelease(currentFrame);
    
     */
    
    posterImage = NULL;

    
    if( kCGLNoError != CGLLockContext((CGLContextObj)[[self openGLContext] CGLContextObj]))
        return;
    
    [[self openGLContext] makeCurrentContext];    
    // clean the OpenGL context
    glClearColor(0.0, 0.0, 0.0, 0.0);         
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    
    CGLUnlockContext((CGLContextObj)[[self openGLContext] CGLContextObj]);
    [lock unlock];
}

- (void)setQTMovie:(QTMovie*)inMovie
{    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    //Context *ctx = (Context *)[freej getContext];

    // no movie has been supplied... perhaps we are going to exit
    if (!inMovie)
        return;
    
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
            return;
        }
        
        movieDuration = [[[qtMovie movieAttributes] objectForKey:QTMovieDurationAttribute] QTTimeValue];
    
        Context *ctx = (Context *)[freej getContext];
        QTTime posterTime = [qtMovie duration];
        posterTime.timeValue /= 2;
        NSImage *poster = [qtMovie frameImageAtTime:posterTime];
        NSData  * tiffData = [poster TIFFRepresentation];
        NSBitmapImageRep * bitmap;
        bitmap = [NSBitmapImageRep imageRepWithData:tiffData];
        
        NSRect bounds = [self bounds];
        CIImage *posterInputImage = [[CIImage alloc] initWithBitmapImageRep:bitmap];
        // scale the frame to fit the preview
        NSAffineTransform *scaleTransform = [NSAffineTransform transform];
    
        float scaleFactor = bounds.size.width/[poster size].width;
        [scaleTransform scaleBy:scaleFactor];
    
        [scaleFilter setValue:scaleTransform forKey:@"inputTransform"];
        [scaleFilter setValue:posterInputImage forKey:@"inputImage"];

        posterImage = [scaleFilter valueForKey:@"outputImage"];

        CGRect  imageRect = CGRectMake(NSMinX(bounds), NSMinY(bounds),
            NSWidth(bounds), NSHeight(bounds));
        
        //[lock lock];
        [ciContext drawImage:posterImage
            atPoint: imageRect.origin
            fromRect: imageRect];
        [[self openGLContext] makeCurrentContext];
        [[self openGLContext] flushBuffer];
        //[lock unlock];
        [posterInputImage release];            
       
        // register the layer within the freej context
        if (!layer) {
            layer = new CVLayer(self);
            layer->init(ctx);
        }
    }

    [lock unlock];
    [pool release];
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

- (void)openFilePanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode  contextInfo:(void  *)contextInfo
{
     if(returnCode == NSOKButton){
         func("openFilePanel: OK");    
     } else if(returnCode == NSCancelButton) {
         func("openFilePanel: Cancel");
         return;
     } else {
         error("openFilePanel: Error %3d",returnCode);
         return;
     } // end if     
     NSString * tvarDirectory = [panel directory];
     func("openFile directory = %@",tvarDirectory);

     NSString * tvarFilename = [panel filename];
     func("openFile filename = %@",tvarFilename);
     
    if (tvarFilename) {
        NSDictionary *movieAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:YES], QTMovieOpenAsyncOKAttribute,
            tvarFilename, QTMovieFileNameAttribute,
            [NSNumber numberWithBool:NO] , QTMovieHasAudioAttribute,
            nil];
        QTMovie *movie = [[QTMovie alloc] initWithAttributes:movieAttributes error:nil];
        [movie setIdling:NO];
        [self setQTMovie:movie];
        if (qtMovie) {
            [movie setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieLoopsAttribute];
            [self togglePlay:nil]; // start playing the movie
        } else {
            warning("Can't open file: %s", [tvarFilename UTF8String]);
        }
    }
}

- (IBAction)openFile:(id)sender 
{    
    NSOpenPanel *fileSelectionPanel    = [NSOpenPanel openPanel];
    NSArray *types = [NSArray arrayWithObjects:
        @"avi", @"mov", @"mpg", @"asf", @"jpg", 
        @"png", @"tif", @"bmp", @"gif", @"pdf", nil];
        
    [fileSelectionPanel 
        beginSheetForDirectory:nil 
        file:nil
        types:types 
        modalForWindow:[sender window]
        modalDelegate:self 
        didEndSelector:@selector(openFilePanelDidEnd: returnCode: contextInfo:) 
        contextInfo:nil];    
   [fileSelectionPanel setCanChooseFiles:YES];
} // end openFile


@end
