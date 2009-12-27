/*  FreeJ WiiMote controller
 *  (c) Copyright 2008 Denis Rojo <jaromil@dyne.org>
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
 * "$Id: $"
 *
 */

#ifndef __WII_CTRL_H__
#define __WII_CTRL_H__

#include <config.h>
#ifdef WITH_CWIID

#include <controller.h>
#include <jsync.h>

extern "C" {
#include <cwiid.h>
}


class WiiController: public Controller , public JSyncThread {

 public:
  WiiController();
  ~WiiController();

  int dispatch();
  int poll();
  void run();

  bool open();
  bool open(const char *hwaddr);
  bool close();

  void accel(uint8_t nx, uint8_t ny, uint8_t nz);
  bool get_accel_report();
  bool set_accel_report(bool state);

  void ir(cwiid_ir_mesg*);
  bool get_ir_report();
  bool set_ir_report(bool state);

  void button(uint16_t buttons);
  bool get_button_report();
  bool set_button_report(bool state);

  void error_event(cwiid_error err);

  bool activate(bool state);

  bool get_rumble();
  bool set_rumble(bool state);

  bool get_led(unsigned int led);
  bool set_led(unsigned int led, bool state);

  double battery();
  double x() { return (double)_x;}
  double y() { return (double)_y;}
  double z() { return (double)_z;}

  int update_state(); // debug
  int dump(); // debug

 private:

  int _nx, _ny, _nz;
  int  _x,  _y,  _z;
  struct cwiid_ir_mesg _ir_data;
  bool _wii_event_ir;
  bool _wii_event_connect;
  bool _wii_event_connect_err;

  uint16_t _newbutt;
  uint16_t _oldbutt;

  // todo nunchuk_state and classic_state extensions

  bdaddr_t _bdaddr;
  cwiid_wiimote_t  *_wiimote;
  cwiid_state _state;


};

#endif // WITH_CWIID

#endif
