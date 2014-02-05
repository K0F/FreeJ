/*  FreeJ
 *  (c) Copyright 2001 Denis Roio aka jaromil <jaromil@dyne.org>
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
 */

#ifndef __XSCREENSAVER_LAYER_H__
#define __XSCREENSAVER_LAYER_H__

#include <config.h>
#ifdef WITH_XSCREENSAVER

#include <layer.h>
#include <vroot.h>

class XScreenSaverLayer: public Layer {

public:
    XScreenSaverLayer();
    ~XScreenSaverLayer();


    bool open(const char *file);
    void *feed();
    void close();
    void pause(bool paused);

// extern char *progclass;
// void xhacks_handle_events(int);

protected:
    bool _init();

    void *output;

private:

    Display *dpy;
    Window back_win;
    //Pixmap back_win;
    XImage *img;
    GC gc;
    bool paused;

    int x_pid;
    char *_name;
    char *_author;
    char *_info;
    int _version;
    //  int _bpp;
    char *_path;

};

#endif // WITH_XSCREENSAVER
#endif // __XSCREENSAVER_LAYER_H__

