/*  FreeJ
 *  (c) Copyright 2001 Denis Roio aka jaromil <jaromil@dyne.org>
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

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <context.h>

#include <sdl_screen.h>

#include <jutils.h>
#include <config.h>

Context::Context() {

  notice("starting %s %s engine",PACKAGE,VERSION);

  screen = NULL;

  /* initialize fps counter */
  framecount=0; fps=0.0;
  track_fps = false;
  magnification = 0;
  changeres = false;
  clear_all = false;
  quit = false;
  pause = false;
}

Context::~Context() {
  close();
  notice("cu on http://freej.dyne.org");
}

bool Context::init(int wx, int hx) {

    js = new JsParser(this);
    js->open("refujees.js");
  
  screen = new SdlScreen();
  if(!screen->init(wx,hx)) {
    error("Can't initialize the viewport");
    error("This is a fatal error");
    return(false);
  }
  
  /* refresh the list of available plugins */
  plugger.refresh();
  
  /* initialize the On Screen Display */
  osd.init(this);
  osd.active = true;
  set_osd(osd.status_msg); /* let jutils know about the osd */

  /* initialize the Keyboard Listener */
  kbd.init(this);

#ifdef WITH_JAVASCRIPT
  /* initialize the Console Parser */
  console.init(this);
#endif
  
  /* initialize the realtime clock and use it if present */
  rtc = false;
#ifdef linux
  if(rtc_open()) rtc = true;
#endif

  set_fps_interval(24);
  gettimeofday( &lst_time, NULL);

  return true;
}

void Context::close() {
  Layer *lay;
  if(screen) delete screen;

  delete js;

  lay = (Layer *)layers.begin();
  while(lay) {
    lay->lock();
    layers.rem(1);
    lay->quit = true;
    lay->signal_feed();
    lay->unlock();
    SDL_Delay(500);
    delete lay;
    lay = (Layer *)layers.begin();
  }

  plugger.close();

  if(rtc) rtc_close();
}  

void Context::cafudda(int secs) {
  Layer *lay;

  if(secs) /* if secs =0 will go out after one cycle */

    /* initialize timing */
#ifdef linux
    if(rtc) { rtc_now = secs; }
    else
#endif
      { now = dtime(); }
  
  do {

    /* fetch keyboard events */
    kbd.run();

    js->parse();
    
    rocknroll(true);
    
    if(clear_all) screen->clear();
    else if(osd.active) osd.clean();
    
    
    lay = (Layer *)layers.end();
    //    if(!lay) {
    //      osd._print_credits();
    //      osd._show_fps();
    //    } else {
      layers.lock();
      while(lay) {
	if(!pause) 
	  lay->cafudda();
	lay = (Layer *)lay->prev;
      }
      layers.unlock();
      //    }
    
    osd.print();
    
    screen->show();
    
    /* change resolution if needed */
    if(changeres) {
      screen->set_magnification(magnification);
      osd.init(this);
      /* crop all layers to new screen size */
      Layer *lay = (Layer *)layers.begin();
      while(lay) {
	lay->lock();
	screen->crop(lay);
	lay->unlock();
	lay = (Layer*)lay->next;
      }
      changeres = false;
    }

    /******* handle timing */
    
    if(!secs) break; /* just one pass */
    
#ifdef linux
    if(rtc) {
      if(!rtc_tick()) riciuca = false; // a second passed
      else {
	rtc_now--;
	riciuca = (rtc_now>0) ? true : false;
      }
    } else
#endif
      {
	riciuca = (dtime()-now<secs) ? true : false;
      }    
    
    calc_fps();
	
  } while(riciuca);
}

void Context::magnify(int algo) {
  if(magnification == algo) return;
  magnification = algo;
  changeres = true;
}

/* FPS */

void Context::set_fps_interval(int interval) {
  fps_frame_interval = interval*1000000;
  min_interval = (long)1000000/interval;
}

void Context::calc_fps() {
  /* 1frame : elapsed = X frames : 1000000 */
  gettimeofday( &cur_time, NULL);
  elapsed = cur_time.tv_usec - lst_time.tv_usec;
  if(cur_time.tv_sec>lst_time.tv_sec) elapsed+=1000000;
  
  if(track_fps) {
    framecount++;
    if(framecount==24) {
      fps=(double)1000000/elapsed;
      framecount=0;
    }
  }

  if(elapsed<=min_interval) {
    /* usleep( min_interval - elapsed ); this is not POSIX, arg */
    slp_time.tv_sec = 0;
    slp_time.tv_nsec = (min_interval - elapsed)<<10;
    nanosleep(&slp_time,NULL);

    lst_time.tv_usec += min_interval;
    if( lst_time.tv_usec > 999999) {
      lst_time.tv_usec -= 1000000;
      lst_time.tv_sec++;
    }
  } else {
    lst_time.tv_usec = cur_time.tv_usec;
    lst_time.tv_sec = cur_time.tv_sec;
  }

}

void Context::rocknroll(bool state) {

  Layer *l = (Layer *)layers.begin();

  if(!l) { osd.credits(true); return; }

  layers.lock();
  while(l) {
    if(!l->running) {
      l->start();
      //    l->signal_feed();
      while(!l->running) jsleep(0,500);
      l->active = state;
    }
    l = (Layer *)l->next;
  }
  layers.unlock();
}
