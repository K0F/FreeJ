/*  FreeJ
 *
 *  Copyright (C) 2004-2006
 *  Silvano Galliani aka kysucix <kysucix@dyne.org>
 *  Denis Rojo aka jaromil <jaromil@dyne.org>
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

#include <string.h>


#include <context.h>
#include <signal.h>
#include <config.h>
#include <jutils.h>
#include <errno.h>

#include <callbacks_js.h>
#include <jsparser.h>
#include <jsparser_data.h> // private data header

#include <impl_layers.h>


JsParser::JsParser(Context *_env) {
    if(_env!=NULL)
	env=_env;
    init();
    notice("JavaScript parser initialized");
}

JsParser::~JsParser() {
    /** The world is over */
    JS_DestroyContext(js_context);
    JS_DestroyRuntime(js_runtime);
    JS_ShutDown();
    func("JsParser::close()");
}

void JsParser::gc() {
    JS_GC(js_context);
}

void JsParser::init() {
  //JSBool ret;
  stop_script=false;
  
  notice("Initializing %s", JS_GetImplementationVersion());

    /* Create a new runtime environment. */
    js_runtime = JS_NewRuntime(8L * 1024L * 1024L);
    if (!js_runtime) {
	error("JsParser :: error creating runtime");
	return ; /* XXX should return int or ptr! */
    }

    /* Create a new context. */
    js_context = JS_NewContext(js_runtime, STACK_CHUNK_SIZE);

    // Store a reference to ourselves in the context ...
    JS_SetContextPrivate(js_context, this);

    /* if js_context does not have a value, end the program here */
    if (js_context == NULL) {
	error("JsParser :: error creating context");
	return ;
    }

    /* Set a more strict error checking */
    JS_SetOptions(js_context, JSOPTION_VAROBJFIX); // | JSOPTION_STRICT);

    /* Create the global object here */
    global_object = JS_NewObject(js_context, &global_class, NULL, NULL);
    //    JS_SetGlobalObject(js_context, global_object);
    //    this is done in JS_InitStandardClasses.

    /* Set the branch callback */
    JS_SetBranchCallback(js_context, js_static_branch_callback);

    /* Set the error reporter */
    JS_SetErrorReporter(js_context, js_error_reporter);

    /* Sets maximum (if stack grows upward) or minimum (downward) legal stack byte
     * address in limitAddr for the thread or process stack used by cx.  To disable
     * stack size checking, pass 0 for limitAddr.
     * JS_SetThreadStackLimit(js_context, 0x0);
     */
   
    /* Initialize the built-in JS objects and the global object */
    JS_InitStandardClasses(js_context, global_object);

    /* Declare shell functions */
    if (!JS_DefineFunctions(js_context, global_object, global_functions)) {
	error("JsParser :: error defining global functions");
	return ;
    }

    ///////////////////////////////////////////////////////////
    // Initialize classes

	JSObject *object_proto; // reminder for inher.
	JSObject *layer_object; // used in REGISTER_CLASS macro
    REGISTER_CLASS("Layer",
		   layer_class,
		   layer_constructor,
		   layer_methods,
           NULL);
    object_proto = layer_object;

    REGISTER_CLASS("ParticleLayer",
		   particle_layer_class,
		   particle_layer_constructor,
		   particle_layer_methods,
           object_proto);

    REGISTER_CLASS("GeometryLayer",
		   geometry_layer_class,
		   geometry_layer_constructor,
		   geometry_layer_methods,
           object_proto);

    REGISTER_CLASS("VScrollLayer",
		   vscroll_layer_class,
		   vscroll_layer_constructor,
		   vscroll_layer_methods,
           object_proto);

    REGISTER_CLASS("ImageLayer",
		   image_layer_class,
		   image_layer_constructor,
		   image_layer_methods,
           object_proto);

    REGISTER_CLASS("FlashLayer",
		   flash_layer_class,
		   flash_layer_constructor,
		   flash_layer_methods,
           object_proto);

    REGISTER_CLASS("GoomLayer",
		   goom_layer_class,
		   goom_layer_constructor,
		   goom_layer_methods,
           object_proto);

#ifdef WITH_V4L
    REGISTER_CLASS("CamLayer",
		   v4l_layer_class,
		   v4l_layer_constructor,
		   v4l_layer_methods,
           object_proto);
#endif

#ifdef WITH_FFMPEG
    REGISTER_CLASS("MovieLayer",
		   video_layer_class,
		   video_layer_constructor,
		   video_layer_methods,
           object_proto);
#else
    REGISTER_CLASS("MovieLayer",
		   movie_layer_class,
		   movie_layer_constructor,
		   movie_layer_methods,
           object_proto);
#endif

#ifdef WITH_AVIFILE
   REGISTER_CLASS("MovieLayer",
		   avi_layer_class,
		   avi_layer_constructor,
		   avi_layer_methods,
           object_proto);
#endif

#ifdef WITH_FT2
    REGISTER_CLASS("TextLayer",
		   txt_layer_class,
		   txt_layer_constructor,
		   txt_layer_methods,
           object_proto);
#endif
    
    
    REGISTER_CLASS("Filter",
                   filter_class,
                   filter_constructor,
                   filter_methods,
                   NULL);

    // controller classes
    REGISTER_CLASS("Controller",
		   js_ctrl_class,
		   NULL,
		   js_ctrl_methods,
           NULL);
    object_proto = layer_object;

    REGISTER_CLASS("KeyboardController",
		   js_kbd_ctrl_class,
		   js_kbd_ctrl_constructor,
		   js_kbd_ctrl_methods,
           object_proto);

    REGISTER_CLASS("JoystickController",
		   js_joy_ctrl_class,
		   js_joy_ctrl_constructor,
		   js_joy_ctrl_methods,
           object_proto);

    REGISTER_CLASS("TriggerController",
           js_trigger_ctrl_class,
           js_trigger_ctrl_constructor,
           js_trigger_ctrl_methods,
           object_proto);

#ifdef WITH_MIDI
    REGISTER_CLASS("MidiController",
		   js_midi_ctrl_class,
		   js_midi_ctrl_constructor,
		   js_midi_ctrl_methods,
           object_proto);
#endif

    REGISTER_CLASS("OscController",
		   js_osc_ctrl_class,
		   js_osc_ctrl_constructor,
		   js_osc_ctrl_methods,
		   object_proto);

#ifdef WITH_OGGTHEORA
    // encoder class
    REGISTER_CLASS("VideoEncoder",
		   js_vid_enc_class,
		   js_vid_enc_constructor,
		   js_vid_enc_methods,
           NULL);
#endif

//    JS_DefineProperties(js_context, layer_object, layer_properties);

   /** register SIGINT signal */
   signal(SIGINT, js_sigint_handler);

   ///////////////////////////////
   // setup the freej context
   env->osd.active = false;


   return;
}

/* return lines read, or 0 on error */
int JsParser::open(const char* script_file) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  jsval res;
  JSString *str;
  JSBool eval_res = JS_TRUE;
  FILE *fd;
  char *buf;
  int len;

  char header[1024];

  fd = fopen(script_file,"r");
  if(!fd) {
    error("%s: %s : %s",__func__,script_file,strerror(errno));
    return 0;
  }

  // read all the file in once: line by line won't work well in blocks
  func("%s reading from file %s",__func__,script_file);
  fseek(fd,0,SEEK_END);
  len = ftell(fd);
  rewind(fd);

  // exclude the first line if it calls the shell interpreter
  fgets(header,1023,fd);
  if(header[0]!='#')
    rewind(fd);
  else
    len -= strlen(header);

  buf = (char*)calloc(len+128,sizeof(char));
  func("JsParser allocated %u bytes",len);
  fread(buf,len,sizeof(char),fd);

  fclose(fd);

  res = JSVAL_VOID;
  eval_res = JS_EvaluateScript(js_context, global_object,
		      buf, len, script_file, 0, &res);
  // return the result (to console)
  if(!JSVAL_IS_VOID(res)){
      str=JS_ValueToString(js_context, res);
      if(str){
          act("JS open result: %s", JS_GetStringBytes(str));
      } else {
          JS_ReportError(js_context, "Can't convert result msg to string");
      }
  } // else
    // if anything more was wrong, our ErrorReporter was called!
  func("%s evalres: %i", __func__, eval_res);
  return eval_res;
}

int JsParser::parse(const char *command) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  JSBool eval_res = JS_TRUE;
  jsval res;
  JSString *str;

  if(!command) { /* true paranoia */
    warning("NULL command passed to javascript parser");
    return 0;
  }

  func("JsParser::parse : %s",command);

  res = JSVAL_VOID;
  eval_res = JS_EvaluateScript(js_context, global_object,
		      command, strlen(command), "console", 0, &res);
  // return the result (to console)
  if(!JSVAL_IS_VOID(res)){
      str=JS_ValueToString(js_context, res);
      if(str){
          act("JS parse res: %s", JS_GetStringBytes(str));
      } else {
          JS_ReportError(js_context, "Can't convert result to string");
      }
  } // else
    // if anything more was wrong, our ErrorReporter was called!
  func("%s evalres: %i", __func__, eval_res);
  return eval_res;
}

void JsParser::stop() {
    stop_script=true;
}
