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

#include "config.h"

#include <stdlib.h>

#include "layer.h"
#include "blitter.h"
#include "blit_instance.h"

#include "jutils.h"
#include "soft_screen.h"

// our objects are allowed to be created trough the factory engine
FACTORY_REGISTER_INSTANTIATOR(ViewPort, SoftScreen, Screen, soft);

typedef void (blit_f)(void *src, void *dst, int len, LinkList<ParameterInstance> &params);


SoftScreen::SoftScreen()
    : ViewPort() {

    screen_buffer = NULL;
    name = "SOFT";
}

SoftScreen::~SoftScreen() {
    func("%s", __PRETTY_FUNCTION__);
    if(screen_buffer) free(screen_buffer);
}

bool SoftScreen::_init() {
    screen_buffer = malloc(geo.getByteSize());
    clear();
    return(true);
}

void SoftScreen::blit(LayerPtr src) {
    int16_t c;

    if(src->screen.lock() != SharedFromThis(SoftScreen)) {
        error("%s: blit called on a layer not belonging to screen",
              __PRETTY_FUNCTION__);
        return;
    }

    if(src->need_crop)
        blitter->crop(src, SharedFromThis(SoftScreen));

    BlitInstancePtr b = src->current_blit;

    pscr = (uint32_t*) get_surface() + b->scr_offset;
    play = (uint32_t*) src->buffer   + b->lay_offset;

    // iterates the blit on each horizontal line
    for(c = b->lay_height; c > 0; c--) {

        ((blit_f*)b->getFun())
            ((void*)play, (void*)pscr,
            b->lay_bytepitch, // * src->geo.bpp>>3,
           b->getParameters());

        // strides down to the next line
        pscr += b->scr_stride + b->lay_pitch;
        play += b->lay_stride + b->lay_pitch;

    }

}

void SoftScreen::set_buffer(void *buf) {
    if(screen_buffer) free(screen_buffer);
    screen_buffer = buf;
}

void *SoftScreen::coords(int x, int y) {
//   func("method coords(%i,%i) invoked", x, y);
// if you are trying to get a cropped part of the layer
// use the .pixelsize geometric property for a pre-calculated stride
// that is: number of bytes for one full line
    return
        (x + geo.getPixelSize() +
         (uint32_t*)get_surface());
}

void *SoftScreen::get_surface() {
    if(!screen_buffer) {
        error("SOFT screen output is not properly initialised via set_buffer");
        error("this will likely segfault FreeJ");
        return NULL;
    }
    return screen_buffer;
}


void SoftScreen::show() {
}

void SoftScreen::clear() {
    memset(screen_buffer, 0, geo.getByteSize()); // Put in black
}

void SoftScreen::fullscreen() {
}

void SoftScreen::do_resize(int resize_w, int resize_h) {
}
