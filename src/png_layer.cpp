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
 */

#include <iostream>
#include <errno.h>
#include <png_layer.h>
#include <context.h>
#include <lubrify.h>
#include <jutils.h>
#include <config.h>

#define PNG_BYTES_TO_CHECK 4

PngLayer::PngLayer()
  :Layer() {
  core = NULL;
  info = NULL;
  buf = NULL;
  row_pointers = NULL;
  fp = NULL;
  setname("IMG");
}

PngLayer::~PngLayer() {
  close();
}

bool PngLayer::open(char *file) {
  func("PngLayer::open(%s)",file);

  fp = fopen(file,"rb");
  if(!fp) {
    error("Pnglayer::open(%s) - %s",file,strerror(errno));
    return (false); }

  fread(sig,1,PNG_BYTES_TO_CHECK,fp);
  if(png_sig_cmp(sig,(png_size_t)0,PNG_BYTES_TO_CHECK)) {
    error("Pnglayer::open(%s) - not a valid png file",file);
    fclose(fp); fp = NULL;
    return (false); }

  return(true);
}

bool PngLayer::init(Context *scr) {

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;

  if(!fp) {
    error("no png file opened, layer skipped");
    return false;
  }

  /* create png structures */

  core = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);
  if (!core) {
    error("can't create PNG core");
    return(false);
  }
  
  info = png_create_info_struct(core);
  if (!info) {
    error("can't gather PNG info");
    png_destroy_read_struct
      (&core, (png_infopp)NULL, (png_infopp)NULL);
    return (false);
  }

  /* initialize error message callback */

  if ( setjmp(core->jmpbuf) )
    error("error reading the PNG file.");

  /* start peeking into the file */

  png_init_io(core,fp);

  png_set_sig_bytes(core,PNG_BYTES_TO_CHECK);  

  png_read_info(core,info);

  png_get_IHDR(core, info, &width, &height, &bit_depth, &color_type,
	       &interlace_type, NULL, NULL);

  /* tell libpng to strip 16 bit/color files down to 8 bits/color */
  png_set_strip_16(core) ;

  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
     byte into separate bytes (useful for paletted and grayscale images). */
  png_set_packing(core);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    func("PNG set palette to rgb");
    png_set_palette_to_rgb(core);
  }
  
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    func("PNG set gray to 8bpp");
    png_set_gray_1_2_4_to_8(core);
  }

  png_set_filler(core, 0xff, PNG_FILLER_AFTER);
  
  png_set_tRNS_to_alpha(core);
  
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    func("PNG set gray to rgb");
    png_set_gray_to_rgb(core);
  }


  /* we don't want background to keep transparence
  png_color_16 bg;
  png_color_16p image_bg;
  if (png_get_bKGD(core, info, &image_bg)) {
    func("PNG set background on file gamma");
    png_set_background(core, image_bg, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
  } else {
    func("PNG set background on screen gamma");
    png_set_background(core, &bg, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
  } */


  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_bgr(core);
  
  png_set_interlace_handling(core);
  
  png_read_update_info(core,info);                 
  
  png_get_IHDR(core, info, &width, &height, &bit_depth, &color_type,
	       &interlace_type, NULL, NULL);
 
  if(scr) screen = scr;
  _init(screen,	width, height, bit_depth*4);

  buf = (png_bytep)jalloc(buf,geo.size);

  /* allocate image memory */
  row_pointers = (png_bytep*)jalloc(row_pointers,geo.h*sizeof(png_bytep));
  for(int i=0;i<geo.h;i++)
    row_pointers[i] = (png_bytep) buf + i*geo.pitch;

  png_read_image(core,row_pointers);

  png_read_end(core,NULL);

  fclose(fp); fp = NULL;
  png_destroy_info_struct(core,&info);
  png_destroy_read_struct(&core,NULL,NULL);
  
  /* apply alpha layer
  for(unsigned int i=0;i<geo.size;i+=4) {
  buf[i] &= buf[i+3];
  buf[i+1] &= buf[i+3];
  buf[i+2] &= buf[i+3];
  } */

  notice("PngLayer :: w[%u] h[%u] bpp[%u] size[%u]",
	 geo.w,geo.h,geo.bpp,geo.size);
  return(true);
}

void *PngLayer::get_buffer() {
  return buf;
}

bool PngLayer::feed() {
  return true;
}

void PngLayer::close() {
  func("PngLayer::close()");
  jfree(row_pointers);
  jfree(buf);
}

#define BLIT(op) \
    { \
      Uint8 *alpha; \
      Uint32 *scr, *pscr; \
      scr = pscr = (Uint32 *) screen->coords(blit_x,blit_y); \
      Uint32 *off, *poff; \
      off = poff = (Uint32 *) ((Uint8*)offset+blit_offset); \
      int c, cc; \
      for(c=blit_height;c>0;c--) { \
	for(cc=blit_width;cc>0;cc--) { \
	  alpha = (Uint8 *) off; \
	  if(*(alpha+4)) *scr op *off; \
	  scr++; off++; \
	} \
	off = poff = poff + geo.w; \
	scr = pscr = pscr + screen->w; \
      } \
    }

void PngLayer::blit(void *offset) {
  /* transparence aware blits:
     if alpha channel is 0x00 then pixel is not blitted
     works with 32bpp */
  
  if(hidden) return;
  
  switch(_blit_algo) {
    
  case 1:
  case 2:
    BLIT(=); return;
    
  case 3:
  case 4:
  case 5:
    {
      Uint8 *alpha;
      int chan = _blit_algo-3;
      char *scr, *pscr;
      scr = pscr = (char *) screen->coords(blit_x,blit_y);
      char *off, *poff;
      off = poff = (char *) ((Uint8*)offset+blit_offset); \
      int c,cc;
      for(c=blit_height;c>0;c--) {
	for(cc=blit_width;cc>0;cc--) {
	  alpha = (Uint8 *) off;
	  if(*(alpha+4)) *(scr+chan) = *(off+chan);
	  scr+=4; off+=4;
	}
	off = poff = poff + geo.pitch;
	scr = pscr = pscr + screen->pitch;
      }
    }
    return;

  case 6: BLIT(+=); return;

  case 7: BLIT(-=); return;
	
  case 8: BLIT(&=); return;

  case 9: BLIT(|=); return;

  default: return;
  }
}
