//
//  CVFilterPanel.m
//  freej
//
//  Created by xant on 2/26/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "CVFilterPanel.h"

#define FILTERS_MAX 12
static FilterParams fParams[FILTERS_MAX] =
{
	{ 1, { { "inputAmount", 0.0, 50.0 } } },  // ZoomBlur
	{ 1, { { "inputRadius", 1.0, 100.0 } } },  // BoxBlur
	//{ 2, { { "inputRadius", 0.0, 50.0 }, { "inputAngle", -3.14, 3.14 } } }, // MotionBlur
	{ 1, { { "inputRadius", 0.0, 50.0 } } }, // DiscBlur
	{ 1, { { "inputRadius", 0.0, 100.0 } } }, // GaussianBlur
	{ 1, { { "inputLevels", 2.0, 30.0 } } }, // ColorPosterize
	{ 0, { { NULL, 0.0, 0.0 } } }, // ComicEffect
	{ 3, { { "CenterX", 0.0, 100.0 }, { "CenterY", 0.0, 100.0 }, { "inputRadius", 1.0, 100.0 } } }, // Crystalize
	{ 1, { { "inputIntensity", 0.0, 10.0 } } }, // Edges
	{ 1, { { "inputRadius", 0.0, 20.0 } } }, // EdgeWork
	{ 3, { { "CenterX", 0.0, 100.0 }, { "CenterY", 0.0, 100.0 }, { "inputScale", 1.0, 100.0 } } }, // HexagonalPixellate
	{ 3, { { "CenterX", 0.0, 100.0 }, { "CenterY", 0.0, 100.0 }, { "inputRadius", 0.01, 1000.0 } } }, // HoleDistortion
	{ 1, { { "inputAngle", -3.14, 3.14 } } } // HueAdjust
};

@implementation CVFilterPanel
- (id) init
{
	if (![super initWithWindowNibName:@"EffectsPanel"])
		return nil;
	layer = nil;
	return self;
}

- (void)windowDidLoad
{
	//NSLog(@"Nib file loaded");
}

- (void)show
{
	// force opening the EffectsPanel under the mouse pointer
	NSPoint origin = [NSEvent mouseLocation];
	NSRect frame = [[self window] frame];
	origin.x -= frame.size.width/2;
	origin.y -= frame.size.height/2;
	[[self window] setFrameOrigin:origin];
	[[self window] makeKeyAndOrderFront:self];
}

- (void)setLayer:(NSView *)lay
{
	layer = lay;
	[[self window] setTitle:[layer toolTip]];
}

- (IBAction)setFilterParameter:(id)sender
{
	if(layer)
		[layer setFilterParameter:sender];
}

- (FilterParams *)getFilterParamsDescriptorAtIndex:(int)index
{
	if (index >= FILTERS_MAX)
		return nil;
	return &fParams[index];
}

@end

@implementation CVFilterBox

- (void)awakeFromNib
{
	trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
				options: (NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow)
				owner:self userInfo:nil];
	[self addTrackingArea:trackingArea];

}

- (void)mouseEntered:(NSEvent *)theEvent {
}
 
- (void)mouseExited:(NSEvent *)theEvent {
	NSPoint p = [[self window] mouseLocationOutsideOfEventStream];
	NSRect bounds = [self bounds];
	// close the filter panel only if mouse coordinates are 
	// really outside of the window (but not if we for examples
	// selected a new filter using the NSPopupButton
	if (p.x < 0 || p.y < 0 || 
		p.x > bounds.size.width || p.y > bounds.size.height)
	{
		[[self window] orderOut:self];
	}
}



@end