/*  FreeJ
 *  (c) Copyright 2008 Denis Rojo <jaromil@dyne.org>
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
 * $Id $
 *
 */

#include <context.h>
#include <jutils.h>

#include <callbacks_js.h>
#include <jsparser_data.h>

#include <audio_collector.h>

/// Javascript audio collector
JS(js_audio_jack_constructor);
void js_audio_jack_gc(JSContext *cx, JSObject *obj);

DECLARE_CLASS_GC("AudioJack", js_audio_jack_class,
		 js_audio_jack_constructor, js_audio_jack_gc)

JS(js_audio_jack_add_output);
JS(js_audio_jack_get_harmonic);
JS(js_audio_jack_fft);

JSFunctionSpec js_audio_jack_methods[] = {
  {"add_output", js_audio_jack_add_output, 1},
  {"get_harmonic", js_audio_jack_get_harmonic, 1},
  {"fft", js_audio_jack_fft, 0},
  {0}
};

JS(js_audio_jack_constructor) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

  char excp_msg[MAX_ERR_MSG + 1];
  char *port;

  JS_CHECK_ARGC(3);


  JS_ARG_STRING(port,0);
  JS_ARG_NUMBER(sample,1);
  JS_ARG_NUMBER(rate,2);
  
  AudioCollector *audio = new AudioCollector(port, (int)sample, (int)rate);

  if( ! JS_SetPrivate(cx, obj, (void*)audio) ) {
    sprintf(excp_msg, "failed assigning audio jack to javascript");
    goto error;
  }
 
  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;

 error:
  JS_ReportErrorNumber(cx, JSFreej_GetErrorMessage, NULL,
		       JSSMSG_FJ_CANT_CREATE, __func__, excp_msg);
  //  cx->newborn[GCX_OBJECT] = NULL;
  delete audio;
  return JS_FALSE;
}

JS(js_audio_jack_add_output) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  char excp_msg[MAX_ERR_MSG + 1];
  Layer *lay;
  AudioCollector *audio;
  JSObject *jslayer;

  JS_CHECK_ARGC(1);
  js_is_instanceOf(&layer_class, argv[0]);

  audio = (AudioCollector*)JS_GetPrivate(cx, obj);
  if(!audio) {
    sprintf(excp_msg, "audio collector core data is NULL");
    goto error;
  }

  jslayer = JSVAL_TO_OBJECT(argv[0]);
  lay = (Layer*)JS_GetPrivate(cx, jslayer);
  if(!lay) {
    sprintf(excp_msg, "audio add_output called on an invalid object");
    goto error;
  }

  // assign the audio collector to the layer
  lay->audio = audio;

  return JS_TRUE;

 error:
  JS_ReportErrorNumber(cx, JSFreej_GetErrorMessage, NULL,
		       JSSMSG_FJ_CANT_CREATE, __func__, excp_msg);
  return JS_FALSE;
}

JS(js_audio_jack_get_harmonic) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  char excp_msg[MAX_ERR_MSG + 1];
  float harmonic;

  JS_CHECK_ARGC(1);
  
  JS_ARG_NUMBER(hc,0);
  
  AudioCollector *audio = (AudioCollector*)JS_GetPrivate(cx, obj);
  if(!audio) {
    sprintf(excp_msg, "audio collector core data is NULL");
    goto error;
  }

  harmonic = audio->GetHarmonic((int)hc);

  return JS_NewNumberValue(cx, (double)harmonic, rval);
  
 error:
  JS_ReportErrorNumber(cx, JSFreej_GetErrorMessage, NULL,
		       JSSMSG_FJ_CANT_CREATE, __func__, excp_msg);
  return JS_FALSE;
}

JS(js_audio_jack_fft) {
  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
  char excp_msg[MAX_ERR_MSG + 1];

  AudioCollector *audio = (AudioCollector*)JS_GetPrivate(cx, obj);
  if(!audio) {
    sprintf(excp_msg, "audio collector core data is NULL");
    goto error;
  }
  
  audio->GetFFT();

  return JS_TRUE;

 error:
  JS_ReportErrorNumber(cx, JSFreej_GetErrorMessage, NULL,
		       JSSMSG_FJ_CANT_CREATE, __func__, excp_msg);
  return JS_FALSE;
}

void js_audio_jack_gc(JSContext *cx, JSObject *obj) {
  func("%s",__PRETTY_FUNCTION__);
  char excp_msg[MAX_ERR_MSG + 1];

  AudioCollector *audio = (AudioCollector*)JS_GetPrivate(cx, obj);
  if(!audio) return;

  delete audio;

}
  
