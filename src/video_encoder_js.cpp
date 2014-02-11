/*  FreeJ
 *  (c) Copyright 2007 Denis Rojo <jaromil@dyne.org>
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

#ifdef WITH_OGGTHEORA

#include <video_encoder.h>
#include <oggtheora_encoder.h>

DECLARE_CLASS("VideoEncoder", js_vid_enc_class, js_vid_enc_constructor);

////////////////////////////////
// Video Encoder methods
JSFunctionSpec js_vid_enc_methods[] = {
    { "start_filesave", vid_enc_start_filesave, 1},
    { "stop_filesave",  vid_enc_stop_filesave,  0},
    { "start_stream", start_stream, 0},
    { "stop_stream", stop_stream, 0},

    { "add_audio", vid_enc_add_audio, 1 },

    { "stream_host",   stream_host,  1},
    { "stream_port",   stream_port,  1},
    { "stream_mountpoint",  stream_mount, 1},
    { "stream_title",   stream_title, 1},
    { "stream_username", stream_username, 1},
    { "stream_password", stream_password, 1},
    { "stream_homepage", stream_homepage, 1},
    { "stream_description", stream_description, 1},

    {0}
};

JS(js_vid_enc_constructor) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);
    OggTheoraEncoder *enc;
    enc = new OggTheoraEncoder();
    //  Theora11Encoder *enc;
    //  enc = new Theora11Encoder();

    if(!enc) {
        error("JS::VideoEncoder : error constructing ogg theora video encoder");
        return JS_FALSE;
    }

    if(argc >= 1)

        enc->video_quality = JSVAL_TO_INT(argv[0]);

    if(argc >= 2)

        enc->video_bitrate = JSVAL_TO_INT(argv[1]);

    if(argc >= 3)

        enc->audio_quality = JSVAL_TO_INT(argv[2]);

    if(argc >= 4)

        enc->audio_bitrate = JSVAL_TO_INT(argv[3]);


    // initialization is done with audio

    if(!JS_SetPrivate(cx, obj, (void*)enc)) {
        error("JS::VideoEncoder : can't set the private value");
        delete enc;
        return JS_FALSE;
    }
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

JS(vid_enc_add_audio) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    JS_CHECK_ARGC(1);
    //#ifdef WITH_AUDIO
    //  js_is_instanceOf(&js_audio_jack_class, argv[0]);
    //#endif

    JSObject *jsaudio;

    jsaudio = JSVAL_TO_OBJECT(argv[0]);
    AudioCollector *audio =
        (AudioCollector*) JS_GetPrivate(cx, jsaudio);


    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    enc->use_audio = true;
    enc->audio = audio;


    return JS_TRUE;
}

JS(vid_enc_start_filesave) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);


    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    JS_CHECK_ARGC(1);

    char *file = js_get_string(argv[0]);

    if(!enc->is_running()) enc->start();

    enc->set_filedump(file);

    return JS_TRUE;
}


JS(vid_enc_stop_filesave) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);


    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    enc->set_filedump(NULL);

    return JS_TRUE;
}

JS(start_stream) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }


    act("starting stream to server %s on port %u", shout_get_host(enc->ice), shout_get_port(enc->ice));

    if(!enc->is_running())
        enc->start();

    if(shout_open(enc->ice) == SHOUTERR_SUCCESS) {

        notice("streaming on url: http://%s:%i%s",
               shout_get_host(enc->ice), shout_get_port(enc->ice), shout_get_mount(enc->ice));

        enc->write_to_stream = true;

    } else {

        error("error connecting to server %s: %s",
              shout_get_host(enc->ice), shout_get_error(enc->ice));

        enc->write_to_stream = false;

    }

    return JS_TRUE;
}

JS(stop_stream) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    enc->write_to_stream = false;

    if(shout_close(enc->ice))
        error("shout_close: %s", shout_get_error(enc->ice));

    //  shout_sync(enc->ice);

    return JS_TRUE;
}

JS(stream_host) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    char *hostname = js_get_string(argv[0]);

    if(shout_set_host(enc->ice, hostname))
        error("shout_set_host: %s", shout_get_error(enc->ice));

    return JS_TRUE;
}

JS(stream_port) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    jsint port = js_get_int(argv[0]);

    if(shout_set_port(enc->ice, port))
        error("shout_set_port: %s", shout_get_error(enc->ice));

    return JS_TRUE;

}

JS(stream_mount) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    char *mount = js_get_string(argv[0]);

    if(shout_set_mount(enc->ice, mount))
        error("shout_set_mount: %s", shout_get_error(enc->ice));

    return JS_TRUE;

}

JS(stream_title) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }


    char *title = js_get_string(argv[0]);

    if(shout_set_name(enc->ice, title))
        error("shout_set_title: %s", shout_get_error(enc->ice));

    return JS_TRUE;

}

JS(stream_username) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }


    char *user = js_get_string(argv[0]);

    if(shout_set_user(enc->ice, user))
        error("shout_set_user: %s", shout_get_error(enc->ice));

    return JS_TRUE;

}

JS(stream_password) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    char *pass = js_get_string(argv[0]);

    if(shout_set_password(enc->ice, pass))
        error("shout_set_pass: %s", shout_get_error(enc->ice));

    return JS_TRUE;

}

JS(stream_homepage) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    char *url = js_get_string(argv[0]);

    if(shout_set_url(enc->ice, url))
        error("shout_set_url: %s", shout_get_error(enc->ice));

    return JS_TRUE;

}

JS(stream_description) {
    func("%u:%s:%s", __LINE__, __FILE__, __FUNCTION__);

    VideoEncoder *enc = (VideoEncoder*)JS_GetPrivate(cx, obj);
    if(!enc) {
        error("%u:%s:%s :: VideoEncoder core data is NULL",
              __LINE__, __FILE__, __FUNCTION__);
        return JS_FALSE;
    }

    char *desc = js_get_string(argv[0]);

    if(shout_set_description(enc->ice, desc))
        error("shout_set_descrition: %s", shout_get_error(enc->ice));

    return JS_TRUE;

}

#endif
