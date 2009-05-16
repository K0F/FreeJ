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
	
    return [(CVScreenView*)displayLinkContext outputFrame:inOutputTime->hostTime];
	
	return kCVReturnSuccess;
	
}


@implementation CVScreenView : NSOpenGLView
- (double)rate
{
    if (rateCalc) 
        return [rateCalc rate];
    return 0;
}

- (void)windowChangedScreen:(NSNotification*)inNotification
{
    NSWindow *window = [inNotification object]; 
    CGDirectDisplayID displayID = (CGDirectDisplayID)[[[[window screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue];

	if(displayID && (viewDisplayID != displayID))
    {
		CVDisplayLinkSetCurrentCGDisplay(displayLink, displayID);
		/*
		CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, 
			(CGLContextObj)[[self openGLContext] CGLContextObj], 
			(CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj]);
		*/
		viewDisplayID = displayID;
    }
}

- (void)windowChangedSize:(NSNotification*)inNotification
{
//	NSRect frame = [self frame];
//	[self setSizeWidth:frame.size.width Height:frame.size.height];	
}

- (void)awakeFromNib
{
	[self init];
}

- (void)start
{

	Context *ctx = [freej getContext];
	CVScreen *screen = (CVScreen *)ctx->screen;
	screen->set_view(self);
    ctx->fps->set(25);
	CVDisplayLinkStart(displayLink);
}

- (id)init
{
	needsReshape = YES;
	//lock = [freej getLock];
    outFrame = NULL;
    lastFrame = NULL;
	lock = [[NSRecursiveLock alloc] init];
	[lock retain];
	[self setNeedsDisplay:NO];
	//cafudding = NO;
	[freej start];
	Context *ctx = (Context *)[freej getContext];
	fjScreen = (CVScreen *)ctx->screen;
	//CVPixelBufferCreate(NULL, 640, 480, k32ARGBPixelFormat , NULL, &pixelBuffer);
	CVReturn err = CVOpenGLBufferCreate (NULL, ctx->screen->w, ctx->screen->h, NULL, &pixelBuffer);
	if (err) {
		// TODO - Error messages
		return nil;
	}
	CVPixelBufferRetain(pixelBuffer);
	return self;
}

- (void)update
{
	//[lock lock];
	[super update];
	//[lock unlock];
}

- (void)dealloc
{
	CVOpenGLTextureRelease(pixelBuffer);
	[ciContext release];
	[currentContext release];
	[outFrame release];
	[lastFrame release];
	[lock release];
	[super dealloc];
}
- (void)prepareOpenGL
{
	CVReturn			    ret;
	// Create display link 
	CGOpenGLDisplayMask	totalDisplayMask = 0;
	int			virtualScreen;
	GLint		displayMask;
	NSOpenGLPixelFormat	*openGLPixelFormat = [self pixelFormat];
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

	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowChangedScreen:) name:NSWindowDidMoveNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowChangedSize:) name:NSWindowDidResizeNotification object:nil];

    rateCalc = [[FrameRate alloc] init];
	[rateCalc retain];
    
	// Set up display link callbacks 
	CVDisplayLinkSetOutputCallback(displayLink, renderCallback, self);

	// start asking for frames
	[self start];
}

- (void)drawRect:(NSRect)theRect
{
    NSRect		frame = [self frame];
    NSRect		bounds = [self bounds];
	//[[freej getLock] lock];
	[lock lock];
	[currentContext makeCurrentContext];

	if(needsReshape)	// if the view has been resized, reset the OpenGL coordinate system
	{
		GLfloat 	minX, minY, maxX, maxY;

		minX = NSMinX(bounds);
		minY = NSMinY(bounds);
		maxX = NSMaxX(bounds);
		maxY = NSMaxY(bounds);

		[self update]; 

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
	//glFlush();
	[[self openGLContext] flushBuffer];
	[self setNeedsDisplay:NO];		
	//[[freej getLock] unlock];
	[lock unlock];
}

- (void)renderFrame
{
	NSAutoreleasePool *pool;
	pool = [[NSAutoreleasePool alloc] init];
	NSRect		frame = [self frame];
    NSRect		bounds = [self bounds];    

    //[lock lock];
	if (outFrame) {
		CGRect  cg = CGRectMake(NSMinX(bounds), NSMinY(bounds),
					NSWidth(bounds), NSHeight(bounds));
		[ciContext drawImage: outFrame
			atPoint: cg.origin  fromRect: cg];
	}
	//[self setNeedsDisplay:YES];
	[self drawRect:NSZeroRect];
	//[lock unlock];
	[pool release];
}

- (void *)getSurface
{		
	return (void *)CVPixelBufferGetBaseAddress(pixelBuffer);			
}

- (CVReturn)outputFrame:(uint64_t)timestamp
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if (outFrame) {
		[outFrame release];
		outFrame = NULL;
	}
	Context *ctx = (Context *)[freej getContext];
	//[lock lock];
	ctx->cafudda(0.0);
	//[lock unlock];
	// export
//	if (outFrame) {
//		NSCIImageRep* rep = [NSCIImageRep imageRepWithCIImage:outFrame];
//		NSImage* image = [[[NSImage alloc] initWithSize:NSMakeSize (fjScreen->w, fjScreen->h)] autorelease];
//		[image addRepresentation:rep];
//	}
    if (rateCalc) {
        [rateCalc tick:timestamp];
        if (fpsString)
            [fpsString release];
        fpsString = [[NSString stringWithFormat:@"%0.1lf", [rateCalc rate]] retain];
        [showFps setStringValue:fpsString];
    }
	[pool release];
	return kCVReturnSuccess;
}

- (void)drawLayer:(Layer *)layer
{
	//NSAutoreleasePool *pool;
	CIImage *inputImage = NULL;
	CIFilter *blendFilter = NULL;
	//pool = [[NSAutoreleasePool alloc] init];
	//[lock lock];
	if (layer->type == Layer::GL_COCOA) {
		CVLayer *cvLayer = (CVLayer *)layer;
		inputImage = cvLayer->gl_texture();
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
		inputImage = [[CIImage imageWithCVImageBuffer:pixelBufferOut] retain];
		CVPixelBufferRelease(pixelBufferOut);
		blendFilter = [CIFilter filterWithName:@"CIOverlayBlendMode"];
	}
	[blendFilter setDefaults];
	if (inputImage) {
		if (!outFrame) {
			outFrame = inputImage;
		} else {
			[blendFilter setValue:outFrame forKey:@"inputBackgroundImage"];
			[blendFilter setValue:inputImage forKey:@"inputImage"];
			[outFrame release];
			outFrame = [blendFilter valueForKey:@"outputImage"];
		}
		[outFrame retain];
        [inputImage release];
	}
	//[lock unlock];
	//[pool release];
}

- (void)setSizeWidth:(int)w Height:(int)h
{
	[lock lock];
	if (w != fjScreen->w || h != fjScreen->h) {

			CVPixelBufferRelease(pixelBuffer);
			CVReturn err = CVOpenGLBufferCreate (NULL, fjScreen->w, fjScreen->h, NULL, &pixelBuffer);
			if (err != noErr) {
				// TODO - Error Messages
			}
			CVPixelBufferRetain(pixelBuffer);
			fjScreen->w = w;
			fjScreen->h = h;
			needsReshape = YES;

	}
	[lock unlock];
}

- (IBAction)toggleFullScreen:(id)sender
{
	if (fullScreen) {
	
		CGDisplayReservationInterval seconds = 2.0;
		CGDisplayFadeReservationToken newToken;
		CGAcquireDisplayFadeReservation(seconds, &newToken); // reserve display hardware time

		CGDisplayFade(newToken,
		0.3,	 // 0.3 seconds
		kCGDisplayBlendNormal,	// Starting state
		kCGDisplayBlendSolidColor, // Ending state
		0.0, 0.0, 0.0,	 // black
		true);	 // wait for completion

		[currentContext clearDrawable];
		
		CGDisplaySwitchToMode(viewDisplayID, savedMode);

		CGDisplayFade(newToken,
		0.2,	 // 0.2 seconds
		kCGDisplayBlendSolidColor, // Starting state
		kCGDisplayBlendNormal,	// Ending state
		0.0, 0.0, 0.0,	 // black
		true);	 // Don't wait for completion

		CGReleaseDisplayFadeReservation(newToken);
		CGDisplayRelease(viewDisplayID);
		// notify our underlying NSView object that we are exiting fullscreen
		[self exitFullScreenModeWithOptions:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:0], 
			NSFullScreenModeAllScreens, nil ]];
		fullScreen = NO;
	} else {
		CFDictionaryRef newMode = CGDisplayBestModeForParameters(viewDisplayID, 32, fjScreen->w, fjScreen->h, 0);
		NSAssert(newMode, @"Couldn't find display mode");

		savedMode = CGDisplayCurrentMode(viewDisplayID);

		//fade out
		CGDisplayReservationInterval seconds = 2.0;
		CGDisplayFadeReservationToken newToken;
		CGAcquireDisplayFadeReservation(seconds, &newToken); // reserve display hardware time

		CGDisplayFade(newToken,
		0.2,	 // 0.3 seconds
		kCGDisplayBlendNormal,	// Starting state
		kCGDisplayBlendSolidColor, // Ending state
		0.0, 0.0, 0.0,	 // black
		true);	 // wait for completion

		CGDisplayCapture(viewDisplayID);	 //capture main display

		//Switch to selected resolution.
		CGDisplayErr err = CGDisplaySwitchToMode(viewDisplayID, newMode);
		NSAssert(err == CGDisplayNoErr, @"Error switching resolution.");

		[currentContext setFullScreen];	 //set openGL context to draw to screen


		CGDisplayFade(newToken,
		0.5,	 // 0.5 seconds
		kCGDisplayBlendSolidColor, // Starting state
		kCGDisplayBlendNormal,	// Ending state
		0.0, 0.0, 0.0,	 // black
		false);	 // Don't wait for completion

		CGReleaseDisplayFadeReservation(newToken);

		// notify our underlying NSView object that we are going fullscreen
		[self enterFullScreenMode:[[self window] screen] withOptions:
			[NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInt:0], NSFullScreenModeAllScreens, nil ]];
		fullScreen = YES;
	}
	[lock lock];
	//[[freej getLock] lock];
	[currentContext makeCurrentContext];
	// clean the OpenGL context 
	glClearColor(0.0, 0.0, 0.0, 0.0);	     
	glClear(GL_COLOR_BUFFER_BIT);

	[currentContext flushBuffer];
	//[[freej getLock] unlock];
	[lock unlock];
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
	QTMovie *mMovie;
    // Check first if the new QuickTime 7.2.1 initToWritableFile: method is available
    if ([[[[QTMovie alloc] init] autorelease] respondsToSelector:@selector(initToWritableFile:error:)] == YES)
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
    
        OSErr err;
        // create a native QuickTime movie
       // Movie qtMovie = [self quicktimeMovieFromTempFile:&mDataHandlerRef error:&err];
        //if (nil == qtMovie) goto bail;
        
        // instantiate a QTMovie from our native QuickTime movie
        //mMovie = [QTMovie movieWithQuickTimeMovie:qtMovie disposeWhenDone:YES error:nil];
        //if (!mMovie || err) goto bail;
    }


  // mark the movie as editable
  [mMovie setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieEditableAttribute];
  
  // keep it around until we are done with it...
  [mMovie retain];

  // build an array of image file paths (NSString objects) to our image files
  NSArray *imagesArray = [[NSBundle mainBundle] pathsForResourcesOfType:@"jpg" 
                              inDirectory:nil];
  // add all the images to our movie as MPEG-4 frames
  //[mMovie addImage:<#(NSImage *)image#> forDuration:<#(QTTime)duration#> withAttributes:<#(NSDictionary *)attributes#>];

bail:

  return;
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
//   func("method coords(%i,%i) invoked", x, y);
// if you are trying to get a cropped part of the layer
// use the .pitch parameter for a pre-calculated stride
// that is: number of bytes for one full line
  return
    ( x + (w*y) +
      (uint32_t*)get_surface() );
}

void *CVScreen::get_surface() {
  if (view)
	return [view getSurface];
  return NULL;
}

void CVScreen::set_view(CVScreenView *v)
{
	view = v;
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
    if (view) 
        [view renderFrame];
}

