/*  FreeJ - blitter layer component
 *  (c) Copyright 2004 Denis Roio aka jaromil <jaromil@dyne.org>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 2 of the License,
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
 * "$Id$"
 *
 */

#ifndef __BLITTER_H__
#define __BLITTER_H__

#include <blit.h>
#include <linklist.h>

FREEJ_FORWARD_PTR(Layer)
FREEJ_FORWARD_PTR(ViewPort)

template <class T> class LinkList;

FREEJ_FORWARD_PTR(Blit)

FREEJ_FORWARD_PTR(Blitter)
class Blitter : public EnableSharedFromThis<Blitter> {
public:
    Blitter();
    ~Blitter();

    LinkList<Blit> blitlist; ///< list of available blits
    BlitPtr default_blit;

    /* ==== CROP */
    /** @param force crop even if nothing changed */
    void crop(LayerPtr lay, ViewPortPtr scr);
    ///< crop to fit in the ViewPort
private:
    int16_t old_lay_x;
    int16_t old_lay_y;
    uint16_t old_lay_w;
    uint16_t old_lay_h;

    int16_t old_scr_x;
    int16_t old_scr_y;
    uint16_t old_scr_w;
    uint16_t old_scr_h;


};

#endif
