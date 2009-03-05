//
//  CVFilterPanel.h
//  freej
//
//  Created by xant on 2/26/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#ifndef __CV_FILTERPANEL_H__
#define __CV_FILTERPANEL_H__

#import <Cocoa/Cocoa.h>

typedef struct __FilterParams {
	int nParams;
	struct __ParamDescr {
		char *label;
		double min;
		double max;
	} params[3];
} FilterParams;

@interface CVFilterBox : NSView
{
	NSTrackingArea *trackingArea;
}
- (void)mouseExited:(NSEvent *)theEvent;
/*
- (void)mouseEntered:(NSEvent *)theEvent;
- (void)mouseDown:(NSEvent *)theEvent;
*/
@end

@interface CVFilterPanel : NSWindowController {
	NSView *layer;
	IBOutlet CVFilterBox	*mainView;
}
- (void)show;
- (id)initWithName:(NSString *)name;
- (void)setLayer:(NSView *)lay;
- (FilterParams *)getFilterParamsDescriptorAtIndex:(int)index;
- (IBAction)setFilterParameter:(id)sender;
@end

#endif