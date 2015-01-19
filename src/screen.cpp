/*  FreeJ
 *  (c) Copyright 2001 - 2010 Denis Roio <jaromil@dyne.org>
 *
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "context.h"
#include "screen.h"
#include "blit_instance.h"
#include "layer.h"
#include "video_encoder.h"
#ifdef WITH_AUDIO
#include "audio_jack.h"
#endif
#include "ringbuffer.h"

#include "video_layer.h"

#ifdef WITH_GD
#include <gd.h>
#endif

#include <algorithm>

LinkList<Blit> &get_linear_blits();

ViewPort::ViewPort()
    : Entry() {
    func("%s this=%p", __PRETTY_FUNCTION__, this);

    opengl = false;

    resize_w = 0;
    resize_h = 0;
    resizing = false;

    indestructible = false;

    active = true;

#ifdef WITH_AUDIO
    audio = NULL;
    m_SampleRate = NULL;
    indestructible = false;
    // if compiled with audio initialize the audio data pipe
//   audio = ringbuffer_create(1024 * 512);
    audio = ringbuffer_create(4096 * 512 * 8);
#endif
}

ViewPort::~ViewPort() {
    func("%s this=%p", __PRETTY_FUNCTION__, this);
#ifdef WITH_AUDIO
    if(audio) ringbuffer_free(audio);
#endif
}

bool ViewPort::init(int w, int h, int bpp) {

    if(bpp != 32) {
        warning("FreeJ is forced to use 32bit pixel formats, hardcoded internally");
        warning("you are initializing a ViewPort with a different bpp value");
        warning("please submit a patch if you can make it :)");
        return false;
    }

    geo.init(w, h, bpp);
    initialized = _init();
    func("screen %s initialized with size %ux%u", name.c_str(), geo.w, geo.h);

    return initialized;

}

bool ViewPort::isActive() const {
    return active;
}

bool ViewPort::add_layer(LayerPtr lay) {
    func("%s", __PRETTY_FUNCTION__);

    if(lay->screen.lock()) {
        warning("passing a layer from a screen to another is not (yet) supported");
        return(false);
    }

    if(!lay->isOpen()) {
        error("layer %s is not yet opened, can't add it", lay->getName().c_str());
        return(false);
    }

    lay->screen = SharedFromThis(ViewPort);
    lay->current_blit = blitter->new_instance(blitter->default_blit);
    
    // center the position
    //lay->geo.x = (screen->w - lay->geo.w)/2;
    //lay->geo.y = (screen->h - lay->geo.h)/2;
    //  screen->blitter->crop( lay, screen );
    layers.push_front(lay);
    lay->active = true;
    func("layer %s added to screen %s", lay->getName().c_str(), name.c_str());
    return(true);
}

#ifdef WITH_AUDIO
bool ViewPort::add_audio(JackClientPtr jcl) {
    LinkList<Layer>::iterator it = layers.begin();
    if(it == layers.end()) return false;

    VideoLayerPtr lay = DynamicPointerCast<VideoLayer>(*it);

    jcl->SetRingbufferPtr(audio, (int)lay->audio_samplerate, (int)lay->audio_channels);
    std::cerr << "------ audio_samplerate :" << lay->audio_samplerate \
              << " audio_channels :" << lay->audio_channels << std::endl;
    m_SampleRate = &jcl->m_SampleRate;
    return (true);
}

#endif

void ViewPort::rem_layer(LayerPtr lay) {
    LinkList<Layer>::iterator it = std::find(layers.begin(), layers.end(), lay);
    if(it == layers.end()) {
        error("layer %s is not inside this screen", lay->getName().c_str());
        return;
    }

    lay->screen.reset(); // symmetry
    layers.erase(it);
    func("layer %s removed from screen %s", lay->getName().c_str(), name.c_str());
}

LinkList<Layer> &ViewPort::getLayers() {
    return layers;
}

void ViewPort::reset() {
    layers.clear();
}

bool ViewPort::add_encoder(VideoEncoderPtr enc) {
    func("%s", __PRETTY_FUNCTION__);

    func("initializing encoder %s", enc->getName().c_str());
    if(!enc->init(SharedFromThis(ViewPort))) {
        error("%s : failed initialization", __PRETTY_FUNCTION__);
        return(false);
    }
    func("initialization done");

    enc->start();

    encoders.push_back(enc);

    act("encoder %s added to screen %s", enc->getName().c_str(), name.c_str());
    return true;
}

#ifdef WITH_GD
void ViewPort::save_frame(char *file) {
    FILE *fp;
    gdImagePtr im;
    int *src;
    int x, y;

    im = gdImageCreateTrueColor(geo.w, geo.h);
    src = (int*)coords(0, 0);
    for(y = 0; y < geo.h; y++) {
        for(x = 0; x < geo.w; x++) {
            gdImageSetPixel(im, x, y, src[x] & 0x00FFFFFF);
            //im->tpixels[y][x] = src[x] & 0x00FFFFFF;
        }
        src += geo.w;
    }
    fp = fopen(file, "wb");
    gdImagePng(im, fp);
    fclose(fp);
}

#endif


void ViewPort::cafudda(double secs) {
    std::for_each(layers.begin(), layers.end(), [&](LayerPtr &lay) {
                      lay->cafudda(secs);
                  });
}

void ViewPort::blit(LayerPtr src) {
    if(src->screen.lock() != SharedFromThis(ViewPort)) {
        error("%s: blit called on a layer not belonging to screen",
              __PRETTY_FUNCTION__);
        return;
    }

    BlitInstancePtr b = src->current_blit;

    (*b)(src);
}

void ViewPort::blit_layers() {
    std::for_each(layers.rbegin(), layers.rend(), [&](LayerPtr &lay) {
                      if(lay->buffer) {
                          if(lay->isActive() && lay->isVisible() && lay->isOpen()) {
                              blit(lay);
                          }
                      }
                  });
    /////////// finish processing layers

}

void ViewPort::handle_resize() {
    lock();
    if(resizing) {
        do_resize(resize_w, resize_h);
    }
    unlock();
    resizing = false;
}

void ViewPort::resize(int w, int h) {
    resize_w = w;
    resize_h = h;
    resizing = true;
}

LinkList<VideoEncoder>& ViewPort::getEncoders() {
    return encoders;
}
