/*  FreeJ - blitter layer component
 *
 *  (c) Copyright 2004-2009 Denis Roio aka jaromil <jaromil@dyne.org>
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
#include <linklist.h>


#include <sdl_screen.h>

#include <jutils.h>
#include <config.h>





Blit::Blit() :Entry() {
  sprintf(desc,"none");

  value = 0.0;
  has_value = false;

  memset(kernel,0,256);
  fun = NULL;
  type = NONE;
  past_frame = NULL;

}

Blit::~Blit() { }

Blitter::Blitter() {

  screen = NULL;

  old_lay_x = 0;
  old_lay_y = 0;
  old_lay_w = 0;
  old_lay_h = 0;


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



// char *Blitter::get_name() {
//   return name;
// }
		       


  // TODO layer 2 layer blit
// pass src->geo and dst->geo from two layers
// in case we want to blit one on another
// then the crop will prepare it's own layer to the other

/* ok, let's draw the crop geometries and be nice commenting ;)

   that's tricky stuff

   here the generic case of a layer on the screen, with variable names:
   

                                                         screen->w
  0,0___________________________________________________ -> 
   '                 ^                                  '
   | screen          |scr_stride_up                     |
   |                 |                                  |
   |       x,y_______V________________ w                |
   | scr_sx '                         '                 |
   | stride | layer                   |                 |
   |<------>|                         |                 |
   |        |                         |<-------------...|
   |...---->|                         | scr_stride_dx   |
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
 (offset)    '                     '
  x,y________|_________            |  offset is the point of start
   '         |         '           |  scr_stride is added to screen every line
   |layer    |         |           |  lay_stride is added to layer every line
   |         |         | scr_stride|
   |         |         |<--------->|
   |         |         |           |
   '_________|_________'           |
   <-------->|                     |
   lay_stride'---------------------'

     so the algorithm of the crop will look like:
*/
     

void Blitter::crop(Layer *lay, ViewPort *scr) {

  Blit *b;

  if(!lay || !scr) return;

  if(!lay->current_blit) return;

  b = lay->current_blit;

  func("crop on layer %s x%i y%i w%i h%i for blit %s",
       lay->name, lay->geo.x, lay->geo.y,
       lay->geo.w, lay->geo.h, b->name);
  
  // crop for the SDL blit
  if(b->type == Blit::SDL) {
    b->sdl_rect.x = -(lay->geo.x);
    b->sdl_rect.y = -(lay->geo.y);
    b->sdl_rect.w = scr->w;
    b->sdl_rect.h = scr->h;

    // crop for the linear and past blit
  } else if(b->type == Blit::LINEAR 
	    || b->type == Blit::PAST) {

    b->lay_pitch =  lay->geo.w; // how many pixels to copy each row
    b->lay_height = lay->geo.h; // how many rows we should copy
    
    b->scr_stride_up = 0; // rows to jump before starting to blit on screen
    b->scr_stride_sx = 0; // screen pixels stride on the left of each row
    b->scr_stride_dx = 0; // screen pixels stride on the right of each row
    
    b->lay_stride_up = 0; // rows to jump before starting to blit layer
    b->lay_stride_sx = 0; // how many pixels stride on the left of each row
    b->lay_stride_dx = 0; // how many pixels stride on the right of each row
    
    // BOTTOM
    if( lay->geo.y + lay->geo.h > scr->h ) {
      if( lay->geo.y > scr->h ) { // out of screen
	lay->geo.y = scr->h+1; // don't go far
	lay->hidden = true;
	return;
      } else { // partially out
	b->lay_height -= (lay->geo.y + lay->geo.h) - scr->h;
      }
    }
    
    // LEFT
    if( lay->geo.x < 0 ) {
      if( lay->geo.x + lay->geo.w < 0 ) { // out of screen
	lay->geo.x = -( lay->geo.w + 1 ); // don't go far
	lay->hidden = true;
	return;
      } else { // partially out
	b->lay_stride_sx += -lay->geo.x;
	b->lay_pitch -= -lay->geo.x;
      } 
    } else { // inside
      b->scr_stride_sx += lay->geo.x;
    }
    
    // UP
    if(lay->geo.y < 0) {
      if( lay->geo.y + lay->geo.h < 0) { // out of screen
	lay->geo.y = -( lay->geo.h + 1 ); // don't go far
	lay->hidden = true;
	return;
      } else { // partially out
	b->lay_stride_up += -lay->geo.y;
	b->lay_height -= -lay->geo.y;
      }
    } else { // inside
      b->scr_stride_up += lay->geo.y;
    }
    
    // RIGHT
    if( lay->geo.x + lay->geo.w > scr->w ) {
      if( lay->geo.x > scr->w ) { // out of screen
	lay->geo.x = scr->w + 1; // don't go far
	lay->hidden = true;
	return;
      } else { // partially out
	b->lay_pitch -= ( lay->geo.x + lay->geo.w ) - scr->w;
	b->lay_stride_dx += ( lay->geo.x + lay->geo.w ) - scr->w;
      } 
    } else { // inside
      b->scr_stride_dx += scr->w - (lay->geo.x + lay->geo.w );
    }
    
    lay->hidden = false;
    
    b->lay_stride = b->lay_stride_dx + b->lay_stride_sx; // sum strides
    // precalculate upper left starting offset for layer
    b->lay_offset = (b->lay_stride_sx +
		     ( b->lay_stride_up * lay->geo.w ));
    
    b->scr_stride = b->scr_stride_dx + b->scr_stride_sx; // sum strides
    // precalculate upper left starting offset for screen
    b->scr_offset = (b->scr_stride_sx +
		     ( b->scr_stride_up * scr->w ));
  }
  
  // calculate bytes per row
  b->lay_bytepitch = b->lay_pitch * (lay->geo.bpp/8);
  
}  


//////////////////

///////// LEFTOVERS


//   /* compare old layer values
//      crop the layer only if necessary */
//   if(!force)
//     if( (lay->geo->x == old_lay_x)
// 	&& (lay->geo->y == old_lay_y)
// 	&& (lay->geo->w == old_lay_w)
// 	&& (lay->geo->h == old_lay_h)
// 	&& (scr->w == old_scr_w)
// 	&& (scr->h == old_scr_h)
// 	)
//       return;




//   // assign the right pointer to the *geo used in crop
//   // we use the normal geometry if not roto|zoom
//   // otherwise the layer::geo_rotozoom
//   if(rotozoom) {

//     geo = &geo_rotozoom;
//     // shift up/left to center rotation
//     geo->x = layer->geo.x - (rotozoom->w - layer->geo.w)/2;
//     geo->y = layer->geo.y - (rotozoom->h - layer->geo.h)/2;

//     geo->w = rotozoom->w;
//     geo->h = rotozoom->h;
//     geo->bpp = 32;
//     geo->pitch = 4*geo->w;

//   } else geo = &layer->geo;

//   // QUAAA

//   if(layer->slide_x != geo->x) geo->x = (int16_t)layer->slide_x;
//   if(layer->slide_y != geo->y) geo->y = (int16_t)layer->slide_y;

//   //////////////////////
