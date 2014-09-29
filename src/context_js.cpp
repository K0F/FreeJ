/*  FreeJ
 *  (c) Copyright 2001-2009 Denis Roio <jaromil@dyne.org>
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
 *
 *
 */

#include <dirent.h>
#include <assert.h>

#include <callbacks_js.h>
#include <jsparser_data.h>
#include <jsparser.h>
#include <video_encoder.h>
#include <controller.h>
#include <algorithm>
//#include <fps.h>

// global environment class
JSClass global_class = {
    "Freej", JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS | JSCLASS_GLOBAL_FLAGS,
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
    {"add_screen",      add_screen,             1},
    {"rem_screen",      rem_screen,             1},
    {"add_layer",       ctx_add_layer,          1},
    //    {"selected_layer",  selected_screen,        0},
    {"debug",           debug,                  1},
    {"set_debug",       js_set_debug,           0},
    {"rand",            rand,                   0},
    {"srand",           srand,                  1},
    {"pause",           pause,                  0},
    {"fullscreen",      fullscreen,             0},
    {"set_clear_all",   set_clear_all,          0},
    {"unset_clear_all", unset_clear_all,        0},
    {"set_fps",         set_fps,                1},
    {"get_width",       get_width,              0},
    {"get_height",      get_height,             0},
    {"set_resolution",  set_resolution,         2},
    {"scandir",         freej_scandir,          1},
    {"echo",            freej_echo,             1},
    {"echo_func",       freej_echo_func,        1},
    {"strstr",          freej_strstr,           2},
    //    {"stream_start",    stream_start,           0},
    //    {"stream_stop",     stream_stop,            0},
    {"read_file", read_file, 1},
    {"file_to_strings", file_to_strings,        1},
    {"register_controller", register_controller, 1},
    {"rem_controller",  rem_controller, 1},
#ifdef WITH_OGGTHEORA
    {"register_encoder", register_encoder, 1},
#endif
    {"include",         include_javascript_file, 1},
    {"use",             execute_javascript_command, 1},
    {"exec",            system_exec,            1},
    {"list_filters",    list_filters,           0},
    {"gc",              js_gc,                  0},
    {"reset",           reset_js,               0},
    {0}
};

JS(js_gc) {
    //JS_GC(cx);
    return JS_TRUE;
    //    env->js->gc();
}

JS(cafudda) {
    //  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
    double *pdouble;
    double seconds = 0.0;
    int isecs;

    if(argc > 0) {

        if(JSVAL_IS_DOUBLE(argv[0])) {

            // JSVAL_TO_DOUBLE segfault when there's an int as input
            pdouble = JSVAL_TO_DOUBLE(argv[0]);
            seconds = *pdouble;

        } else if(JSVAL_IS_INT(argv[0])) {

            isecs = JSVAL_TO_INT(argv[0]);
            seconds = (double)isecs;

        }

    } else seconds = 0;


    //  func("JsParser :: run for %f seconds",seconds);
    global_environment->cafudda(seconds);

    return JS_TRUE;
}

JS(pause) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    global_environment->pause = !global_environment->pause;

    return JS_TRUE;
}

JS(quit) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    global_environment->quit = true;
    return JS_TRUE;
}


JS(rem_screen) {
    func("%s", __PRETTY_FUNCTION__);
    JSObject *tmp_jsobj;
    ViewPort *scr;

    if(argc < 1)
        JS_ERROR("missing argument");
    //    js_is_instanceOf(&layer_class, argv[0]);

    tmp_jsobj = JSVAL_TO_OBJECT(argv[0]);
    scr = (ViewPort *) JS_GetPrivate(cx, tmp_jsobj);
    if(!scr)
        JS_ERROR("Screen core data is NULL");

    //    global_environment->rem_screen(scr);
    warning("Context::rem_screen TODO");
    return JS_TRUE;
}

JS(add_screen) {
    func("%s", __PRETTY_FUNCTION__);
    ViewPort *scr;
    JSObject *tmp_jsobj;
    *rval = JSVAL_FALSE;

    if(argc < 1)
        JS_ERROR("missing argument");
    //    _js_is_instanceOf(global_environment->js->global_context, &layer_class, argv[0], "Context");

    tmp_jsobj = JSVAL_TO_OBJECT(argv[0]);
    scr = (ViewPort *) JS_GetPrivate(cx, tmp_jsobj);
    if(!scr)
        JS_ERROR("Screen core data is NULL");

    /** really add screen */
    if(global_environment->add_screen(scr)) {
        *rval = JSVAL_TRUE;
    } else {
        *rval = JSVAL_FALSE;
    }

    return JS_TRUE;
}

JS(ctx_add_layer) {
    func("%s", __PRETTY_FUNCTION__);

    JSObject *jslayer = NULL;
    Layer *lay;

    if(argc < 1)
        JS_ERROR("missing argument");
    //  js_is_instanceOf(&layer_class, argv[0]);

    jslayer = JSVAL_TO_OBJECT(argv[0]);
    lay = (Layer*) JS_GetPrivate(cx, jslayer);
    if(!lay)
        JS_ERROR("Layer is NULL");

    if(global_environment->add_layer(lay)) {
        *rval = JSVAL_TRUE;
    } else {
        *rval = JSVAL_FALSE;
    }

    return JS_TRUE;
}

JS(list_filters) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    JSObject *arr;
    JSString *str;
    jsval val;


    arr = JS_NewArrayObject(cx, 0, NULL); // create void array
    if(!arr) return JS_FALSE;

    int c = 0;
    LockedLinkList<Filter> list = global_environment->filters.getLock();
    std::for_each(list.begin(), list.end(), [&](Filter *&f) {
                      str = JS_NewStringCopyZ(cx, f->getName().c_str());
                      val = STRING_TO_JSVAL(str);
                      JS_SetElement(cx, arr, c, &val);
                      c++;
                  });
    *rval = OBJECT_TO_JSVAL(arr);
    return JS_TRUE;
}

JS(register_controller) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    Controller *ctrl;
    JSObject *jsctrl;
    *rval = JSVAL_FALSE;

    if(argc < 1)
        JS_ERROR("missing argument");
    //    _js_is_instanceOf(global_environment->js->global_context, &js_ctrl_class, argv[0], "Context");

    jsctrl = JSVAL_TO_OBJECT(argv[0]);
    ctrl = (Controller *)JS_GetPrivate(cx, jsctrl);
    if(!ctrl)
        JS_ERROR("Controller core data is NULL");

    /// really add controller
    global_environment->register_controller(ctrl);
    *rval = JSVAL_TRUE;
    func("JSvalcmp: %p / %p", argv[0], ctrl->data);

    return JS_TRUE;
}

JS(rem_controller) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    JSObject *jsctrl;
    Controller *ctrl;

    if(argc < 1)
        JS_ERROR("missing argument");
    //    js_is_instanceOf(&js_ctrl_class, argv[0]);

    jsctrl = JSVAL_TO_OBJECT(argv[0]);
    ctrl = (Controller *) JS_GetPrivate(cx, jsctrl);
    if(!ctrl)
        JS_ERROR("Layer core data is NULL");

    func("JSvalcmp: %p / %p", argv[0], ctrl->data);
    global_environment->rem_controller(ctrl);
    return JS_TRUE;
}

#ifdef WITH_OGGTHEORA
JS(register_encoder) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    VideoEncoder *enc;
    JSObject *jsenc;
    *rval = JSVAL_FALSE;

    if(argc < 1)
        JS_ERROR("missing argument");
    //    js_is_instanceOf(&js_vid_enc_class, argv[0]);

    jsenc = JSVAL_TO_OBJECT(argv[0]);
    enc = (VideoEncoder *)JS_GetPrivate(cx, jsenc);
    if(!enc)
        JS_ERROR("VideoEncoder core data is NULL");

    //    enc->start();

    /// really add controller
    global_environment->add_encoder(enc);

    *rval = JSVAL_TRUE;

    return JS_TRUE;
}
#endif

JS(fullscreen) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    global_environment->mSelectedScreen->fullscreen();
    //  global_environment->clear_all = !global_environment->clear_all;
    return JS_TRUE;
}

JS(set_clear_all) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    global_environment->clear_all = true;
    return JS_TRUE;
}

JS(unset_clear_all) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    global_environment->clear_all = false;
    return JS_TRUE;
}

JS(set_fps) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    jsint fps = js_get_int(argv[0]);

    global_environment->fps.set(fps);
    return JS_TRUE;
}

JS(js_set_debug) {
    JSBool ret = JS_NewNumberValue(cx, get_debug(), rval);
    if(argc == 1) {
        jsint level = js_get_int(argv[0]);
        set_debug(level);
    }
    return ret;
}

JS(get_width) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    *rval = INT_TO_JSVAL(global_environment->screens.getLock().front()->geo.w);
    return JS_TRUE;
}

JS(get_height) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    *rval = INT_TO_JSVAL(global_environment->screens.getLock().front()->geo.h);
    return JS_TRUE;
}

JS(set_resolution) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    int w = JSVAL_TO_INT(argv[0]);
    int h = JSVAL_TO_INT(argv[1]);
    global_environment->mSelectedScreen->resize(w, h);
    return JS_TRUE;
}

/*
   JS(stream_start) {
   func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
   notice ("Streaming to %s:%u",global_environment->shouter->host(), global_environment->shouter->port());
   act ("Saving to %s", env -> video_encoder -> get_filename());
   global_environment->save_to_file = true;
   return JS_TRUE;
   }
   JS(stream_stop) {
   func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);
   ::notice ("Stopped stream to %s:%u", global_environment->shouter->host(), global_environment->shouter->port());
   ::act ("Video saved in file %s",env -> video_encoder -> get_filename());
   global_environment->save_to_file = false;
   return JS_TRUE;
   }
 */
#if defined (HAVE_DARWIN) || defined (HAVE_FREEBSD)
static int dir_selector(struct dirent *dir)
#else
static int dir_selector(const struct dirent *dir)
#endif
{
    if(dir->d_name[0] == '.') return(0);  // remove hidden files
    return(1);
}

JS(freej_scandir) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    JSObject *arr;
    JSString *str;
    jsval val;
#if defined (HAVE_DARWIN) || defined (HAVE_FREEBSD)
    struct dirent **filelist;
#else
    struct dirent **filelist;
#endif
    int found;
    int c = 0;

    char *dir = js_get_string(argv[0]);

    found = scandir(dir, &filelist, dir_selector, alphasort);
    if(found < 0) {
        error("scandir error: %s", strerror(errno));
        return JS_TRUE; // fatal error
    }

    arr = JS_NewArrayObject(cx, 0, NULL); // create void array
    if(!arr) return JS_FALSE;

    // now fill up the array
    while(found--) {
        char tmp[512];
        snprintf(tmp, 512, "%s/%s", dir, filelist[found]->d_name);
        free(filelist[found]);
        str = JS_NewStringCopyZ(cx, tmp);
        val = STRING_TO_JSVAL(str);
        JS_SetElement(cx, arr, c, &val);
        c++;
    }
    free(filelist);

    *rval = OBJECT_TO_JSVAL(arr);
    return JS_TRUE;
}

JS(freej_echo) {
    char *msg = js_get_string(argv[0]);
    fprintf(stdout, "%s\n", msg);
    return JS_TRUE;
}

JS(freej_echo_func) {
    char *msg = js_get_string(argv[0]);
    func("%s", msg);
    return JS_TRUE;
}

JS(freej_strstr) {
    char *res;
    int intval;

    char *haystack = js_get_string(argv[0]);
    char *needle = js_get_string(argv[1]);

    res = strstr(haystack, needle);
    if(res == NULL)
        intval = 0;
    else intval = 1;
    return JS_NewNumberValue(cx, intval, rval);
}

JS(read_file) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    JS_CHECK_ARGC(1);

    JSString *str;

    FILE *fd;

    int len;
    char *buf;

    char *file = js_get_string(argv[0]);

    // try to open the file
    fd = ::fopen(file, "r");
    if(!fd) {
        error("read_file failed for %s: %s", file, strerror(errno));
        *rval = JSVAL_NULL;
        return JS_TRUE;
    }

    // read it all in *buf
    fseek(fd, 0, SEEK_END);
    len = ftell(fd);
    rewind(fd);
    buf = (char*)calloc(len, sizeof(char));
    fread(buf, len, 1, fd);
    fclose(fd);
    // file is now read in memory

    // create the new string
    str = JS_NewStringCopyN(cx, buf, len);
    // cast it into a js return value
    *rval = STRING_TO_JSVAL(str);

    act("file loaded: %s", file);

    return JS_TRUE;
}



JS(file_to_strings) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    JS_CHECK_ARGC(1);

    JSObject *arr;
    JSString *str;
    jsval val;

    FILE *fd;

    char *buf;
    char *punt;
    char *pword;
    int len;
    int c;

    char *file = js_get_string(argv[0]);

    // try to open the file and read it in memory
    fd = ::fopen(file, "r");
    if(!fd) {
        error("file_to_strings failed for %s: %s", file, strerror(errno));
        *rval = JSVAL_NULL;
        return JS_TRUE;
    }

    // read it all in *buf
    fseek(fd, 0, SEEK_END);
    len = ftell(fd);
    rewind(fd);
    buf = (char*)calloc(len, sizeof(char));
    fread(buf, len, 1, fd);
    fclose(fd);
    // file is now read in memory

    arr = JS_NewArrayObject(cx, 0, NULL);
    if(!arr) return JS_FALSE;

    punt = buf;
    c = 0;
    // now fill up the array

    while(punt - buf < len) { // parse it until the end
        while(!isgraph(*punt)) // goes forward until it meets a word
            if(punt - buf >= len) // end of chunk reached
                break;
            else punt++;

        // word found, now reach its end
        pword = punt;
        while(isgraph(*punt)
              && *punt != ' '
              && *punt != '\0'
              && *punt != '\n'
              && *punt != '\r'
              && *punt != '\t') {
            if(punt - buf >= len) // end of chunk reached
                break;
            else punt++;
        }

        // there is a word to acquire!
        // create the new entry
        str = JS_NewStringCopyN(cx, pword, punt - pword);
        val = STRING_TO_JSVAL(str);
        JS_SetElement(cx, arr, c, &val);
        c++;
    }

    free(buf);

    *rval = OBJECT_TO_JSVAL(arr);
    return JS_TRUE;
}

// debugging commodity
// run freej with -D3 to see this
JS(debug) {
    char *msg = js_get_string(argv[0]);

    func("%s", msg);

    return JS_TRUE;
}

static uint32_t randval;
JS(rand) {
    //  func("%u:%s:%s",__LINE__,__FILE__,__FUNCTION__);

    randval = randval * 1073741789 + 32749;

    return JS_NewNumberValue(cx, randval, rval);
    /*
       r = rand();

       if(argc<1) *rval = 1+(int)(r/(RAND_MAX+1.0));
       else {
       jsdouble max = js_get_double(argv[0]);
       JS_ARG_NUMBER(max, 0);
       func("randomizing with max %f",max);
       r = 1+(int)(max*r/(RAND_MAX+1.0));
       *rval = INT_TO_JSVAL(r);
       }
     */
}

JS(srand) {
    // this is not fast and you'd better NOT use it often
    // to achieve more randomization on higher numbers:
    // u get unpredictably sloow
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    int seed;
    jsint r;
    if(argc < 1)
        seed = time(NULL);
    else {
        r = js_get_int(argv[0]);
        seed = r;
    }
    randval = seed;

    return JS_TRUE;
}


////////////////////////////////
// Linklist Entry Methods

JS(entry_down) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    GET_LAYER(Layer);

    if(!lay->down())
        warning("cannot move %s down", lay->getName().c_str());

    return JS_TRUE;
}
JS(entry_up) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    GET_LAYER(Layer);

    if(!lay->up())
        warning("cannot move %s up", lay->getName().c_str());

    return JS_TRUE;
}


JS(entry_move) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    GET_LAYER(Layer);

    int pos = JSVAL_TO_INT(argv[0]);
    if(!lay->move(pos))
        warning("cannot move %s to position %u", lay->getName().c_str(), pos);

    return JS_TRUE;
}

JS(entry_next) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    Layer *res = NULL;
    JSObject *objtmp;

    GET_LAYER(Layer);

    if(lay->screen != NULL) {
        LockedLinkList<Layer> list = lay->screen->layers.getLock();
        LockedLinkList<Layer>::iterator layerIt = std::find(list.begin(), list.end(), lay);
        assert(layerIt != list.end());
        if((++layerIt) != list.end()) {
            res = *layerIt;
        }
    }

    objtmp = JS_NewObject(cx, res->jsclass, NULL, obj);
    JS_SetPrivate(cx, objtmp, (void*) res);

    *rval = OBJECT_TO_JSVAL(objtmp);

    return JS_TRUE;
}

JS(entry_prev) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    Layer *res = NULL;
    JSObject *objtmp;

    GET_LAYER(Layer);

    if(lay->screen != NULL) {
        LockedLinkList<Layer> list = lay->screen->layers.getLock();
        LockedLinkList<Layer>::iterator layerIt = std::find(list.begin(), list.end(), lay);
        assert(layerIt != list.end());
        if(layerIt != list.begin()) {
            res = *(--layerIt);
        }
    }

    objtmp = JS_NewObject(cx, res->jsclass, NULL, obj);
    JS_SetPrivate(cx, objtmp, (void*) res);

    *rval = OBJECT_TO_JSVAL(objtmp);

    return JS_TRUE;
}

JS(include_javascript_file) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    if(argc < 1)
        JS_ERROR("missing argument");

    char *jscript;
    jscript = js_get_string(argv[0]);

    JsParser *js = (JsParser *)JS_GetContextPrivate(cx);

    if(!js->include(cx, jscript))
        error("javascript include not found: \"%s\"", jscript);

    // if its the first script loaded, save it as main one
    // this is then used in reset to return at beginning stage
    //	if(global_environment->main_javascript[0] == 0x0)
    //	  memcpy(global_environment->main_javascript, temp, 512);

    return JS_TRUE;
}

JS(execute_javascript_command) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    char *jscript;
    jsval use;

    if(argc < 1)
        JS_ERROR("missing argument");
    jscript = js_get_string(argv[0]);
    JsParser *js = (JsParser *)JS_GetContextPrivate(cx);

    use = js->use(cx, obj, jscript);
    if(use == JS_FALSE) {
        // all errors already reported,
        // js->open talks too much
        error("JS include('%s') failed", jscript);
        return JS_FALSE;
    }
    *rval = use;
    return JS_TRUE;
}

// XXX - are we sure that we really want to expose such a function? :/
JS(system_exec) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    unsigned int c;
    char *prog;
    char **args;

    // get the executable program
    prog = js_get_string(argv[0]);

    // get the arguments in a NULL terminated array
    args = (char**)calloc(argc + 1, sizeof(char*));

    for(c = 0; c < argc; c++) {

        if(JSVAL_IS_STRING(argv[c]))

            args[c] = (char*)JS_GetStringBytes(JS_ValueToString(cx, argv[c]));

        else {

            JS_ReportError(cx, "%s: argument %u is not a string", __FUNCTION__, c);
            global_environment->quit = true;
            return JS_FALSE;

        }

    }

    if(!fork()) {
        /* execvp(3) functions provide an array of pointers to
           null-terminated strings that represent the argument list
           available to the new program.  The first argument, by convention,
           should point to the file name associated with the file being
           executed.  The array of pointers must be terminated by a NULL
           pointer.  */
        execvp(prog, args);
    }

    return JS_TRUE;
}

JS(reset_js) {
    func("%s", __PRETTY_FUNCTION__);
    //	char *jscript;

    *rval = JSVAL_TRUE;
    //JsParser *js = (JsParser *)JS_GetContextPrivate(cx);
    func("resetting freej context");
    global_environment->reset();
    //func("reloading main script: %s", global_environment->main_javascript);
    //	js->open(global_environment->main_javascript);

    //	func("garbage collection of jsparser");
    //	js->reset();

//      if(argc == 1) {
//	char *jscript = js_get_string(argv[0]);
//              if (js->open(jscript) == 0) {
//                      error("JS reset('%s') failed", jscript);
//                      *rval = JSVAL_FALSE;
//                      return JS_FALSE;
//              }
//      }
//      JS_GC(cx);
    // if called by a controller, it must then return true
    // otherwise rehandling it's event can be endless loop
    return JS_TRUE;
}

