/*  FreeJ
 *  (c) Copyright 2001-2005 Denis Roio aka jaromil <jaromil@dyne.org>
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
 * "$Id: freej.cpp 654 2005-08-18 16:52:47Z jaromil $"
 *
 */

#include <dirent.h>
#include <callbacks_js.h>
#include <jsparser_data.h>

// global environment class
JSClass global_class = {
  "Freej", JSCLASS_NEW_RESOLVE,
  JS_PropertyStub,  JS_PropertyStub,
  JS_PropertyStub,  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub,   JS_FinalizeStub
};

JSFunctionSpec global_functions[] = {
    /*    name          native			nargs    */
    {"cafudda",         cafudda,                1},
    {"run",             cafudda,                1},
    {"quit",            quit,                   0},
    {"add_layer",	add_layer,		1},
    {"rem_layer",	rem_layer,		1},
    {"list_layers",     list_layers,            0},
    {"debug",           debug,                  1},
    {"rand",            rand,                   0},
    {"srand",           srand,                  1},
    {"pause",           pause,                  0},
    {"fullscreen",      fullscreen,             0},
    {"set_resolution",  set_resolution,         2},
    {"scandir",         freej_scandir,          1},
    {"echo",            freej_echo,             1},
    {"strstr",          freej_strstr,           2},
    {"stream_start",    stream_start,           0},
    {"stream_stop",     stream_stop,            0},
    {0}
};


JS(cafudda) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  double tmp_double;
  double *seconds;
  int int_seconds;

  if(JSVAL_IS_DOUBLE(argv[0])) {
    
    // JSVAL_TO_DOUBLE segfault when there's an int as input
    seconds=JSVAL_TO_DOUBLE(argv[0]);
    
  } else if(JSVAL_IS_INT(argv[0])) {

    int_seconds=JSVAL_TO_INT(argv[0]);
    seconds=&tmp_double;
    *seconds=(double )int_seconds;

  }
  
  func("JsParser :: run for %f seconds",*seconds);
  env->cafudda(*seconds);

  return JS_TRUE;
}

JS(pause) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  env->pause = !env->pause;

  return JS_TRUE;
}

JS(quit) {
 func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

 env->quit = true;
 return JS_TRUE;
}


JS(rem_layer) {
    func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

    JSObject *jslayer;
    Layer *lay;

    jslayer = JSVAL_TO_OBJECT(argv[0]);
    if(!jslayer) JS_ERROR("missing argument");

    lay = (Layer *) JS_GetPrivate(cx, jslayer);
    if(!lay) JS_ERROR("Layer core data is NULL");

    lay->rem();
    lay->quit=true;
    lay->signal_feed();
    lay->join();

    delete lay;
    return JS_TRUE;
}

JS(add_layer) {
    func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
    Layer *lay;
    JSObject *jslayer;
    *rval=JSVAL_FALSE;

    if(argc<1) JS_ERROR("missing argument");

    jslayer = JSVAL_TO_OBJECT(argv[0]);

    lay = (Layer *) JS_GetPrivate(cx, jslayer);
    if(!lay) JS_ERROR("Layer core data is NULL");

    /** really add layer */
    if(lay->init(env)) {
      env->layers.add(lay);
      *rval=JSVAL_TRUE;
      //      env->layers.sel(0); // deselect others
      //      lay->sel(true);
    } else error("%s: problem occurred initializing Layer",__FUNCTION__);

    return JS_TRUE;
}


JS(fullscreen) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  env->screen->fullscreen();
  env->clear_all = !env->clear_all;
  return JS_TRUE;
}

JS(set_resolution) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  int w = JSVAL_TO_INT(argv[0]);
  int h = JSVAL_TO_INT(argv[1]);
  env->screen->resize(w, h);
  return JS_TRUE;
}


JS(stream_start) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  notice ("Streaming to %s:%u",env->shouter->host(), env->shouter->port());
  act ("Saving to %s", env -> video_encoder -> get_filename());
  env->save_to_file = true;
}
JS(stream_stop) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  ::notice ("Stopped stream to %s:%u", env->shouter->host(), env->shouter->port());
  ::act ("Video saved in file %s",env -> video_encoder -> get_filename());
  env->save_to_file = false;
}

static int dir_selector(const struct dirent *dir) {
  if(dir->d_name[0]=='.') return(0); // remove hidden files
  return(1);
}
JS(freej_scandir) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  JSObject *arr;
  JSString *str;
  jsval val;
    
  struct dirent **filelist;
  int found;
  int c = 0;
  char *dir;
  
  JS_ARG_STRING(dir,0);
  
  found = scandir(dir,&filelist,dir_selector,alphasort);
  if(found<0) {
    error("scandir error: %s",strerror(errno));
    return JS_TRUE; // fatal error
  }

  arr = JS_NewArrayObject(cx, 0, NULL); // create void array
  if(!arr) return JS_FALSE;

  // now fill up the array  
  while(found--) {
    
    str = JS_NewStringCopyZ(cx, filelist[found]->d_name); 
    val = STRING_TO_JSVAL(str);    
    JS_SetElement(cx, arr, c, &val );
    c++;
  }

  *rval = OBJECT_TO_JSVAL( arr );
  return JS_TRUE;
}

JS(freej_echo) {
  char *msg;
  JS_ARG_STRING(msg,0);
  fprintf(stderr,"%s\n",msg);
  return JS_TRUE;
}

JS(freej_strstr) {
  char *haystack;
  char *needle;
  char *res;
  int intval;
  JS_ARG_STRING(haystack,0);
  JS_ARG_STRING(needle,1);
  res = strstr(haystack, needle);
  if(res == NULL)
    intval = 0;
  else intval = 1;
  *rval = INT_TO_JSVAL(intval);
  return JS_TRUE;
}
  

// debugging commodity
// run freej with -D3 to see this
JS(debug) {
  char *msg;
  
  JS_ARG_STRING(msg,0);
 
  func("%s", msg);

  return JS_TRUE;
}


JS(rand) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

  int r;

  r = rand();

  if(argc<1) *rval = 1+(int)(r/(RAND_MAX+1.0));
  else {
    JS_ARG_NUMBER(max, 0);
    func("randomizing with max %f",max);
    r = 1+(int)(max*r/(RAND_MAX+1.0));
    *rval = INT_TO_JSVAL(r);
  }

  return JS_TRUE;
}
JS(srand) {
  // this is not fast and you'd better NOT use it often
  // to achieve more randomization on higher numbers:
  // u get unpredictably sloow
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  int seed;
  if(argc<1)
    seed = time(NULL);
  else {
    JS_ARG_NUMBER(r,0);
    seed = (int)r;
  }
  
  srand(seed);

  return JS_TRUE;
}


////////////////////////////////
// Linklist Entry Methods

JS(entry_down) {
 func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

 GET_LAYER(Entry);

 lay->down();
 
 return JS_TRUE;
}
JS(entry_up) {
 func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

 GET_LAYER(Entry);

 lay->up();

 return JS_TRUE;
}

JS(entry_move) {
 func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

 GET_LAYER(Entry);

 int pos = JSVAL_TO_INT(argv[0]);
 lay->move(pos);

 return JS_TRUE;
}

////////////////////////////////
// Effect methods
// TODO effect methods to control effect parameters
JSFunctionSpec effect_methods[] = {
  ENTRY_METHODS  ,
  {0}
};
DECLARE_CLASS("Effect",effect_class,effect_constructor);
JS(effect_constructor) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

  Filter *filter;
  char *effect_name;

  if(argc < 1) JS_ERROR("missing argument");

  JS_ARG_STRING(effect_name,0);

  filter = env->plugger.pick(effect_name);

  if(filter==NULL) {
    error("JsParser::effect_constructor : filter not found :%s",effect_name); 
    *rval = JSVAL_FALSE;
    return JS_TRUE;
  }
  
  if (!JS_SetPrivate(cx, obj, (void *) filter))
    JS_ERROR("internal error setting private value");

  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
}
