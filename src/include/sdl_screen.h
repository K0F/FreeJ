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
 *
 * "$Id$"
 *
 */

#ifndef __SDL_SCREEN_H__
#define __SDL_SCREEN_H__

#include <SDL.h>
#include <screen.h>

class SdlScreen : public ViewPort {
 public:
  SdlScreen();
  ~SdlScreen();

  bool init(int widt, int height);
  void show();
  bool update(void *buf);
  void *get_surface();

  SDL_Surface *scr;
  SDL_Surface *emuscr;
  void *surface;

 private:
  int setres(int wx, int hx);
  bool sdl_lock();
  bool sdl_unlock();
  
  SDL_Surface *blit;
  SDL_Rect rect;
  uint32_t rmask, gmask, bmask, amask;

  bool dbl;
  uint32_t sdl_flags;
  
};

#endif 
