/*  FreeJ
 *  (c) Copyright 2004 Silvano Galliani aka kysucix <silvano.galliani@poste.it>
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
 *
 */

#include <config.h>
#ifdef WITH_JAVASCRIPT

#include <jsparser.h>
#include <jsparser_data.h> // private data header
#include <context.h>

#include <string.h>

/* we declare the Context pointer static here
   in order to have it accessed from callback functions
   which are not class methods */
static Context *env;

//JSBool layer_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
//JSBool add_layer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
//JSBool add_filter(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JsParser::JsParser(Context *_env) {
    if(_env!=NULL)
	env=_env;
    init();
    parse_count=0;
    notice("JsParser::JsParser created");
}

JsParser::~JsParser() {
    /** The world is over */
    JS_DestroyContext(js_context);
    JS_DestroyRuntime(js_runtime);
    JS_ShutDown();
    notice("JsParser::close()");
}

/*
JSBool kolos(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSString *str;
    char *h;
    printf("kolos() has %d arguments.\n", argc);
    printf("Argument number %u is %d\n", 1,JSVAL_TO_INT(argv[0]));
    printf("Argument number %u is %s\n", 2,JS_GetStringBytes(JS_ValueToString(cx,argv[1])));
    printf("\n");
    h=strdup("Stringa di ritorno");
    str = JS_NewStringCopyZ(cx, h); 
    *rval = STRING_TO_JSVAL(str);

    return JS_TRUE;
}
*/

void JsParser::init() {
    /* Create a new runtime environment. */
    js_runtime = JS_NewRuntime(8L * 1024L * 1024L);
    if (!js_runtime) {
	error("JsParser :: error creating runtime");
	return ; /* XXX should return int or ptr! */
    }

    /* Create a new context. */
    js_context = JS_NewContext(js_runtime, STACK_CHUNK_SIZE);

    /* if js_context does not have a value, end the program here */
    if (js_context == NULL) {
	error("JsParser :: error creating context");
	return ;
    }

    /* Create the global object here */
    global_object = JS_NewObject(js_context, &global_class, NULL, NULL);

    /* Initialize the built-in JS objects and the global object */
    JS_InitStandardClasses(js_context, global_object);

    /* Declare shell functions */
    if (!JS_DefineFunctions(js_context, global_object, global_functions)) {
	error("JsParser :: error defining global functions");
	return ;
    }

    /** Initialize Layer class constructor */
    layer_object = JS_InitClass(js_context, global_object, NULL,
		 &layer_class, layer_constructor,
		 0, NULL, NULL, NULL, NULL);

    /** Initialize Layer methods */
    JSBool ret = JS_DefineFunctions(js_context, layer_object, layer_methods);
    if(ret != JS_TRUE) {
	error("JsParser:: init() can't initialize layer methods");
    }

    /** Initialize Filter class. TODO */
    /*
    JS_InitClass(js_context, global_object, NULL,
		 &filter_class, filter_constructor,
		 0, NULL, NULL, NULL, NULL);
		 */
//    JS_DefineProperties(js_context, layer_object, layer_properties);

    return ;
}

#define JS(fun) \
JSBool fun(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)


JS(cafudda) { func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
 int t = JSVAL_TO_INT(argv[0]);
 func("cafudda for %d seconds",t);
 env->cafudda(t);
 return JS_TRUE;
}

JS(quit) { func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
 env->quit = true;
 return JS_TRUE;
}
   
JS(layer_constructor) { func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
    //    JSObject *this_obj;
    func("JsParser::layer_constructor()");
    Layer *layer;

    if(argc < 1)
	return JS_TRUE;
    else
	layer=create_layer(JS_GetStringBytes(JS_ValueToString(cx,argv[0])));
    if(layer==NULL)
	return JS_FALSE;

//    this_obj = JS_NewObject(cx, &layer_class, NULL, obj); 
    if (!JS_SetPrivate(cx, obj, (void *) layer)) {
	 error("JsParser::layer_constructor : couldn't set the private value"); 
	 return JS_FALSE;
    }
    *rval = OBJECT_TO_JSVAL(obj);
 //   func("this_obj JSObject : %p",this_obj);
//    func("obj JSObject : %p",obj);
    return JS_TRUE;
}





//            Linklist Entry

JS(entry_down) { func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
 func("JsParser :: entry_down()");
 Entry *entry;
 entry = (Entry *) JS_GetPrivate(cx, obj);
 if(!entry) {
   error("JsParser :: entry_down : class core data is null");
   return JS_FALSE;
 } else 
   entry->down();
 return JS_TRUE;
}
JS(entry_up) { func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
 func("JsParser :: entry_up()");
 Entry *entry;
 entry = (Entry *) JS_GetPrivate(cx, obj);
 if(!entry) {
   error("JsParser :: entry_up : class core data is null");
   return JS_FALSE;
 } else 
   entry->up();
 return JS_TRUE;
}
JS(entry_move) { func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
 func("JsParser :: entry_move()");
 int pos;
 pos = JSVAL_TO_INT(argv[0]);
 Entry *entry;
 entry = (Entry *) JS_GetPrivate(cx, obj);
 if(!entry) {
   error("JsParser :: entry_up : class core data is null");
   return JS_FALSE;
 } else 
   entry->move(pos);
 return JS_TRUE;
}






JSBool rem_layer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    func("JsParser :: remove_layer()");
    JSObject *jslayer;

    jslayer = JSVAL_TO_OBJECT(argv[0]);
    if(!jslayer) {
      error("JsParser :: remove_layer called with NULL argument");
      return JS_FALSE;
    }

    func("JsParser :: layer JSObject : %p",jslayer);
    Layer *lay;
    lay = (Layer *) JS_GetPrivate(cx, jslayer);
    if(!lay) {
      error("JsParser :: remove_layer : Layer core data is null");
      return JS_FALSE;
    }
    /** remove layer in real life */
    if(lay) {
	lay->rem();
	delete lay;
	lay = NULL;
    }
    return JS_TRUE;
}
JSBool add_layer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    func("JsParser :: add_layer()");
    JSObject *jslayer;

    jslayer = JSVAL_TO_OBJECT(argv[0]);
    if(!jslayer) {
      error("JsParser :: add_layer called with NULL argument");
      return JS_FALSE;
    }

    Layer *lay;
    lay = (Layer *) JS_GetPrivate(cx, jslayer);
    if(!lay) {
      error("JsParser :: add_layer : Layer core data is null");
      return JS_FALSE;
    }

    /** really add layer */
    if(lay->init(env)) {
      env->layers.add(lay);
      env->layers.sel(0); /* deselect others */
      lay->sel(true);
    } else delete lay;

    return JS_TRUE;
}
JS(layer_set_blit) { func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
    JSObject *jslayer=NULL;
    //    JSString *jsblit_type=NULL;
    char *blit_type=NULL;

    blit_type=JS_GetStringBytes(JS_ValueToString(cx,argv[0]));
    if(!blit_type) {
      error("JsParser :: set_blit called with NULL argument");
    }
    Layer *lay;
    lay = (Layer *) JS_GetPrivate(cx, obj);
    if(!lay) {
      error("JsParser :: Layer core data is null");
      return JS_FALSE;
    }
    else {
	int blit=1;
	if (strncasecmp(blit_type,"rgb",3)== 0) blit=1;
	else if (strncasecmp(blit_type,"red",3)== 0) blit=2;
	else if (strncasecmp(blit_type,"green",5)== 0) blit=3;
	else if (strncasecmp(blit_type,"blue",3)== 0) blit=4;
	else if (strncasecmp(blit_type,"add",3)== 0) blit=5;
	else if (strncasecmp(blit_type,"sub",3)== 0) blit=6;
	else if (strncasecmp(blit_type,"and",3)== 0) blit=7;
	else if (strncasecmp(blit_type,"or",2)== 0) blit=8;
	lay->set_blit(blit);
    }

    return JS_TRUE;
}



/* return lines read, or 0 on error */
int JsParser::open(const char* script_file) {
  jsval ret_val;
  FILE *fd;
  int c = 0;
  char line[512]; /* is it enough? */
  
  fd = fopen(script_file,"r");
  if(!fd) {
    error("JsParser::open : %s : %s",script_file,strerror(errno));
    return 0;
  }
  func("JsParser reading from file %s",script_file);
  while(fgets(line,512,fd)) {
    c++;
    func("%03i : %s",c,line);
    
    JSBool ok = JS_EvaluateScript (js_context, global_object,
				   line, strlen(line), script_file, c, &ret_val);

    if(ok!=JS_TRUE) {
      error("JsParser::open : %s : error evaluating script:",script_file);
      error("%03i : %s",c,line);
    }

  }

  return ret_val;
}

int JsParser::parse(const char *command) {
  jsval res;
  JSBool ok;

  if(!command) { /* true paranoia */
    warning("NULL command passed to javascript parser");
    return 0;
  }

  func("JsParser::parse : %s",command);
    
  ok =
    JS_EvaluateScript(js_context, global_object,
		      command, strlen(command), "console", 1, &res);
  if(!ok) {
    char err[512];
    JS_ReportError(js_context, "%s", err);
    error("%s",err);
    return 0;
  }
  return 1;
}

#endif
