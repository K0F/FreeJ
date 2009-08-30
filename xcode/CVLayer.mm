/*
 *  CVLayer.cpp
 *  freej
 *
 *  Created by xant on 2/23/09.
 *  Copyright 2009 dyne.org. All rights reserved.
 *
 */

#include "CVLayer.h"

CVLayer::CVLayer() : Layer()
{
    bufsize = 0;
    blendMode = NULL;
    type = Layer::GL_COCOA;
    data = (void *)this;
}

CVLayer::CVLayer(CVLayerView *vin) : Layer()
{
    input = vin;
    bufsize = 0;
    blendMode = NULL;
    type = Layer::GL_COCOA;
    data = (void *)this;
    [input setLayer:this];
    set_name([[(NSView *)input toolTip] UTF8String]);
}

CVLayer::~CVLayer()
{
    stop();
    close();
    deactivate();
    //[input togglePlay:nil];
    [input setLayer:nil];
}

/*
void 
CVLayer::run()
{
    feed();
    [input renderPreview];
}
*/

void
CVLayer::activate()
{
	opened = true;
    freej->add_layer(this);
    active = true;
    notice("Activating %s", name);
}

void
CVLayer::deactivate()
{
    freej->rem_layer(this);
    active = false;
}

bool
CVLayer::open(const char *path)
{
    //[input openFile: path];
    return false;
}

bool
CVLayer::init(Context *ctx)
{
     // * TODO - probe resolution of the default input device
    return init(ctx, ctx->screen->w, ctx->screen->h);
}

bool
CVLayer::init(Context *ctx, int w, int h)
{
    width = w;
    height = h;
    freej = ctx;
    _init(width,height);
    return start();
}

void *
CVLayer::feed()
{
    lock();
    if (active || [input needPreview]) {
        [input renderFrame];
    }
    unlock();
    return (void *)vbuffer;
}

void
CVLayer::close()
{
    //[input release];
}

bool
CVLayer::forward()
{
    if ([input respondsToSelector:@selector(stepForward)])
        [(id)input stepForward];
    return true;
}

bool
CVLayer::backward()
{
    if ([input respondsToSelector:@selector(stepForward)])
        [(id)input stepBackward];
    return true;
}

bool
CVLayer::backward_one_keyframe()
{
    return backward();
}

bool
CVLayer::set_mark_in()
{
    return false;
}

bool
CVLayer::set_mark_out()
{
    return false;
}

void
CVLayer::pause()
{
    if ([input respondsToSelector:@selector(pause)])
        [(id)input pause];
}

bool
CVLayer::relative_seek(double increment)
{
    return false;
}

// accessor to get a texture from the CVLayerView cocoa class
CVTexture * 
CVLayer::gl_texture()
{
    return [input getTexture];
}
