//
//  CVideoFile.mm
//  freej
//
//  Created by xant on 2/16/09.
//  Copyright 2009 dyne.org. All rights reserved.
//

#import "CIAlphaFade.h"
#import "CVideoFile.h"
#import <QTKit/QTMovie.h>
#include <math.h>

#define _ARGB2BGRA(__buf, __size) \
	{\
		long *__bgra = (long *)__buf;\
		for (int __i = 0; __i < __size; __i++)\
			__bgra[__i] = ntohl(__bgra[__i]);\
	}
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
	//if (inNow->hostTime
    return [(CVideoFileInput*)displayLinkContext _renderTime:inOutputTime];
}

@implementation CVideoFileInput : NSOpenGLView

- (void)windowChangedScreen:(NSNotification*)inNotification
{
    NSWindow *window = [inNotification object]; 
    CGDirectDisplayID displayID = (CGDirectDisplayID)[[[[window screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue];

    if(displayID && (viewDisplayID != displayID))
    {
		CVDisplayLinkSetCurrentCGDisplay(displayLink, displayID);
		viewDisplayID = displayID;
    }
}

- (void)prepareOpenGL
{
	CVReturn			    ret;
	NSAutoreleasePool *pool;
	pool = [[NSAutoreleasePool alloc] init];
	
	// Create CGColorSpaceRef 
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		
	// Create CIContext 
	ciContext = [[CIContext contextWithCGLContext:(CGLContextObj)[[self openGLContext] CGLContextObj]
			    pixelFormat:(CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj]
			    options:[NSDictionary dictionaryWithObjectsAndKeys:
				(id)colorSpace,kCIContextOutputColorSpace,
				(id)colorSpace,kCIContextWorkingColorSpace,nil]] retain];
	CGColorSpaceRelease(colorSpace);
	
	// Create CIFilters used for both preview and main frame
	colorCorrectionFilter = [[CIFilter filterWithName:@"CIColorControls"] retain];	    // Color filter	
	[colorCorrectionFilter setDefaults];						    // set the filter to its default values
	exposureAdjustFilter = [[CIFilter filterWithName:@"CIExposureAdjust"] retain];
	[exposureAdjustFilter setDefaults];
	// adjust exposure
	[exposureAdjustFilter setValue:[NSNumber numberWithFloat:0.0] forKey:@"inputEV"];
	
	// rotate
	NSAffineTransform *rotateTransform = [NSAffineTransform transform];
	[rotateTransform rotateByDegrees:0.0];
	rotateFilter = [[CIFilter filterWithName:@"CIAffineTransform"] retain];
	[rotateFilter setValue:rotateTransform forKey:@"inputTransform"];
	//translateFilter = [[CIFilter filterWithName:@"CIAffineTransform"] retain];
	//[translateFilter setValue:translateTransform forKey:@"inputTransform"];
	scaleFilter = [[CIFilter filterWithName:@"CIAffineTransform"] retain];
	//CIFilter *scaleFilter = [CIFilter filterWithName:@"CILanczosScaleTransform"];
	[scaleFilter setDefaults];    // set the filter to its default values
	//[scaleFilter setValue:[NSNumber numberWithFloat:scaleFactor] forKey:@"inputScale"];
	
	effectFilter = [[CIFilter filterWithName:@"CIZoomBlur"] retain];		    // Effect filter	
	[effectFilter setDefaults];							    // set the filter to its default values
	[effectFilter setValue:[NSNumber numberWithFloat:0.0] forKey:@"inputAmount"]; // don't apply effects at startup
	compositeFilter = [[CIFilter filterWithName:@"CISourceOverCompositing"] retain];    // Composite filter
	[CIAlphaFade class];	
	alphaFilter = [[CIFilter filterWithName:@"CIAlphaFade"] retain]; // AlphaFade filter
	[alphaFilter setDefaults]; // XXX - setDefaults doesn't work properly
	[alphaFilter setValue:[NSNumber numberWithFloat:0.5] forKey:@"outputOpacity"]; // set default value
	  	    		
	// Create display link 
	CGOpenGLDisplayMask	totalDisplayMask = 0;
	int			virtualScreen;
	GLint		displayMask;
	NSOpenGLPixelFormat	*openGLPixelFormat = [self pixelFormat];
	viewDisplayID = (CGDirectDisplayID)[[[[[self window] screen] deviceDescription] objectForKey:@"NSScreenNumber"] intValue];  // we start with our view on the main display
	// build up list of displays from OpenGL's pixel format
	for (virtualScreen = 0; virtualScreen < [openGLPixelFormat  numberOfVirtualScreens]; virtualScreen++)
	{
		[openGLPixelFormat getValues:&displayMask forAttribute:NSOpenGLPFAScreenMask forVirtualScreen:virtualScreen];
		totalDisplayMask |= displayMask;
	}
	//ret = CVDisplayLinkCreateWithOpenGLDisplayMask(totalDisplayMask, &displayLink);
	ret = CVDisplayLinkCreateWithCGDisplay(viewDisplayID, &displayLink);
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowChangedScreen:) name:NSWindowDidMoveNotification object:nil];
	// Set up display link callbacks 
	CVDisplayLinkSetOutputCallback(displayLink, renderCallback, self);
		
	// Setup the timecode overlay
	/*
	NSDictionary *fontAttributes = [[NSDictionary alloc] initWithObjectsAndKeys:[NSFont labelFontOfSize:24.0f], NSFontAttributeName,
		[NSColor colorWithCalibratedRed:1.0f green:0.2f blue:0.2f alpha:0.60f], NSForegroundColorAttributeName,
		nil];
	*/
	//timeCodeOverlay = [[TimeCodeOverlay alloc] initWithAttributes:fontAttributes targetSize:NSMakeSize(720.0,480.0 / 4.0)];	// text overlay will go in the bottom quarter of the display
	
	[pool release];
}

- (void)awakeFromNib
{
	[self init];
}

- (id)init
{
	[self setNeedsDisplay:NO];
	needsReshape = YES;
	//freejFrame = NULL;
	doFilters = true;
	currentFrame = NULL;
	lastFrame = NULL;
	renderedImage = NULL;
	filterPanel = NULL;
	if (CVOpenGLBufferCreate (NULL, 400, 300, NULL, &pixelBuffer) != noErr) {
		// TODO - error messages
		pixelBuffer = NULL;
	}
	lock = [[NSRecursiveLock alloc] init];
	[lock retain];
	
	return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (qtMovie)
		[qtMovie release];
    [colorCorrectionFilter release];
    [effectFilter release];
    [compositeFilter release];
	[alphaFilter release];
	[exposureAdjustFilter release];
	[rotateFilter release];
	[scaleFilter release];
	//[translateFilter release];
    ///[timeCodeOverlay release];
    CVOpenGLTextureRelease(currentFrame);
    if(qtVisualContext)
		QTVisualContextRelease(qtVisualContext);
	[ciContext release];
	if (cifjContext)
		[cifjContext release];
	if (pixelBuffer)
		CVOpenGLBufferRelease(pixelBuffer);
	[lock release];
    [super dealloc];
}

- (void)update
{
	//[lock lock];
	[super update];
	//[lock unlock];
}

- (void)drawRect:(NSRect)theRect
{
    NSRect		frame = [self frame];
    NSRect		bounds = [self bounds];
	[lock lock];
	[[self openGLContext] makeCurrentContext];
	
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
		
		
		needsReshape = NO;
	}
	
	// clean the OpenGL context - not so important here but very important when you deal with transparency
	glClearColor(0.0, 0.0, 0.0, 0.0);	     
	glClear(GL_COLOR_BUFFER_BIT);
	// and do preview
	CGRect imageRect = [previewImage extent];
	[ciContext drawImage:previewImage
			atPoint:CGPointMake((int)((frame.size.width - imageRect.size.width) * 0.5), (int)((frame.size.height - imageRect.size.height) * 0.5))
			fromRect:imageRect];
	// flush our output to the screen - this will render with the next beamsync
	glFlush();
	[lock unlock];
}

- (void)unloadMovie
{
	NSRect		frame = [self frame]; 
	[lock lock];
	if(CVDisplayLinkIsRunning(displayLink))
		[self togglePlay:nil];
	
	[[self openGLContext] makeCurrentContext];	
	// clean the OpenGL context
	glClearColor(0.0, 0.0, 0.0, 0.0);	     
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	
	SetMovieVisualContext([qtMovie quickTimeMovie], NULL);
	[qtMovie release];
	[cifjContext release];
	cifjContext = NULL;
	qtMovie = NULL;
	QTVisualContextRelease(qtVisualContext);
	qtVisualContext = NULL;
	needsReshape = YES;
	//[QTMovie exitQTKitOnThread];
	[lock unlock];
}

- (void)setQTMovie:(QTMovie*)inMovie
{	
	OSStatus			    err;
	Context *ctx = (Context *)[freej getContext];
	// if we own already a movie let's relase it before trying to open the new one
	if (qtMovie) 
		[self unloadMovie];
		
	// no movie has been supplied... perhaps we are going to exit
	if (!inMovie)
		return;
		
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
		err = QTPixelBufferContextCreate(kCFAllocatorDefault,
                                     visualContextOptions, &qtVisualContext);
			
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		
		// Create CIContext to render full-frame images
		cifjContext = [[CIContext contextWithCGContext:(CGContextRef)qtVisualContext options:[NSDictionary dictionaryWithObjectsAndKeys:
				(id)colorSpace,kCIContextOutputColorSpace,
				(id)colorSpace,kCIContextWorkingColorSpace,nil]] retain];
		CGColorSpaceRelease(colorSpace);
		
	}
    if(qtMovie) { // ok the movie is here ... let's start the underlying QTMovie object
		OSStatus error;
		
		// create a slave-movie used to render preview images (quicktime is so much faster 
		// to render scaled images that it doesn't make sense to scale them ourselves
		//QTTime t0 = { 0, 0, 0 };
		//QTTimeRange mDuration = { t0, [qtMovie duration] };
	
		error = SetMovieVisualContext([qtMovie quickTimeMovie], qtVisualContext);
		SetMoviePlayHints([qtMovie quickTimeMovie],hintsHighQuality, hintsHighQuality);	
		[qtMovie gotoBeginning];
		[qtMovie setMuted:YES]; // still no audio?
		movieDuration = [[[qtMovie movieAttributes] objectForKey:QTMovieDurationAttribute] QTTimeValue];
		// register the layer within the freej context
		layer = new CVLayer((NSObject *)self);
		layer->init(ctx);
		
		layer->buffer = (void *)pixelBuffer; // give freej a fake buffer ... that's not going to be used anyway
		//layer->start();
    }
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
    [qtMovie setCurrentTime:inTime];
    if(CVDisplayLinkIsRunning(displayLink))
		[self togglePlay:nil];
    [self updateCurrentFrame];
    [self display];
}

- (IBAction)setMovieTime:(id)sender
{
    [self setTime:QTTimeFromString([sender stringValue])];
}

- (IBAction)togglePlay:(id)sender
{
    if(CVDisplayLinkIsRunning(displayLink))
    {
		CVDisplayLinkStop(displayLink);
		[qtMovie stop];
    } else {
		[qtMovie play];
		CVDisplayLinkStart(displayLink);
    }
}

- (IBAction)setFilterParameter:(id)sender
{
	NSAutoreleasePool *pool;
	float deg = 0;
	float x = 0;
	float y = 0;
	NSAffineTransform		*rotateTransform;
	NSAffineTransform		*translateTransform;
	pool = [[NSAutoreleasePool alloc] init];
    [lock lock];
	switch([sender tag])
    {
	case 0:  // opacity (AlphaFade)
		[alphaFilter setValue:[NSNumber numberWithFloat:[sender floatValue]] forKey:@"outputOpacity"];
		break;
	case 1: //brightness (ColorCorrection)
	    [colorCorrectionFilter setValue:[NSNumber numberWithFloat:[sender floatValue]] forKey:@"inputBrightness"];
	    break;
	case 2: // saturation (ColorCorrection)
	    [colorCorrectionFilter setValue:[NSNumber numberWithFloat:[sender floatValue]] forKey:@"inputSaturation"];
	    break;
	case 3: // contrast (ColorCorrection)
	    [colorCorrectionFilter setValue:[NSNumber numberWithFloat:[sender floatValue]] forKey:@"inputContrast"];
	    break;
	case 4: // exposure (ExposureAdjust)
		[exposureAdjustFilter setValue:[NSNumber numberWithFloat:[sender floatValue]] forKey:@"inputEV"];
		break;
	case 5: // rotate and translate
		rotateTransform = [NSAffineTransform transform];
		[rotateTransform rotateByDegrees:[sender floatValue]];
		 deg = ([sender floatValue]*M_PI)/180.0;
		if (deg) {
			 x = ((layer->geo.w)-((layer->geo.w)*cos(deg)-(layer->geo.h)*sin(deg)))/2;
			 y = ((layer->geo.h)-((layer->geo.w)*sin(deg)+(layer->geo.h)*cos(deg)))/2;
		}
		translateTransform = [NSAffineTransform transform];
		[translateTransform translateXBy:x yBy:y];
		[rotateTransform appendTransform:translateTransform];
		[rotateTransform concat];
		[translateTransform concat];
		[rotateFilter setValue:rotateTransform forKey:@"inputTransform"];
		
		break;
	case 6:
		NSString *filterName = [[NSString alloc] initWithFormat:@"CI%@", [[sender selectedItem] title]];
		NSLog(filterName);
		
		break;
	case 7:
	    [effectFilter setValue:[NSNumber numberWithFloat:[sender floatValue]] forKey:@"inputAmount"];
	    break;
	default:
	    break;
    }
    [lock unlock];
	[pool release];
}

- (IBAction)setBlendMode:(id)sender
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [lock lock];
	switch([sender tag])
    {
		case 0:
			NSString *blendMode = [[NSString alloc] initWithFormat:@"CI%@BlendMode", [[sender selectedItem] title]];
			layer->blendMode = blendMode;
			break;
		default:
			break;
	}
	[lock unlock];
	[pool release];
}

- (void)setFilterCenterFromMouseLocation:(NSPoint)where
{
    CIVector	*centerVector = nil;
    
    [lock lock];
    
	centerVector = [CIVector vectorWithX:where.x Y:where.y];
    [effectFilter setValue:centerVector forKey:@"inputCenter"];
    
	[lock unlock];
    if(!CVDisplayLinkIsRunning(displayLink))
		[self display];
}

- (void)renderCurrentFrame
{
	NSAutoreleasePool *pool;
	pool = [[NSAutoreleasePool alloc] init];
    NSRect		frame = [self frame];
    NSRect		bounds = [self bounds];
	float		scaleFactor;
	
	//[QTMovie enterQTKitOnThread];
    if(currentFrame)
    {
		Context *ctx = (Context *)[freej getContext];
		// scale the frame to fit the preview
		CIImage	    *previewInputImage = [CIImage imageWithCVImageBuffer:currentFrame];//, *scaledImage;
		NSAffineTransform *scaleTransform = [NSAffineTransform transform];
		scaleFactor = frame.size.width/ctx->screen->w;
		[scaleTransform scaleBy:scaleFactor];
	
		[scaleFilter setValue:scaleTransform forKey:@"inputTransform"];
		[scaleFilter setValue:previewInputImage forKey:@"inputImage"];
		if (previewImage) 
			[previewImage release];
		previewImage = [scaleFilter valueForKey:@"outputImage"];
		
		// update timecode overlay
		//timecodeImage = [timeCodeOverlay getImageForTime:[self currentTime]];
		if (doFilters) {
			// preview
			[colorCorrectionFilter setValue:previewImage forKey:@"inputImage"];
			[exposureAdjustFilter setValue:[colorCorrectionFilter valueForKey:@"outputImage"] forKey:@"inputImage"];
			[effectFilter setValue:[exposureAdjustFilter valueForKey:@"outputImage"] forKey:@"inputImage"];
			[rotateFilter setValue:[effectFilter valueForKey:@"outputImage"] forKey:@"inputImage"];
			//[translateFilter setValue:[rotateFilter valueForKey:@"outputImage"] forKey:@"inputImage"];
			previewImage = [rotateFilter valueForKey:@"outputImage"];
			
		} 
		[previewImage retain];
	
	}
	QTVisualContextTask(qtVisualContext);
	//[QTMovie exitQTKitOnThread];
	[pool release];
}

- (BOOL)getFrameForTime:(const CVTimeStamp *)timeStamp
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    OSStatus err = noErr;
	BOOL rv = NO;
	//[QTMovie enterQTKitOnThread];
	[lock lock];
    // See if a new frame is available
    if(QTVisualContextIsNewImageAvailable(qtVisualContext,timeStamp))
    {	    
		CVPixelBufferRelease(currentFrame);
		QTVisualContextCopyImageForTime(qtVisualContext,
			NULL,
			timeStamp,
			&currentFrame);
		
		//[delegate performSelectorOnMainThread:@selector(movieTimeChanged:) withObject:self waitUntilDone:NO];
	    rv = YES;
		newFrame = YES; // announce that we have a new frame
		[self renderCurrentFrame]; 
    }
	[lock unlock];
	//[QTMovie exitQTKitOnThread];
	[pool release];
    return rv;
}

- (void)updateCurrentFrame
{
    [self getFrameForTime:nil];    
}

- (CVReturn)_renderTime:(const CVTimeStamp *)timeStamp
{
    CVReturn rv = kCVReturnError;
    NSAutoreleasePool *pool;
	pool = [[NSAutoreleasePool alloc] init];
    if([self getFrameForTime:timeStamp])
    {
		// make sure we have a frame to render    
		// render the frame
		//[lock lock];
		[self drawRect:NSZeroRect]; // refresh the whole view
		//[lock unlock];
		rv = kCVReturnSuccess;
    } else {
		rv = kCVReturnError;
    }
    [pool release];
    return rv;
}

- (void *)grabFrame
{
	return (void *)[self getTexture];
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

- (IBAction)setAlpha:(id)sender
{
	if (layer) {
		//layer->set_blit("alpha");
		//layer->set_([sender floatValue]);
	}
}

- (void) setLayer:(CVLayer *)lay
{
	layer = lay;
	if (lay) {
		// set alpha
		[self setAlpha:alphaBar];
	} else {
		[self setQTMovie:nil];
	}
}

- (IBAction)openFile:(id)sender
{
     func("doOpen");	
     NSOpenPanel *tvarNSOpenPanelObj	= [NSOpenPanel openPanel];
     NSInteger tvarNSInteger	= [tvarNSOpenPanelObj runModalForTypes:nil];
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
 
	QTMovie *movie = [QTMovie movieWithFile:tvarFilename error:nil];
	[self setQTMovie:movie];
	[movie setAttribute:[NSNumber numberWithBool:YES] forKey:QTMovieLoopsAttribute];
	[movie gotoBeginning];
	[self togglePlay:nil];

	if (layer)
		layer->activate();
}

- (IBAction)toggleFilters:(id)sender
{
	doFilters = doFilters?false:true;
}

- (CIImage *)getTexture
{
	NSAutoreleasePool *pool;
	pool = [[NSAutoreleasePool alloc] init];
	CIImage     *inputImage = NULL;
	MoviesTask([qtMovie quickTimeMovie] , 0);
	[lock lock];
	
	if (newFrame) {
		if (lastFrame) {
			CVPixelBufferRelease(lastFrame);
			[renderedImage release];
		}
		
		lastFrame = currentFrame;
		CVPixelBufferRetain(lastFrame);
		inputImage = [CIImage imageWithCVImageBuffer:currentFrame];
		newFrame = NO;
	}
	if (inputImage) {
		if (doFilters) {
			[colorCorrectionFilter setValue:inputImage forKey:@"inputImage"];
			[exposureAdjustFilter setValue:[colorCorrectionFilter valueForKey:@"outputImage"] forKey:@"inputImage"];
			[effectFilter setValue:[exposureAdjustFilter valueForKey:@"outputImage"] 
						  forKey:@"inputImage"];
			[alphaFilter  setValue:[effectFilter valueForKey:@"outputImage"]
						  forKey:@"inputImage"];
		    [rotateFilter setValue:[alphaFilter valueForKey:@"outputImage"] forKey:@"inputImage"];
			//[translateFilter setValue:[rotateFilter valueForKey:@"outputImage"] forKey:@"inputImage"];
			renderedImage = [rotateFilter valueForKey:@"outputImage"];
			//CVPixelBufferLockBaseAddress(freejFrame, 0);
							
			renderedImage = [renderedImage retain];
		} else {
			renderedImage = [inputImage retain];
		}
	}
	[lock unlock];

	// housekeeping on the visual context
	[pool release];
	return renderedImage;
}

- (void)mouseDown:(NSEvent *)theEvent {
	if (!filterPanel) {
		filterPanel = [[CVFilterPanel alloc] init];
	}
	[filterPanel setLayer:self];
	[filterPanel show];
	[super mouseDown:theEvent];
}

@synthesize layer;
@end
