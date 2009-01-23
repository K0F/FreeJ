/*  FreeJ
 *  (c) Copyright 2001-2007 Denis Roio aka jaromil <jaromil@dyne.org>
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


#ifndef __JOY_CTRL_H__
#define __JOY_CTRL_H__

#include <controller.h>

#include <SDL.h>

class JoyController : public Controller {

 public:
  JoyController();
  ~JoyController();
  
  bool init(JSContext *env, JSObject *obj);
  int  poll();
  virtual int dispatch();
  
  virtual int axismotion(int device, int axis, int value);
  virtual int ballmotion(int device, int ball, int xrel, int yrel);
  virtual int hatmotion(int device, int hat, int value);
  virtual int button_down(int device, int button);
  virtual int button_up(int device, int button);

 private:
  SDL_Joystick *joy[4];
  int num;
  int axes;
  int buttons;
  int balls;
  int hats;

};

#endif
