/*  FreeJ
 *  (c) Copyright 2006-2009 Denis Roio <jaromil@dyne.org>
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

 * Virtual controller class to be inherited by other controllers

 */

/**
   @file controller.h
   @brief FreeJ generic Controller interface
 */

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <config.h>
#include <cstdarg> // va_list
#ifdef WITH_JAVASCRIPT
#include <jsapi.h> // spidermonkey header
#endif //WITH_JAVASCRIPT

#include <linklist.h>

FREEJ_FORWARD_PTR(Context)

FREEJ_FORWARD_PTR(ControllerListener)
class ControllerListener : public Entry {
public:
#ifdef WITH_JAVASCRIPT
    ControllerListener(JSContext *cx, JSObject *obj);
#endif //WITH_JAVASCRIPT
    ~ControllerListener();
    bool frame();
    // TODO: eliminate runtime resolution -> C++ overhead alert!
#ifdef WITH_JAVASCRIPT
    bool call(const char *funcname, int argc, jsval *argv);
    bool call(const char *funcname, int argc, const char *format, ...);
#endif //WITH_JAVASCRIPT
private:
#ifdef WITH_JAVASCRIPT
    jsval frameFunc;
#endif //WITH_JAVASCRIPT

};

FREEJ_FORWARD_PTR(Controller)
class Controller : public Entry {
    friend class Context;

public:
    Controller();
    virtual ~Controller();

    // we need a pointer to context because controllers aren't depending from javascript
    // those who need it will retreive JSContext/Object from freej->js->global_context/object
    virtual bool init(ContextPtr freej); ///< initialize the controller to be used on the Context

    // function called in main loop,
    virtual int poll() = 0; ///< poll() is called internally at every frame processed,
    ///< returns 0 = requeue, 1 = event handled

    virtual int dispatch() = 0; ///< dispatch() is implemented by the specific controller
    ///< distributes the signals to listeners, can be overrided in python

    bool initialized; ///< is this class initialized on a context?
    bool active; ///< is this class active?

    bool indestructible; ///< set to true if you want the controller to survive a reset

    bool javascript; ///< was this controller created by javascript?

    bool add_listener(ControllerListenerPtr listener);

    void reset();

    // TODO: eliminate runtime resolution -> C++ overhead alert!
#ifdef WITH_JAVASCRIPT
    int JSCall(const char *funcname, int argc, jsval *argv);
    int JSCall(const char *funcname, int argc, const char *format, ...);
#endif //WITH_JAVASCRIPT
    Linklist<ControllerListener> listeners;
};

#endif
