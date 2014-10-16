/*  FreeJ - S-Lang console
 *
 *  (c) Copyright 2004-2009 Denis Roio <jaromil@dyne.org>
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

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "controller.h"
#include "linklist.h"
#include "screen.h"
#include "layer.h"
#include "filter_instance.h"

FREEJ_FORWARD_PTR(ConsoleController)
class ConsoleController : public Controller, public WrapperLogger {
public:

    ConsoleController() : Controller() {
    };
    virtual ~ConsoleController() {
    };
    virtual bool console_init() = 0;

    virtual int poll() = 0;
    virtual int dispatch() = 0;
    virtual void close() = 0;

    virtual void notice(const char *msg) = 0;
    virtual void error(const char *msg) = 0;
    virtual void warning(const char *msg) = 0;
    virtual void act(const char *msg) = 0;
    virtual void func(const char *msg) = 0;
    virtual void old_printlog(const char *msg) = 0;
    virtual void logmsg(LogLevel level, const char *msg) = 0;

    virtual void refresh() = 0;
};

#ifndef SWIG
const LayerPtr &getSelectedLayer();
void setSelectedLayer(const LayerPtr &ptr);
const ViewPortPtr &getSelectedScreen();
void setSelectedScreen(const ViewPortPtr &ptr);
const FilterInstancePtr &getSelectedFilter();
void setSelectedFilter(const FilterInstancePtr &ptr);
#endif

#endif
