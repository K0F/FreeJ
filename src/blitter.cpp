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
#include <iterator.h>
#include <imagefilter.h>

#include <sdl_screen.h>

#include <jutils.h>
#include <config.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
static uint32_t rmask = 0xff000000;
static uint8_t rchan = 0;
static uint32_t gmask = 0x00ff0000;
static uint8_t gchan = 1;
static uint32_t bmask = 0x0000ff00;
static uint8_t bchan = 2;
static uint32_t amask = 0x000000ff;
static uint8_t achan = 3;
#else
static uint32_t rmask = 0x000000ff;
static uint8_t rchan = 2;
static uint32_t gmask = 0x0000ff00;
static uint8_t gchan = 1;
static uint32_t bmask = 0x00ff0000;
static uint8_t bchan = 0;
static uint32_t amask = 0xff000000;
static uint8_t achan = 0;
#endif

// blit functions prototype
#define BLIT static inline void


/* blit functions, prototype is:
   void (*blit_f)(void *src, void *dst, int bytes) */

BLIT memcpy_blit(void *src, void *dst, int bytes, void *value) {
  memcpy(dst,src,bytes);
}


BLIT accel_memcpy_blit(void *src, void *dst, int bytes, void *value) {
  jmemcpy(dst,src,bytes);
}

BLIT red_channel(void *src, void *dst, int bytes, void *value) {
  register int c;
  register uint8_t *s = (uint8_t*)src;
  register uint8_t *d = (uint8_t*)dst;
  for(c=bytes>>2;c>0;c--,s+=4,d+=4)
    *(d+rchan) = *(s+rchan);
}

BLIT green_channel(void *src, void *dst, int bytes, void *value) {
  register int c;
  register uint8_t *s = (uint8_t*)src;
  register uint8_t *d = (uint8_t*)dst;
  for(c=bytes>>2;c>0;c--,s+=4,d+=4)
    *(d+gchan) = *(s+gchan);
}

BLIT blue_channel(void *src, void *dst, int bytes, void *value) {
  register int c;
  register uint8_t *s = (uint8_t*)src;
  register uint8_t *d = (uint8_t*)dst;
  for(c=bytes>>2;c>0;c--,s+=4,d+=4)
    *(d+bchan) = *(s+bchan);
}

BLIT schiffler_add(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAdd((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_sub(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterSub((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_mean(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMean((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_absdiff(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAbsDiff((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_mult(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMult((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_multnor(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultNor((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_div(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterDiv((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_multdiv2(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultDivby2((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_multdiv4(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultDivby2((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_and(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBitAnd((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

BLIT schiffler_or(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBitOr((unsigned char*)src,(unsigned char*)dst,(unsigned char*)dst,bytes);
}

/* ====== end of transparent blits
   all the following blits can be considered effects
   they completely overwrite the underlying image */


// PAST blits
BLIT past_add(void *src, void *past, void *dst, int bytes) {
  SDL_imageFilterAdd((unsigned char*)src,
		     (unsigned char*)past,
		     (unsigned char*)dst,bytes);
}

BLIT past_sub(void *src, void *past, void *dst, int bytes) {
  SDL_imageFilterSub((unsigned char*)src,
		     (unsigned char*)past,
		     (unsigned char*)dst,bytes);
}

BLIT past_absdiff(void *src, void *past, void *dst, int bytes) {
  SDL_imageFilterAbsDiff((unsigned char*)src,
			 (unsigned char*)past,
			 (unsigned char*)dst,bytes);
}

BLIT past_multdiv4(void *src, void *past, void *dst, int bytes) {
  SDL_imageFilterMultDivby4((unsigned char*)src,
			    (unsigned char*)past,
			    (unsigned char*)dst,bytes);
}

BLIT past_or(void *src, void *past, void *dst, int bytes) {
  SDL_imageFilterBitOr((unsigned char*)src,
		       (unsigned char*)past,
		       (unsigned char*)dst,bytes);
}


// non transparent schiffler blits
BLIT schiffler_neg(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBitNegation((unsigned char*)src,(unsigned char*)dst,bytes);
}

BLIT schiffler_addbyte(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAddByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

BLIT schiffler_addbytetohalf(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterAddByteToHalf((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

BLIT schiffler_subbyte(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterSubByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

BLIT schiffler_shl(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterShiftLeft((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

BLIT schiffler_shlb(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterShiftLeftByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

BLIT schiffler_shr(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterShiftRight((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

BLIT schiffler_mulbyte(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterMultByByte((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

BLIT schiffler_binarize(void *src, void *dst, int bytes, void *value) {
  SDL_imageFilterBinarizeUsingThreshold
    ((unsigned char*)src,(unsigned char*)dst,bytes, *(unsigned char*)value);
}

///////////////////////////////////////////////////////////////////
// SDL BLITS
//
// TODO: more SDL blits playing with color masks

// temporary surface used in SDL blits
static SDL_Surface *sdl_surf;

BLIT sdl_rgb(void *src, SDL_Rect *src_rect,
	     SDL_Surface *dst, SDL_Rect *dst_rect,
	     ScreenGeometry *geo, void *value) {
  
  sdl_surf = SDL_CreateRGBSurfaceFrom
    (src, geo->w, geo->h, geo->bpp,
     geo->pitch, bmask, gmask, rmask, 0x0);
  
  SDL_BlitSurface( sdl_surf, src_rect, dst, dst_rect );
  
  SDL_FreeSurface( sdl_surf );

}

BLIT sdl_alpha(void *src, SDL_Rect *src_rect,
	       SDL_Surface *dst, SDL_Rect *dst_rect,
	       ScreenGeometry *geo, void *value) {

  sdl_surf = SDL_CreateRGBSurfaceFrom
    (src, geo->w, geo->h, geo->bpp,
     geo->pitch, bmask, gmask, rmask, 0x0);

  SDL_SetAlpha( sdl_surf, SDL_SRCALPHA|SDL_RLEACCEL, *(unsigned int*)value );  

  SDL_BlitSurface( sdl_surf, src_rect, dst, dst_rect );
  
  SDL_FreeSurface( sdl_surf );
  
}

BLIT sdl_srcalpha(void *src, SDL_Rect *src_rect,
		  SDL_Surface *dst, SDL_Rect *dst_rect,
		  ScreenGeometry *geo, void *value) {

  sdl_surf = SDL_CreateRGBSurfaceFrom
    (src, geo->w, geo->h, geo->bpp,
     geo->pitch, bmask, gmask, rmask, amask);

  //  SDL_SetAlpha( sdl_surf, SDL_SRCALPHA|SDL_RLEACCEL, *(unsigned int*)value );  

  SDL_BlitSurface( sdl_surf, src_rect, dst, dst_rect );
  
  SDL_FreeSurface( sdl_surf );
  
}

BLIT sdl_chromakey(void *src, SDL_Rect *src_rect,
		   SDL_Surface *dst, SDL_Rect *dst_rect,
		   ScreenGeometry *geo, void *value) {

  sdl_surf = SDL_CreateRGBSurfaceFrom
    (src, geo->w, geo->h, geo->bpp,
     geo->pitch, bmask, gmask, rmask, 0x0);

  SDL_SetColorKey( sdl_surf, SDL_SRCCOLORKEY | SDL_RLEACCEL, *(uint32_t*)value);

  SDL_BlitSurface( sdl_surf, src_rect, dst, dst_rect );
  
  SDL_FreeSurface( sdl_surf );

}

Blit::Blit() :Entry() {
  sprintf(desc,"none");
  value = 0x0;
  memset(kernel,0,256);
  fun = NULL;
  type = 0x0;
  past_frame = NULL;
}

Blit::~Blit() { }

Blitter::Blitter() {
  Blit *b;

  x_scale = 0.0;
  y_scale = 0.0;
  rotation = 0.0;

  /* fill up linklist of blits */

  // default blit is MEMCPY
  b = new Blit(); sprintf(b->get_name(),"MEMCPY");
  sprintf(b->desc,"vanilla glibc memcpy");
  b->type = LINEAR_BLIT;
  b->fun = memcpy_blit; blitlist.append(b);
  current_blit = b; b->sel(true);

  b = new Blit(); b->set_name("AMEMCPY");
  sprintf(b->desc,"cpu accelerated memcpy");
  b->type = LINEAR_BLIT;
  b->fun = accel_memcpy_blit; blitlist.append(b);

  b = new Blit(); b->set_name("ADD");
  sprintf(b->desc,"bytewise addition");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_add; blitlist.append(b);
  
  b = new Blit(); b->set_name("SUB");
  sprintf(b->desc,"bytewise subtraction");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_sub; blitlist.append(b);
  
  b = new Blit(); b->set_name("MEAN");
  sprintf(b->desc,"bytewise mean");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_add; blitlist.append(b);

  b = new Blit(); b->set_name("ABSDIFF");
  sprintf(b->desc,"absolute difference");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_absdiff; blitlist.append(b);

  b = new Blit(); b->set_name("MULT");
  sprintf(b->desc,"multiplication");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_mult; blitlist.append(b);

  b = new Blit(); b->set_name("MULTNOR");
  sprintf(b->desc,"normalized multiplication");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_multnor; blitlist.append(b);

  b = new Blit(); b->set_name("DIV");
  sprintf(b->desc,"division");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_div; blitlist.append(b);

  b = new Blit(); b->set_name("MULTDIV2");
  sprintf(b->desc,"multiplication and division by 2");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_multdiv2; blitlist.append(b);

  b = new Blit(); b->set_name("MULTDIV4");
  sprintf(b->desc,"multiplication and division by 4");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_multdiv4; blitlist.append(b);

  b = new Blit(); b->set_name("AND");
  sprintf(b->desc,"bitwise and");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_and; blitlist.append(b);

  b = new Blit(); b->set_name("OR");
  sprintf(b->desc,"bitwise or");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_or; blitlist.append(b);

  b = new Blit(); b->set_name("RED");
  sprintf(b->desc,"red channel only blit");
  b->type = LINEAR_BLIT;
  b->fun = red_channel; blitlist.append(b);

  b = new Blit(); b->set_name("GREEN");
  sprintf(b->desc,"green channel only blit");
  b->type = LINEAR_BLIT;
  b->fun = green_channel; blitlist.append(b);

  b = new Blit(); b->set_name("BLUE");
  sprintf(b->desc,"blue channel only blit");
  b->type = LINEAR_BLIT;
  b->fun = blue_channel; blitlist.append(b);

  b = new Blit(); b->set_name("NEG");
  sprintf(b->desc,"bitwise negation");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_neg; blitlist.append(b);

  b = new Blit(); b->set_name("ADDB");
  sprintf(b->desc,"add byte to bytes");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_addbyte; blitlist.append(b);

  b = new Blit(); b->set_name("ADDBH");
  sprintf(b->desc,"add byte to half");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_addbytetohalf; blitlist.append(b);
  
  b = new Blit(); b->set_name("SUBB");
  sprintf(b->desc,"subtract byte to bytes");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_subbyte; blitlist.append(b);

  b = new Blit(); b->set_name("SHL");
  sprintf(b->desc,"shift left bits");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_shl; blitlist.append(b);

  b = new Blit(); b->set_name("SHLB");
  sprintf(b->desc,"shift left byte");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_shlb; blitlist.append(b);

  b = new Blit(); b->set_name("SHR");
  sprintf(b->desc,"shift right bits");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_shr; blitlist.append(b);

  b = new Blit(); b->set_name("MULB");
  sprintf(b->desc,"multiply by byte");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_mulbyte; blitlist.append(b);

  b = new Blit(); b->set_name("BIN");
  sprintf(b->desc,"binarize using threshold");
  b->type = LINEAR_BLIT;
  b->fun = schiffler_binarize; blitlist.append(b);

  // SDL blits
  b = new Blit(); b->set_name("SDLCPY");
  sprintf(b->desc,"RGB blit (SDL)");
  b->type = SDL_BLIT;
  b->sdl_fun = sdl_rgb; blitlist.append(b);

  b = new Blit(); b->set_name("ALPHA");
  sprintf(b->desc,"alpha blit (SDL)");
  b->type = SDL_BLIT;
  b->sdl_fun = sdl_alpha; blitlist.append(b);

  b = new Blit(); b->set_name("SRCALPHA");
  sprintf(b->desc,"source alpha blit (SDL)");
  b->type = SDL_BLIT;
  b->sdl_fun = sdl_srcalpha; blitlist.append(b);
  
  b = new Blit(); b->set_name("CHROMAKEY");
  sprintf(b->desc,"chromakey blit (SDL)");
  b->type = SDL_BLIT;
  b->sdl_fun = sdl_chromakey; blitlist.append(b);

  // PAST blits
  b = new Blit(); b->set_name("PAST_ADD");
  sprintf(b->desc,"add to past frame");
  b->type = PAST_BLIT;
  b->past_fun = past_add; blitlist.append(b);

  b = new Blit(); b->set_name("PAST_SUB");
  sprintf(b->desc,"subtract from past frame");
  b->type = PAST_BLIT;
  b->past_fun = past_sub; blitlist.append(b);

  b = new Blit(); b->set_name("PAST_ABSDIFF");
  sprintf(b->desc,"absolute difference on past frame");
  b->type = PAST_BLIT;
  b->past_fun = past_absdiff; blitlist.append(b);

  b = new Blit(); b->set_name("PAST_MULTDIV4");
  sprintf(b->desc,"multiply with past frame and divide by 4");
  b->type = PAST_BLIT;
  b->past_fun = past_multdiv4; blitlist.append(b);

  b = new Blit(); b->set_name("PAST_OR");
  sprintf(b->desc,"bitwise OR with past frame");
  b->type = PAST_BLIT;
  b->past_fun = past_or; blitlist.append(b);

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
  func("blitter initialized for layer %s",lay->get_name());

  crop( lay->freej->screen );

  return true;
}

void Blitter::blit() {
  int c;
  
  /* compare old layer values
     crop the layer if necessary */
  if( layer->geo.x != old_x 
      || layer->geo.y != old_y
      || layer->geo.w != old_w
      || layer->geo.h != old_h )
    crop( NULL );
  
  // executes LINEAR blits
  if( current_blit->type == LINEAR_BLIT ) {

    // setup crop variables
    current_blit->scr = current_blit->pscr = 
      (uint32_t*)current_blit->blit_coords;
    current_blit->off = current_blit->poff =
      (uint32_t*)layer->offset + current_blit->blit_offset;
    
    // iterates the blit on each horizontal line
    for(c = current_blit->blit_height; c>0; c--) {
      
      (*current_blit->fun)
	((void*)current_blit->off,
	 (void*)current_blit->scr,
	 current_blit->blit_pitch,
	 (void*)&current_blit->value);
      
      
      // strides down to the next line
      current_blit->off = current_blit->poff =
	current_blit->poff + layer->geo.w;
      current_blit->scr = current_blit->pscr = 
	current_blit->pscr + layer->freej->screen->w;
    }
    
    // executes SDL blit
  } else if (current_blit->type == SDL_BLIT) {
    
    (*current_blit->sdl_fun)
      (layer->offset, &current_blit->sdl_rect,
       ((SdlScreen*)layer->freej->screen)->surface, NULL,
       &layer->geo, (void*)&current_blit->value);

  } else if (current_blit->type == PAST_BLIT) {
    // this is a linear blit which operates
    // line by line on the previous frame

    // setup crop variables
    current_blit->scr = current_blit->pscr = 
      (uint32_t*)current_blit->blit_coords;
    current_blit->off = current_blit->poff =
      (uint32_t*)layer->offset + current_blit->blit_offset;
    current_blit->pastoff = current_blit->ppastoff =
      (uint32_t*)current_blit->past_frame;

    // iterates the blit on each horizontal line
    for(c = current_blit->blit_height; c>0; c--) {
      
      (*current_blit->past_fun)
	((void*)current_blit->off,
	 (void*)current_blit->pastoff,
	 (void*)current_blit->scr,
	 current_blit->blit_pitch);

      // copy the present to the past
      jmemcpy(current_blit->pastoff,
	      current_blit->off,
	      current_blit->blit_pitch);
      
      // strides down to the next line
      current_blit->off = current_blit->poff =
	current_blit->poff + layer->geo.w;
      current_blit->scr = current_blit->pscr = 
	current_blit->pscr + layer->freej->screen->w;
      current_blit->pastoff = current_blit->ppastoff =
	current_blit->ppastoff + layer->geo.w;

    }

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
  if(b->type == PAST_BLIT) { // must fill previous frame
    if(b->past_frame) free(b->past_frame);
    b->past_frame = calloc(layer->geo.size,1);
  }
  if(layer) crop(NULL);
  act("blit %s selected for layer %s",
      b->get_name(),(layer)?layer->get_name():" ");
  return true;
}

bool Blitter::set_value(int val) {
  Iterator *iter;

  /* setup an iterator to gradually change the value */
  iter = new Iterator((int16_t*)&current_blit->value);
  iter->set_step(1);
  iter->set_aim(val);
  layer->iterators.add(iter);

  //  b->value = val;
  act("layer %s blit %s set to %i",
      layer->get_name(),current_blit->get_name(),val);
  return true;
}

bool Blitter::set_colorkey(int colorkey_x,int colorkey_y) {

    Blit *b = (Blit*)blitlist.search("CHROMAKEY");
    if(!b) {
      error("can't find chromakey blit");
      return false;
    }

    uint8_t *colorkey=(uint8_t *)layer->offset + (colorkey_x<<2) + (colorkey_y * layer->geo.pitch);
	    
    uint8_t colorkey_r = *(colorkey + rchan);
    uint8_t colorkey_g = *(colorkey + gchan);
    uint8_t colorkey_b = *(colorkey + bchan);


    b->value = (colorkey_r)|((uint32_t)colorkey_g<<8)|((uint32_t)colorkey_b<<16);

      
    layer->has_colorkey=true;

    notice("Chromakey value: r%x g%x b%x #%x\n",
	   colorkey_r,colorkey_g,colorkey_b,b->value);
    return true;
}

bool Blitter::set_kernel(short *krn) {
  notice("Blitter::set_kernel : TODO convolution on blits");
  return false;
}


/* ok, let's draw the crop geometries and be nice commenting ;)

   that's tricky stuff, here i try to do it with tiling

   here the generic case of a layer on the screen, with variable names:
   

                                                         screen->w
  0,0___________________________________________________ -> 
   '                 ^                                  '
   | screen          |blit_y                            |
   |                 |                                  |
   |       x,y_______V________________ w                |
   |        '                         '                 |
   | blit_x | layer                   |                 |
   |<------>|                         |                 |
   |        |                         |<-------------...|
   |...---->|                         |  blit_pitch     |
   |        '-------------------------'                 |
   |       h                                            |
   |                                                    |
   '----------------------------------------------------'
   |
   V screen->h

   we have a couple of cases in which both x and y as well
   w and h of the layer can be out of the bounds of the screen.

   for instance, if the layer goes out of the left bound (x<0):

            0,0____________________
             '                     '
  x,y________|_________            |
   '         |         '           |
   |layer    |         |           |
   |         |         | blit_pitch|
   |         |         |<--------->|
   |         |         |           |
   '_________|_________'           |
   <-------->|                     |
     blit_x  '---------------------'

   the algorithm of the blit will look like:

   if(x < 0)
     blit_x = -x;
     blit_pitch = screen->w - (layer->w - blit_x);
   else
     blit_x = x;
     blit_pitch = screen->w - layer->w

     for(c=h;c>0;c--)
      cpy( layer+blit_x , screen, w );
      screen += blit_pitch;
      layer += layer->w;
     
   

*/
void Blitter::crop(ViewPort *screen) {
  Blit *b = current_blit;
  /* needed in linear blit crop */
  uint32_t blit_xoff=0;
  uint32_t blit_yoff=0;

  if(!b) return;
  if(!screen)
    screen = layer->freej->screen;

  /* crop for the SDL blit */
  if(b->type == SDL_BLIT) {
    b->sdl_rect.x = -(layer->geo.x);
    b->sdl_rect.y = -(layer->geo.y);
    b->sdl_rect.w = screen->w;
    b->sdl_rect.h = screen->h;

    /* crop for the linear blit */
  } else if(b->type == LINEAR_BLIT 
	    || b->type == PAST_BLIT) {
    
    b->blit_x = layer->geo.x;
    b->blit_y = layer->geo.y;
    b->blit_width = layer->geo.w;
    b->blit_height = layer->geo.h;
    blit_xoff = 0;
    blit_yoff = 0;
    
    /* left bound 
       affects x-offset and width */
    if(layer->geo.x<0) {
      blit_xoff = (-layer->geo.x);
      b->blit_x = 0;
      
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
	 b->get_name(), b->blit_x, b->blit_y, b->blit_width,
	 b->blit_height, blit_xoff, blit_yoff);

  }

  /* store values for further crop checking */
  old_x = layer->geo.x;
  old_y = layer->geo.y;
  old_w = layer->geo.w;
  old_h = layer->geo.h;

}


