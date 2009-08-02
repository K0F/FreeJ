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

//#include <fps.h>

Layer::Layer()
  :Entry(), JSyncThread() {
  func("%s this=%p",__PRETTY_FUNCTION__, this);
  env = NULL;
  active = false;
  hidden = false;
  fade = false;
  use_audio = false;
  need_crop = true;
  audio = NULL;
  opened = false;
  bgcolor = 0;
  set_name("???");
  filename[0] = 0;
  buffer = NULL;
  offset = NULL;
  screen = NULL;
  is_native_sdl_surface = false;
  jsclass = &layer_class;

  zoom_x = 1.0;
  zoom_y = 1.0;
  rotate = 0.0;
  zooming = false;
  rotating = false;

  antialias = false;

  blitter = NULL;
  current_blit = NULL;

  null_feeds = 0;
  max_null_feeds = 10;

  parameters = NULL;

  fps.set(25);

}

Layer::~Layer() {
  func("%s this=%p",__PRETTY_FUNCTION__, this);

  FilterInstance *f = (FilterInstance*)filters.begin();
  while(f) {
    f->rem(); // rem is contained in delete for Entry
    delete f;
    f = (FilterInstance*)filters.begin();
  }

  // free all parameters
  if(parameters) {
    Parameter *par;
    par = parameters->begin();
    while(par) {
      par->rem();
      delete par;
      par = parameters->begin();
    }
  }
  
  if(blitter) delete blitter;
  if(buffer) free(buffer);
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

  if(geo.size>0)
    buffer = jalloc(geo.size);
  // if  the   size  is  still  unknown   at  init  then   it  is  the
  // responsability for the layer implementation to create the buffer
  // (see for instance text_layer)
  
  func("initialized %s layer %ix%i",
	 get_name(), geo.w, geo.h);
}

Context * Layer::context(){
	return env;
}

void Layer::thread_setup() {
  func("ok, layer %s in rolling loop",get_name());
 
  while(!feed()) fps.calc();

  func(" layer %s entering loop",get_name());
}

void Layer::thread_loop() {

  void *tmp_buf;

  // process all registered operations
  // and signal to the synchronous waiting feed()
  // includes parameter changes for layer

  tmp_buf = feed();

  // check if feed returned a NULL buffer
  if(tmp_buf) {
    // process filters on the feed buffer
    tmp_buf = do_filters(tmp_buf);
  }

  // we add  a memcpy at the end  of the layer pipeline  this is not
  // that  expensive as leaving  the lock  around the  feed(), which
  // slows down the whole engine in case the layer is slow. -jrml
  lock();
  jmemcpy(buffer, tmp_buf, geo.size);
  unlock();

  fps.calc();
  fps.delay();
}

void Layer::thread_teardown() {
  func("%s this=%p thread end: %p %s",__PRETTY_FUNCTION__, this, pthread_self(), name);
}

char *Layer::get_blit() {

  if(!current_blit) {
    error("no blit currently selected for layer %s",name);
    return((char*)"unknown");
  }
  return current_blit->name;
}

bool Layer::set_blit(const char *bname) {
  Blit *b;
  int idx;
  
  if (screen && blitter) {
	  b = (Blit*)blitter->blitlist.search(bname, &idx);

	  if(!b) {
		error("blit %s not found in screen %s",bname, screen->name);
		return(false);
	  }

	  func("blit for layer %s set to %s",name, b->name);

	  current_blit = b; // start using
	  need_crop = true;
	  blitter->crop(this, screen);
	  blitter->blitlist.sel(0);
	  b->sel(true);
  }
  return(true);
}

void Layer::blit() {
  if(!buffer) {
    // check threshold of tolerated null feeds
    // deactivate the layer when too many
    func("feed returns NULL on layer %s",get_name());
    null_feeds++;
    if(null_feeds > max_null_feeds) {
      warning("layer %s has no video, deactivating", get_name());
      active = false;
      return;
    }

  } else {

    null_feeds = 0;

    lock();
    //    offset = buffer;
    screen->blit(this);
    unlock();
  }
}

bool Layer::cafudda() {
  if(!opened) return false;

  if(!fade)
    if(!active || hidden)
      return false;

  do_iterators();


  //  signal_feed();

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
  return(1);
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
  geo.x = x;
  geo.y = y;
  need_crop = true;
  unlock();
}



bool Layer::set_zoom(double x, double y) {

  if ((x == 1) && (y == 1)) {

    zooming = false;
    zoom_x = zoom_y = 1.0;
    act("%s layer %s zoom deactivated", name, filename);

  } else {

    zoom_x = x;
    zoom_y = y;
    zooming = true;
    func("%s layer %s zoom set to x%.2f y%.2f",	name, filename, zoom_x, zoom_y);

  }

  need_crop = true;
  return zooming;
}

bool Layer::set_rotate(double angle) {

  if(!angle) {

    rotating = false;
    rotate = 0;
    act("%s layer %s rotation deactivated", name, filename);

  } else {

    rotate = angle;
    rotating = true;
    func("%s layer %s rotation set to %.2f", name, filename, rotate);

  }

  need_crop = true;
  return rotating;
}


void Layer::_fit(bool maintain_aspect_ratio){
	if(env){
		double width_zoom, height_zoom;
		int new_x = 0;
		int new_y = 0;
		lock();
		width_zoom = (double)screen->w / geo.w;
		height_zoom = (double)screen->h / geo.h;
		if (maintain_aspect_ratio){
			//to maintain the aspect ratio we simply zoom to the smaller of the
			//two zoom values
			if(width_zoom > height_zoom) {
				//if we're using the height zoom then there is going to be space
				//in x [width] that is unfilled, so center it in the x
				set_zoom(height_zoom, height_zoom);
				new_x = ((double)(screen->w - height_zoom * geo.w) / 2.0);
			} else {
				//if we're using the width zoom then there is going to be space
				//in y [height] that is unfilled, so center it in the y
				set_zoom(width_zoom, width_zoom);
				new_y = ((double)(screen->h - width_zoom * geo.h) / 2.0);
			}
		} else
			set_zoom(width_zoom, height_zoom);
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

