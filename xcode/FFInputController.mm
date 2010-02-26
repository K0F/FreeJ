/*  FreeJ
 *  (c) Copyright 2010 Robin Gareus <robin@gareus.org>
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

#import <FFInputController.h>
#import <CFreej.h>
#import <CIAlphaFade.h>

#include <jutils.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <libavutil/pixfmt.h>

extern "C" {
#include "ffdec.h"
}

@implementation FFInputController : CVLayerController

- (id)init
{
    ff = NULL;
    exportedFrame = nil;
    currentFrame = nil;
    return [super init];
}

- (void)dealloc
{
    if (currentFrame)
        CVPixelBufferRelease(currentFrame);
    [super dealloc];
}


- (CVTexture *)getTexture
{
    CVTexture *ret;
    ret = [super getTexture];
    return ret;
}

- (CVReturn)setStream
{
    Context *ctx = [freej getContext];

    char *movie = "/Users/rgareus/Movies/tac-ogg/Celluloidremix-EenDoordeweekseDag19325min50397.ogg";
  //char *movie = "http://theartcollider.org:8002/example1.ogv";

    [lock lock];
    if (ff) {
	fprintf(stderr,"FFdec: DEBUG: close stream");
        close_movie(ff);
        free_moviebuffer(ff);
        free_ff(&ff);
        ff=NULL;
    }
#if 0
    if (open_ffmpeg(&ff, movie)) {
	fprintf(stderr,"FFdec: open failed");
    }
#else
    ffdec_thread(&ff, movie, ctx->screen->geo.w, ctx->screen->geo.h, PIX_FMT_ARGB); 
#endif

/*
    if (layerView)
       [layerView setPosterImage:poster];
*/
       
    // register the layer within the freej context
    if (!layer) {
	layer = new CVLayer(self);
	layer->init();
    }
    [lock unlock];
}

- (CVReturn)renderFrame
{
    Context *ctx = [freej getContext];

    if (!currentFrame) {
	// http://developer.apple.com/mac/library/DOCUMENTATION/GraphicsImaging/Reference/CoreVideoRef/Reference/reference.html#//apple_ref/c/func/CVOpenGLBufferCreate
#if 0 
	NSDictionary *d = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES], kCVPixelBufferCGImageCompatibilityKey, [NSNumber numberWithBool:YES], kCVPixelBufferCGBitmapContextCompatibilityKey, nil];

#else 
	NSDictionary *d = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES], kCVPixelBufferCGImageCompatibilityKey, [NSNumber numberWithBool:YES], kCVPixelBufferCGBitmapContextCompatibilityKey, [NSNumber numberWithBool:YES], kCVPixelBufferOpenGLCompatibilityKey, nil];
#endif
        
        CVPixelBufferCreate(
		kCFAllocatorDefault,
		ctx->screen->geo.w,
		ctx->screen->geo.h,
		k32ARGBPixelFormat,
		(CFDictionaryRef)d, 
		&currentFrame);
    }

    if (1) { // decode nextFrame to currentFrame
   //   CGLLockContext(glContext);
   //   CGLSetCurrentContext(glContext);
	CVPixelBufferLockBaseAddress(currentFrame, 0);
	//int w=CVPixelBufferGetBytesPerRow(currentFrame);
	uint8_t* buf= (uint8_t*) CVPixelBufferGetBaseAddress(currentFrame);
	//fprintf(stderr," buf-size:%d - %dx%d", sizeof(buf), ctx->screen->geo.w, ctx->screen->geo.h);
#if 0
	int i;
	for (i=0; i< 4*ctx->screen->geo.w*ctx->screen->geo.h; i++)
		buf[i] = i%255;	
#else
	if (ff && (get_pt_status(ff)&3) ==1) {
/*
	    printf("%dx%d -> %dx%d\n", get_scaled_width(ff), get_scaled_height(ff),
	                               ctx->screen->geo.w, ctx->screen->geo.h);
*/
	    //memcpy(buf, get_bufptr(ff), 4 * ctx->screen->geo.w * ctx->screen->geo.h *sizeof(uint8_t));
	    memcpy(buf, get_bufptr(ff), 4 * get_scaled_width(ff)* get_scaled_height(ff) *sizeof(uint8_t));
	}
#endif
	CVPixelBufferUnlockBaseAddress(currentFrame, 0);
    //  CGLUnlockContext(glContext);
    }
    if (!ffdec_thread(&ff, NULL, 0, 0, 0) && ff) {
    ; // ifpsc[i]+=get_fps(ff);
    }

    [lock lock];
    newFrame = YES; // XXX
    if (layer)
        layer->vbuffer = currentFrame;
    [lock unlock];
    [self renderPreview];
    return kCVReturnSuccess;
}

- (BOOL)togglePlay
{
    fprintf(stderr,"toggleplay\n");
    return NO;
}

@end
