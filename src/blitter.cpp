/*  FreeJ - blitter layer component
 *  (c) Copyright 2004 Denis Roio aka jaromil <jaromil@dyne.org>
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

#include <layer.h>
#include <blitter.h>
#include <context.h>
#include <imagefilter.h>

#include <jutils.h>
#include <config.h>

/* blit functions, prototype is:
   void (*blit_f)(void *src, void *dst, int bytes) */

static inline void memcpy_blit(void *src, void *dst, int bytes, void *value) {
  memcpy(dst,src,bytes);
}

static inline void accel_memcpy_blit(void *src, void *dst, int bytes, void *value) {
  jmemcpy(dst,src,bytes);
}

static inline void schiffler_add(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAdd((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_sub(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterSub((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_mean(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMean((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_absdiff(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAbsDiff((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_mult(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMult((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_multnor(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultNor((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_div(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterDiv((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_multdiv2(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultDivby2((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_multdiv4(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultDivby2((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_and(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBitAnd((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_or(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBitOr((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

static inline void schiffler_neg(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBitNegation((unsigned char*)src,(unsigned char*)dst,bytes);
}

static inline void schiffler_addbyte(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAddByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

static inline void schiffler_addbytetohalf(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAddByteToHalf((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

static inline void schiffler_subbyte(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterSubByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

static inline void schiffler_shl(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterShiftLeft((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

static inline void schiffler_shlb(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterShiftLeftByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

static inline void schiffler_shr(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterShiftRight((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

static inline void schiffler_mulbyte(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultByByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

static inline void schiffler_binarize(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBinarizeUsingThreshold
    ((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}




Blit::Blit() :Entry() {
  sprintf(desc,"none");
  value = 0xff;
  memset(kernel,0,256);
  fun = NULL;
  type = 0x0;

#ifdef ARCH_PPC
  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;
#else
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = 0xff000000;
#endif

}

Blit::~Blit() { }

Blitter::Blitter() {
  x_scale = 0.0;
  y_scale = 0.0;
  rotation = 0.0;

  /* the default blit that should always be present */
  Blit *b = new Blit(); sprintf(b->name,"MEMCPY");
  sprintf(b->desc,"vanilla glibc memcpy");
  b->type = LINEAR_BLIT;
  b->fun = memcpy_blit; blitlist.append(b);
  current_blit = b; b->sel(true);

}

Blitter::~Blitter() {
  Blit *tmp;
  Blit *b = (Blit*)blitlist.begin();
  while(b) {
    tmp = (Blit*)b->next;
    delete b;
    b = tmp;
  }
}

bool Blitter::init(Layer *lay) {
  layer = lay;
  func("blitter initialized for layer %s",lay->name);

  /* fill up linklist of blits */
  Blit *b;


  b = new Blit(); sprintf(b->name,"AMEMCPY");
  sprintf(b->desc,"cpu accelerated memcpy");
  b->type = LINEAR_BLIT;
  b->fun = accel_memcpy_blit; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"ADD");
  sprintf(b->desc,"bytewise addition");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_add; blitlist.append(b);
  
  b = new Blit(); sprintf(b->name,"SUB");
  sprintf(b->desc,"bytewise subtraction");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_sub; blitlist.append(b);
  
  b = new Blit(); sprintf(b->name,"MEAN");
  sprintf(b->desc,"bytewise mean");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_add; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"ABSDIFF");
  sprintf(b->desc,"absolute difference");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_absdiff; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"MULT");
  sprintf(b->desc,"multiplication");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_mult; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"MULTNOR");
  sprintf(b->desc,"normalized multiplication");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_multnor; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"DIV");
  sprintf(b->desc,"division");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_div; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"MULTDIV2");
  sprintf(b->desc,"multiplication and division by 2");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_multdiv2; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"MULTDIV4");
  sprintf(b->desc,"multiplication and division by 4");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_multdiv4; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"AND");
  sprintf(b->desc,"bitwise and");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_and; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"OR");
  sprintf(b->desc,"bitwise or");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_or; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"NEG");
  sprintf(b->desc,"bitwise negation");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_neg; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"ADDB");
  sprintf(b->desc,"add byte to bytes");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_addbyte; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"ADDBH");
  sprintf(b->desc,"add byte to half");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_addbytetohalf; blitlist.append(b);
  
  b = new Blit(); sprintf(b->name,"SUBB");
  sprintf(b->desc,"subtract byte to bytes");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_subbyte; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"SHL");
  sprintf(b->desc,"shift left bits");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_shl; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"SHLB");
  sprintf(b->desc,"shift left byte");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_shlb; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"SHR");
  sprintf(b->desc,"shift right bits");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_shr; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"MULB");
  sprintf(b->desc,"multiply by byte");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_mulbyte; blitlist.append(b);

  b = new Blit(); sprintf(b->name,"BIN");
  sprintf(b->desc,"binarize using threshold");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_binarize; blitlist.append(b);


  // SDL blits
  b = new Blit(); sprintf(b->name,"ALPHA");
  sprintf(b->desc,"SDL alpha blit (hardware accelerated)");
  b->type = SDL_BLIT;
  b->fun = NULL; blitlist.append(b);

  // SET DEFAULT
  set_blit("alpha");

  return true;
}

void Blitter::blit() {
  register int c;
  if(!current_blit) return;
  switch(current_blit->type) {

  case LINEAR_BLIT:
    current_blit->scr = current_blit->pscr = 
      (uint32_t*)current_blit->blit_coords;
    current_blit->off = current_blit->poff =
      (uint32_t*)layer->offset + current_blit->blit_offset;
    
    for(c = current_blit->blit_height; c>0; c--) {

      (*current_blit->fun)
	((void*)current_blit->off,
	 (void*)current_blit->scr,
	 current_blit->blit_pitch,
	 (void*)&current_blit->value);


      current_blit->off = current_blit->poff =
	current_blit->poff + layer->geo.w;
      current_blit->scr = current_blit->pscr = 
	current_blit->pscr + layer->freej->screen->w;
    }
    
  case SDL_BLIT:
    current_blit->sdl_surface = 
      SDL_CreateRGBSurfaceFrom
      (layer->offset, layer->geo.w, layer->geo.h, layer->geo.bpp,
       layer->geo.pitch, current_blit->bmask, current_blit->gmask,
       current_blit->rmask, 0x0);

    if(current_blit->value <0xff) // is there transparency?
      SDL_SetAlpha(current_blit->sdl_surface,SDL_SRCALPHA,
		   current_blit->value);
    
    SDL_BlitSurface(current_blit->sdl_surface,
		    &current_blit->sdl_rect,
		    SDL_GetVideoSurface(),NULL);

    SDL_FreeSurface(current_blit->sdl_surface);
    break;

  case PLANAR_BLIT:
    break;
  }
}



bool Blitter::set_blit(char *name) {
  Blit *b = (Blit*)blitlist.search(name);

  if(!b) {
    error("blit %s not found",name);
    return false;
  }
  
  // found the matching name!
  current_blit = b;
  blitlist.sel(0);
  b->sel(true);
  crop(NULL);
  act("blit %s selected for layer %s",
      b->name,layer->name);
  return true;
}

bool Blitter::set_value(int val) {
  Blit *b = (Blit*)blitlist.selected();
  if(!b) {
    error("no blit selected on layer %s",layer->name);
    return false;
  }
  b->value = val;
  act("layer %s blit %s set to %i",
      layer->name,b->name,val);
  return true;
}

bool Blitter::set_kernel(short *krn) {
  notice("Blitter::set_kernel : TODO convolution on blits");
  return false;
}

void Blitter::crop(ViewPort *screen) {
  if(!current_blit) return;
  Blit *b = current_blit;
  if(!screen)
    screen = layer->freej->screen;

  /* needed in linear blit crop */
  uint32_t blit_xoff=0;
  uint32_t blit_yoff=0;

  switch(b->type) {
  case SDL_BLIT: /* crop for the SDL blit */
    b->sdl_rect.x = -(layer->geo.x);
    b->sdl_rect.y = -(layer->geo.y);
    b->sdl_rect.w = screen->w;
    b->sdl_rect.h = screen->h;
    break;
    
  case LINEAR_BLIT:
    
    b->blit_width = layer->geo.w;
    b->blit_height = layer->geo.h;
    blit_xoff = 0;
    blit_yoff = 0;
    
    /* left bound 
       affects x-offset and width */
    if(layer->geo.x<0) {
      blit_xoff = (-layer->geo.x);
      b->blit_x = 1;
      
      if(blit_xoff>layer->geo.w) {
	func("layer out of screen to the left");
	layer->hidden = true; /* out of the screen */
	layer->geo.x = -(layer->geo.w+1); /* don't let it go far */      
      } else {
	layer->hidden = false;
	b->blit_width -= blit_xoff;
      }
    }
    
    /* right bound
       affects width */
    if((layer->geo.x+layer->geo.w)>screen->w) {
      if(layer->geo.x>screen->w) { /* out of screen */
	func("layer out of screen to the right");
	layer->hidden = true; 
	layer->geo.x = screen->w+1; /* don't let it go far */
      } else {
	layer->hidden = false;
	b->blit_width -= (layer->geo.w - (screen->w - layer->geo.x));
      }
    }
    
    /* upper bound
       affects y-offset and height */
    if(layer->geo.y<0) {
      blit_yoff = (-layer->geo.y);
      b->blit_y = 1;
      
      if(blit_yoff>layer->geo.h) {
	func("layer out of screen up");
	layer->hidden = true; /* out of the screen */
	layer->geo.y = -(layer->geo.h+1); /* don't let it go far */      
      } else {
	layer->hidden = false;
	b->blit_height -= blit_yoff;
      }
    }
    
    /* lower bound
       affects height */
    if((layer->geo.y+layer->geo.h) >screen->h) {
      if(layer->geo.y >screen->h) { /* out of screen */
	func("layer out of screen down");
	layer->hidden = true; 
	layer->geo.y = screen->h+1; /* don't let it go far */
      } else {
	layer->hidden = false;
	b->blit_height -= (layer->geo.h - (screen->h - layer->geo.y));
      }
    }
    
    b->blit_coords = (uint32_t*)screen->coords(b->blit_x,b->blit_y);
    
    b->blit_offset = (blit_xoff<<2) + (blit_yoff*layer->geo.pitch);
    
    b->blit_pitch = b->blit_width * (layer->geo.bpp>>3);
    
    func("LINEAR CROP for blit %s x[%i] y[%i] w[%i] h[%i] xoff[%i] yoff[%i]",
	 b->name, b->blit_x, b->blit_y, b->blit_width,
	 b->blit_height, blit_xoff, blit_yoff);
    break;

  case PLANAR_BLIT:
    notice("PLANAR_BLIT TO BE DONE ;)");
    break;
  }

}


