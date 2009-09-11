//
//  CVLayerView.h
//  freej
//
//  Created by xant on 8/30/09.
//  Copyright 2009 dyne.org. All rights reserved.
//

#ifndef __CVLAYERVIEW_H__
#define __CVLAYERVIEW_H__

#include "CVLayerController.h"
#include <QuartzCore/QuartzCore.h>
#import  <CVtexture.h>
#import  <CFreeJ.h>

@class CVFilterPanel;
@class CVLayerController;
@class CVPreview;
class CVLayer;

@interface CVLayerView : NSOpenGLView {
@protected
    NSRecursiveLock       *lock;
    bool                   needsReshape;

    CIFilter               *scaleFilter;

    CVPreview              *previewTarget;
    CIContext              *ciContext;
    CIImage                *posterImage;
    IBOutlet CFreej        *freej;
    IBOutlet CVFilterPanel *filterPanel;
    IBOutlet NSSlider      *alphaBar;
    IBOutlet NSButton      *showButton;
    IBOutlet NSButton      *previewButton;
    IBOutlet CVLayerController *layerController;
}

- (void)clear;
- (CVFilterPanel *)filterPanel;
- (CVPreview *)getPreviewTarget;
- (NSString *)filterName;
- (NSDictionary *)filterParams;
// - (void)drawPosterImage:(CIImage *)posterInputImage withScaleFactor:(float)scaleFactor;
- (void)setPosterImage:(NSImage *)image;
- (void)initialized;
// Interface Builder API 
- (IBAction)setFilterParameter:(id)sender; /// tags from 0 to 10
- (IBAction)setBlendMode:(id)sender; /// tag -1
- (IBAction)toggleFilters:(id)sender; /// toggle CIImage filters
- (IBAction)togglePreview:(id)sender; /// toggle preview rendering
// toggle layer registration on the underlying context
// (so to control wether the layer has to be sent to the Screen or not)
- (IBAction)toggleVisibility:(id)sender;

@end

#endif
