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

#include <callbacks_js.h>
#include <jsparser_data.h>
#include <config.h>

#if defined WITH_TEXTLAYER
#include <text_layer.h>


class TextLayerJS: public TextLayer {
public:
    TextLayerJS();
};

TextLayerJS::TextLayerJS() {
    jsclass = &txt_layer_class;
}

DECLARE_CLASS_GC("TextLayer", txt_layer_class, txt_layer_constructor, js_layer_gc);


////////////////////////////////
// Txt Layer methods
JSFunctionSpec txt_layer_methods[] = {
    ENTRY_METHODS  ,
    //   name            native                  nargs
    {    "print",        txt_layer_print,        1},
    {    "color",        txt_layer_color,        3},
    {    "font",         txt_layer_font,         1},
    {    "size",         txt_layer_size,         1},
    {    "calculate_size", txt_layer_calculate_size, 1},
    {0}
};


JS_CONSTRUCTOR("TextLayer", txt_layer_constructor, TextLayerJS);

JS(txt_layer_color) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    //JS_SetContextThread(cx);
    JS_BeginRequest(cx);
    JS_CHECK_ARGC(1);

    GET_LAYER(TextLayer);

    // color accepts arguments in many ways
    // R,G,B,A or R,G,B or the whole 32bit value
    //  uint32_t r,g,b,a;

    if(JSVAL_IS_DOUBLE(argv[0])) {
        //double *hex;
        //hex = JSVAL_TO_DOUBLE(argv[0]);

        warning("TODO: assign colors to text layer in hex form");
        //    lay->color = (uint32_t)*hex;

    } else {

        lay->set_fgcolor(JSVAL_TO_INT(argv[0]),
                         JSVAL_TO_INT(argv[1]),
                         JSVAL_TO_INT(argv[2]));

        //    lay->color = 0x0|(r<<8)|(g<<16)|(b<<24);
    }
    JS_EndRequest(cx);
    //JS_ClearContextThread(cx);
    return JS_TRUE;
}

JS(txt_layer_print) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    //JS_SetContextThread(cx);
    JS_BeginRequest(cx);
    JS_CHECK_ARGC(1);

    GET_LAYER(TextLayer);

    char *str = js_get_string(argv[0]);
    JS_EndRequest(cx);
    //JS_ClearContextThread(cx);
    lay->write(str);

    return JS_TRUE;
}
JS(txt_layer_size) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    if(argc < 1)
        return JS_FALSE;
    //JS_SetContextThread(cx);
    JS_BeginRequest(cx);
    GET_LAYER(TextLayer);
    JS_EndRequest(cx);
    //JS_ClearContextThread(cx);
    jsint size = js_get_int(argv[0]);

    lay->set_fontsize(size);

    return JS_TRUE;
}
JS(txt_layer_font) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    if(argc < 1)
        return JS_FALSE;
    //JS_SetContextThread(cx);
    JS_BeginRequest(cx);
    GET_LAYER(TextLayer);

    const char *font = js_get_string(argv[0]);
    JS_EndRequest(cx);
    //JS_ClearContextThread(cx);
    // try full path to .ttf file
    if(lay->set_font(font))
        *rval = JSVAL_TRUE;
    else
        *rval = JSVAL_FALSE;

    return JS_TRUE;
}

JS(txt_layer_calculate_size) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    if(argc < 1)
        return JS_FALSE;
    //JS_SetContextThread(cx);
    JS_BeginRequest(cx);
    GET_LAYER(TextLayer);

    int w, h;
    char *text;
    JSObject *arr;
    jsdouble num;
    jsval val;

    text = js_get_string(argv[0]);

    lay->calculate_string_size(text, &w, &h);

    arr = JS_NewArrayObject(cx, 0, NULL); // create a void array
    if(!arr) {
        error("error generating array");
        return JS_FALSE;
    }

    // fill the array with [0]w [1]h
    num = JS_NewNumberValue(cx, (double)w, &val);
    if(!num) warning("Can't fill the array");
    JS_SetElement(cx, arr, 0, &val);
    num = JS_NewNumberValue(cx, (double)h, &val);
    if(!num) warning("Can't fill the array");
    JS_SetElement(cx, arr, 1, &val);

    *rval = OBJECT_TO_JSVAL(arr);
    JS_EndRequest(cx);
    //JS_ClearContextThread(cx);
    return JS_TRUE;
}

#endif
