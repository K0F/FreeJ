/*  FreeJ
 *  (c) Copyright 2001-2002 Denis Rojo aka jaromil <jaromil@dyne.org>
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

//#include <SDL.h>

#include <layer.h>
#include <blitter.h>
#include <filter.h>
#include <iterator.h>
#include <closure.h>

#include <context.h>
#include <jutils.h>
#include <config.h>

#include <jsparser_data.h>


Layer::Layer()
  :Entry(), JSyncThread() {
  func("%s this=%p",__PRETTY_FUNCTION__, this);
  env = NULL;
  quit = false;
  active = false;
  hidden = false;
  fade = false;
  use_audio = false;
  audio = NULL;
  opened = true;
  bgcolor = 0;
  bgmatte = NULL;
  set_name("???");
  filename[0] = 0;
  buffer = NULL;
  offset = NULL;
  screen = NULL;
  is_native_sdl_surface = false;
  jsclass = &layer_class;
  slide_x = 0;
  slide_y = 0;

  null_feeds = 0;
  max_null_feeds = 10;

  parameters = NULL;
  running = false;
}

Layer::~Layer() {
  func("%s this=%p",__PRETTY_FUNCTION__, this);
  FilterInstance *f = (FilterInstance*)filters.begin();
  while(f) {
    //    f->rem(); rem is contained in delete for Entry
    delete f;
    f = (FilterInstance*)filters.begin();
  }
  if(bgmatte) { 
    jfree(bgmatte);
    bgmatte = NULL;
  }

}

void Layer::_init(int wdt, int hgt) {

  geo.w = wdt;
  geo.h = hgt;
  geo.bpp = 32;
  geo.size = geo.w*geo.h*(geo.bpp/8);
  geo.pitch = geo.w*(geo.bpp/8);

  //  this->freej = freej;
  //  geo.fps = freej->fps;
  //  geo.fps = env->fps_speed;
  geo.x = 0;//(freej->screen->w - geo.w)/2;
  geo.y = 0;//(freej->screen->h - geo.h)/2;
  //  blitter->crop( freej->screen );

  /* initialize the blitter */
  blitter.init(this);

  /* allocate memory for the matte background */
//  bgmatte = jalloc(bgmatte,geo.size);
  
  func("initialized %s layer %ix%i",
	 get_name(), geo.w, geo.h);
}

Context * Layer::context(){
	return env;
}

void Layer::run() {

  void *tmp_buf;

  //  func("%s this=%p thread: %p %s",__PRETTY_FUNCTION__, this,
  //pthread_self(), name);
  //  lock_feed();
  func("ok, layer %s in rolling loop",get_name());
    
  running = true;
	
  while(!quit) {

    //    lock();
    //    do_jobs();
    //    unlock();
    lock();
    tmp_buf = feed();
    unlock();
    //    lock();

    // check if feed returned a NULL buffer
    if(!tmp_buf) {

      func("feed returns NULL on layer %s",get_name());
      // check threshold of tolerated null feeds
      // deactivate the layer when too many
      null_feeds++;
      if(null_feeds > max_null_feeds) {
        warning("layer %s feed seems empty, deactivating", get_name());
        active = false;
	//        unlock(); // avoid causing deadlocks
        break;
      }
      continue;

      ////////////////////////////////////////
      // process filters on the feed buffer
    } else {

      null_feeds = 0;

      buffer = do_filters(tmp_buf);

    }
    
    Closure *blit_cb = NewSyncClosure<Layer>(this, &Layer::blit);
    add_job(blit_cb);
    blit_cb->wait();

    //wait_feed();
    //    sleep_feed();
  }
    
  running = false;
  func("%s this=%p thread end: %p %s",__PRETTY_FUNCTION__, this, pthread_self(), name);
}


void Layer::blit() {
  lock();
  offset = buffer;
  blitter.blit(this);
  unlock();
}

bool Layer::cafudda() {
  if(!opened) return false;

  if(!fade)
    if(!active || hidden)
      return false;

  do_iterators();

  // process all registered operations
  // and signal to the synchronous waiting feed()
  // includes blits and parameter changes
  do_jobs();
  
  //signal_feed();

  return(true);
}

void *Layer::do_filters(void *tmp_buf) {
  if( filters.len() ) {
    FilterInstance *filt;
    filters.lock();
    filt = (FilterInstance *)filters.begin();
    while(filt) {
      if(filt->active) {
	tmp_buf = (void*) filt->process(env->fps_speed, (uint32_t*)tmp_buf);
      }
      filt = (FilterInstance *)filt->next;
    }
    filters.unlock();
  }
  return tmp_buf;
}

int Layer::do_iterators() {

  /* process thru iterators */
  if(iterators.len()) {
    iterators.lock();
    iter = (Iterator*)iterators.begin();
    while(iter) {
      res = iter->cafudda(); // if cafudda returns -1...
      itertmp = iter;
      iter = (Iterator*) ((Entry*)iter)->next;
      if(res<0) {
	iterators.unlock();
	delete itertmp; // ...iteration ended
	iterators.lock();
	if(!iter)
	  if(fade) { // no more iterations, fade out deactivates layer
	    fade = false;
	    active = false;
	  }
      }
    }
    iterators.unlock();
  }
}

bool Layer::set_parameter(int idx) {

  Parameter *param;
  param = (Parameter*)parameters->pick(idx);
  if( ! param) {
    error("parameter %s not found in layer %s", param->name, name );
    return false;
  } else 
    func("parameter %s found in layer %s at position %u",
	 param->name, name, idx);

  if(!param->layer_set_f) {
    error("no layer callback function registered in this parameter");
    return false;
  }

  (*param->layer_set_f)(this, param, idx);

  return true;
}


void Layer::set_filename(const char *f) {
  const char *p = f + strlen(f);
  while(*p!='/' && (p >= f)) 
      p--;
  strncpy(filename,p+1,256);
}

void Layer::set_position(int x, int y) {
  lock();
  slide_x = geo.x = x;
  slide_y = geo.y = y;
  blitter.crop( screen );
  unlock();
}

void Layer::slide_position(int x, int y, int speed) {
  
  slide_x = (float)geo.x;
  slide_y = (float)geo.y;

  if(x!=geo.x) {
    iter = new Iterator(&slide_x);
    iter->set_aim((float)x);
    iter->set_step((float)speed);
    iterators.append(iter);
  }

  if(y!=geo.y) {
    iter = new Iterator(&slide_y);
    iter->set_aim((float)y);
    iter->set_step((float)speed);
    iterators.append(iter);
  }

}

void Layer::_fit(bool maintain_aspect_ratio){
	if(env){
		double width_zoom, height_zoom;
		int new_x = 0;
		int new_y = 0;
		lock();
		width_zoom = (double)env->screen->w / geo.w;
		height_zoom = (double)env->screen->h / geo.h;
		if (maintain_aspect_ratio){
			//to maintain the aspect ratio we simply zoom to the smaller of the
			//two zoom values
			if(width_zoom > height_zoom) {
				//if we're using the height zoom then there is going to be space
				//in x [width] that is unfilled, so center it in the x
				blitter.set_zoom(height_zoom, height_zoom);
				new_x = ((double)(env->screen->w - height_zoom * geo.w) / 2.0);
			} else {
				//if we're using the width zoom then there is going to be space
				//in y [height] that is unfilled, so center it in the y
				blitter.set_zoom(width_zoom, width_zoom);
				new_y = ((double)(env->screen->h - width_zoom * geo.h) / 2.0);
			}
		} else
			blitter.set_zoom(width_zoom, height_zoom);
		unlock();
		//set_position locks, so we unlock before it
		set_position(new_x, new_y);
	}
}

void Layer::fit(bool maintain_aspect_ratio) {
	// the rest is not yet ready for closures so keep it as before
	//Closure *job = NewClosure(this, &Layer::_fit, maintain_aspect_ratio);
	//add_job(job);
	this->_fit(maintain_aspect_ratio);
}

/* wrap JSyncThread::get_fps() so we don't need to export it in SWIG */
float Layer::get_fps() {
	return JSyncThread::get_fps();
}

/* wrap JSyncThread::set_fps() so we don't need to export it in SWIG */
float Layer::set_fps(float fps_new) {
	float fps_old = JSyncThread::set_fps(fps_new);
	JSyncThread::signal_feed();
	return fps_old;
}

void Layer::pulse_alpha(int step, int value) {
  if(!fade) {
    blitter.set_blit("0alpha"); /* by placing a '0' in front of the
				   blit name we switch its value to
				   zero before is being showed */
    fade = true; // after the iterator it should deactivate the layer
    // fixme: doesn't works well with concurrent iterators
  }

  blitter.pulse_value(step,value);
}
