/*  FreeJ
 *  (c) Copyright 2005 Denis Roio aka jaromil <jaromil@dyne.org>
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
 * "$Id: layer.h 670 2005-08-21 18:49:32Z jaromil $"
 *
 */

#ifndef __TEXT_LAYER_H__
#define __TEXT_LAYER_H__

#include <SDL.h>
#include <SDL_ttf.h>
#include <layer.h>

#define MAX_FONTS 512

class TTFLayer: public Layer {

 public:
  TTFLayer();
  ~TTFLayer();

  
  bool init(int width, int height);
  
  bool open(char *file);
  void *feed();
  void close();

  void print(char *str);

  bool keypress(int key);

  SDL_Color bgcolor;
  SDL_Color fgcolor;
  int size;

  int scanfonts(char *path);

  TTF_Font *font;

 private:
  SDL_Surface *surf;


  char* font_files[MAX_FONTS];
  int num_fonts;
  int sel_font;

};

#endif
