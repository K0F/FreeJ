/*  FreeJ
 *  (c) Copyright 2001-2007 Denis Rojo aka jaromil <jaromil@dyne.org>
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

#include <kbd_ctrl.h>

#include <config.h>

#include <context.h>
#include <jutils.h>

#include <callbacks_js.h> // javascript
#include <jsparser_data.h>

#define SDL_REPEAT_DELAY	200
#define SDL_REPEAT_INTERVAL	20

#define SDL_KEYEVENTMASK (SDL_KEYDOWNMASK|SDL_KEYUPMASK)


/////// Javascript KeyboardController
JS(js_kbd_ctrl_constructor);

DECLARE_CLASS("KeyboardController",js_kbd_ctrl_class, js_kbd_ctrl_constructor);

JSFunctionSpec js_kbd_ctrl_methods[] = {
  {0}
};


KbdCtrl::KbdCtrl()
  :Controller() {
  set_name("Keyboard");
  func("%s this=%p",__PRETTY_FUNCTION__, this);
}

KbdCtrl::~KbdCtrl() {
  func("%s this=%p",__PRETTY_FUNCTION__, this);

}

bool KbdCtrl::init(JSContext *env, JSObject *obj) {

  /* enable key repeat */
  SDL_EnableKeyRepeat(SDL_REPEAT_DELAY, SDL_REPEAT_INTERVAL);

  jsenv = env;
  jsobj = obj;
  //SDL_EnableUNICODE(true);
  
  initialized = true;
  return(true);
}

int KbdCtrl::peep(Context *env) {
  int res;
  SDL_Event user_event;

  res = SDL_PeepEvents(&env->event, 1, SDL_PEEKEVENT, SDL_KEYEVENTMASK);
  if (!res) return 1;

  user_event.type=SDL_USEREVENT;
  user_event.user.code=42;
  SDL_PeepEvents(&user_event, 1, SDL_ADDEVENT, SDL_ALLEVENTS);

  res = SDL_PeepEvents(&env->event, 1, SDL_GETEVENT, SDL_KEYEVENTMASK|SDL_EVENTMASK(SDL_USEREVENT));
  while (res>0) {
    int handled = poll(env);
    if (handled == 0)
        SDL_PeepEvents(&env->event, 1, SDL_ADDEVENT, SDL_ALLEVENTS);
    res = SDL_PeepEvents(&env->event, 1, SDL_GETEVENT, SDL_KEYEVENTMASK|SDL_EVENTMASK(SDL_USEREVENT));
    if (env->event.type == SDL_USEREVENT)
        res = 0;
  }
  return 1;
}

int KbdCtrl::checksym(SDLKey key, char *name) {
  if(keysym->sym == key) {
    strcat(keyname,name);
    func("keyboard controller detected key: %s",keyname);
    if(env->event.key.state == SDL_PRESSED)
      snprintf(funcname, 511, "pressed_%s", keyname);
    else // if(env->event.key.state == SDL_RELEASED)
      snprintf(funcname, 511, "released_%s", keyname);

    return JSCall(funcname);
  }
  return 0;
}


int KbdCtrl::poll(Context *env) {
  char tmp[8];
  int res = 0;

  if(env->event.key.state != SDL_PRESSED)
    if(env->event.key.state != SDL_RELEASED)
      return 0; // no key state change
  
  keysym = &env->event.key.keysym;
  //Uint16 keysym->unicode
  //char * SDL_GetKeyName(keysym->sym);
  func("KB u: %i / ks: %s", keysym->unicode, SDL_GetKeyName(keysym->sym));

  memset(keyname, 0, sizeof(char)<<9);  // *512
  memset(funcname, 0, sizeof(char)<<9); // *512
  
  // check key modifiers
  if(keysym->mod & KMOD_SHIFT)
    strcat(keyname,"shift_");
  if(keysym->mod & KMOD_CTRL)
    strcat(keyname,"ctrl_");
  if(keysym->mod & KMOD_ALT)
    strcat(keyname,"alt_");
  
  // check normal alphabet and letters
  if( keysym->sym >= SDLK_0
      && keysym->sym <= SDLK_9) {
    tmp[0] = keysym->sym;
    tmp[1] = 0x0;
    strcat(keyname,tmp);
    if(env->event.key.state == SDL_PRESSED)
      sprintf(funcname,"pressed_%s",keyname);
    else //if(env->event.key.state != SDL_RELEASED)
      sprintf(funcname,"released_%s",keyname);
    return JSCall(funcname);
  }
  
  if( keysym->sym >= SDLK_a
      && keysym->sym <= SDLK_z) {
    
    tmp[0] = keysym->sym;
    tmp[1] = 0x0;
    strcat(keyname,tmp);
    if(env->event.key.state == SDL_PRESSED)
      sprintf(funcname,"pressed_%s",keyname);
    else //if(env->event.key.state != SDL_RELEASED)
      sprintf(funcname,"released_%s",keyname);
    return JSCall(funcname);
  }

  // check arrows
  res |= checksym(SDLK_UP,        "up");
  res |= checksym(SDLK_DOWN,      "down");
  res |= checksym(SDLK_RIGHT,     "right");
  res |= checksym(SDLK_LEFT,      "left");
  res |= checksym(SDLK_INSERT,    "insert");
  res |= checksym(SDLK_HOME,      "home");
  res |= checksym(SDLK_END,       "end");
  res |= checksym(SDLK_PAGEUP,    "pageup");
  res |= checksym(SDLK_PAGEDOWN,  "pagedown");


  // check special keys
  res |= checksym(SDLK_BACKSPACE, "backspace");
  res |= checksym(SDLK_TAB,       "tab");
  res |= checksym(SDLK_RETURN,    "return");
  res |= checksym(SDLK_SPACE,     "space");
  res |= checksym(SDLK_PLUS,      "plus");
  res |= checksym(SDLK_MINUS,     "minus");
  res |= checksym(SDLK_ESCAPE,    "esc");
  res |= checksym(SDLK_LESS,      "less");
  res |= checksym(SDLK_GREATER,   "greater");
  res |= checksym(SDLK_EQUALS,    "equal");
  

  // check numeric keypad
  if(keysym->sym >= SDLK_KP0
     && keysym->sym <= SDLK_KP9) {
    tmp[0] = keysym->sym - SDLK_KP0 + 48;
    tmp[1] = 0x0;
    strcat(keyname,"num_");
    strcat(keyname,tmp);
    if(env->event.key.state == SDL_PRESSED)
      sprintf(funcname,"pressed_%s",keyname);
    else //if(env->event.key.state != SDL_RELEASED)
      sprintf(funcname,"released_%s",keyname);
    return JSCall(funcname);
  }
  res |= checksym(SDLK_KP_PERIOD,   "num_period");
  res |= checksym(SDLK_KP_DIVIDE,   "num_divide");
  res |= checksym(SDLK_KP_MULTIPLY, "num_multiply");
  res |= checksym(SDLK_KP_MINUS,    "num_minus");
  res |= checksym(SDLK_KP_PLUS,     "num_plus");
  res |= checksym(SDLK_KP_ENTER,    "num_enter");
  res |= checksym(SDLK_KP_EQUALS,   "num_equals");

  return res;
}

int KbdCtrl::JSCall(char *funcname) {
    jsval fval = JSVAL_VOID;
    jsval ret = JSVAL_VOID;

    func("%s calling method %s()", __func__, funcname);
    int res = JS_GetProperty(jsenv, jsobj, funcname, &fval);
    if(!JSVAL_IS_VOID(fval)) {
        res = JS_CallFunctionValue(jsenv, jsobj, fval, 0, NULL, &ret);
        if (res)
            if(!JSVAL_IS_VOID(ret)) {
                JSBool ok;
                JS_ValueToBoolean(jsenv, ret, &ok);
                if (ok) // JSfunc returned 'true', we are done.
                    return 1;
            }
    }
    return 0;
}

JS(js_kbd_ctrl_constructor) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

  KbdCtrl *kbd = new KbdCtrl();

  // assign instance into javascript object
  if( ! JS_SetPrivate(cx, obj, (void*)kbd) ) {
    error("failed assigning keyboard controller to javascript");
    delete kbd; return JS_FALSE;
  }

  // initialize with javascript context
  if(! kbd->init(cx, obj) ) {
    error("failed initializing keyboard controller");
    delete kbd; return JS_FALSE;
  }

  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
}
    
