/*  FreeJ
 *  (c) Copyright 2001-2004 Denis Roio aka jaromil <jaromil@dyne.org>
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <layer.h>
#include <jutils.h>
#include <config.h>

#ifdef WITH_GTK2
#include <gtk/gtk.h>
#endif

#include <impl_layers.h>
#include <context.h>

const char *layers_description =
#ifdef WITH_V4L
" .  - Video4Linux devices as of BTTV cards and webcams\n"
" .    you can specify the size  /dev/video0%160x120\n"
#endif
#ifdef WITH_AVIFILE
" .  - AVI,ASF,WMA,WMV movies as of codecs supported by avifile lib\n"
#endif
#ifdef WITH_FFMPEG
" .  - AVI,ASF,WMA,WMV,MPEG local and remote (http://localhost/file.mpg), dv1394 firewire devices\n"
#endif
#ifdef WITH_PNG
" .  - PNG images (also with transparency)\n"
#endif
#ifdef WITH_FT2
" .  - TXT files rendered with freetype2 library\n"
#endif
#ifdef WITH_XHACKS
" .  - xscreensaver screen hack. ex. /usr/X11R6/lib/xscreensaver/cynosure\n"
#endif
" .  - SWF flash v.3 layer for vectorial graphics animations\n"
" .  - particle generator ( try: 'freej layer_gen' on commandline)\n"
" .  - vertical text scroller (any other extension)\n"
"\n";


Layer *create_layer(Context *env, char *file) {
  char *end_file_ptr,*file_ptr;
  FILE *tmp;
  Layer *nlayer = NULL;

  /* check that file exists */
  if(strncasecmp(file,"/dev/",5)!=0
     && strncasecmp(file,"http://",7)!=0
     && strncasecmp(file,"layer_",6)!=0) {
    tmp = fopen(file,"r");
    if(!tmp) {
      error("can't open %s to create a Layer: %s",
	    file,strerror(errno));
      return NULL;
    } else fclose(tmp);
  }
  /* check file type, add here new layer types */
  end_file_ptr = file_ptr = file;
  end_file_ptr += strlen(file);
//  while(*end_file_ptr!='\0' && *end_file_ptr!='\n') end_file_ptr++; *end_file_ptr='\0';


  /* ==== Video4Linux */
  if(strncasecmp(file_ptr,"/dev/",5)==0 && ! IS_FIREWIRE_DEVICE(file_ptr)) {
#ifdef WITH_V4L
    unsigned int w=env->screen->w, h=env->screen->h;
    while(end_file_ptr!=file_ptr) {
      if(*end_file_ptr!='%') end_file_ptr--;
      else { /* size is specified */
        *end_file_ptr='\0'; end_file_ptr++;
        sscanf(end_file_ptr,"%ux%u",&w,&h);
        end_file_ptr = file_ptr; 
      }
    }
    nlayer = new V4lGrabber();
    if(! ((V4lGrabber*)nlayer)->init( env, (int)w, (int)h) ) {
      error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
      delete nlayer; return NULL;
    }
    if(nlayer->open(file_ptr)) {
        notice("v4l opened");
    //  ((V4lGrabber*)nlayer)->init_width = w;
    //  ((V4lGrabber*)nlayer)->init_heigth = h;
    } else {
      error("create_layer : V4L open failed");
      delete nlayer; nlayer = NULL;
    }
#else
    error("Video4Linux layer support not compiled");
    act("can't load %s",file_ptr);
#endif

  } else /* VIDEO LAYER */

    if( IS_VIDEO_EXTENSION(end_file_ptr) | IS_FIREWIRE_DEVICE(file_ptr)) {
      func("is a movie layer");

#if 0 // MLT
      nlayer = new MovieLayer();
      func("MovieLayer instantiated");
      if(!nlayer->init(env)) {
 	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
 	delete nlayer; return NULL;
      }
      func("MovieLayer initialized");
      if(!nlayer->open(file_ptr)) {
 	error("create_layer : VIDEO open failed");
 	delete nlayer; nlayer = NULL;
      }
#endif

#ifdef WITH_FFMPEG
       nlayer = new VideoLayer();
       if(!nlayer->init( env )) {
 	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
 	delete nlayer; return NULL;
       }
       if(!nlayer->open(file_ptr)) {
 	error("create_layer : VIDEO open failed");
 	delete nlayer; nlayer = NULL;
       }
// #elif WITH_AVIFILE
//       if( strncasecmp(file_ptr,"/dev/ieee1394/",14)==0)
// 	  nlayer=NULL;
//       nlayer = new AviLayer();
//       if(!nlayer->init( env )) {
// 	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
// 	delete nlayer; return NULL;
//       }
//       if(!nlayer->open(file_ptr)) {
// 	error("create_layer : AVI open failed");
// 	delete nlayer; nlayer = NULL;
//       }
 #else
      error("VIDEO and AVI layer support not compiled");
      act("can't load %s",file_ptr);
#endif
  } else /* IMAGE LAYER */
    if( IS_IMAGE_EXTENSION(end_file_ptr)) {
//		strncasecmp((end_file_ptr-4),".png",4)==0) 
	      nlayer = new ImageLayer();
              if(!nlayer->init( env )) {
                error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
                delete nlayer; return NULL;
              }
	      if(!nlayer->open(file_ptr)) {
		  error("create_layer : IMG open failed");
		  delete nlayer; nlayer = NULL;
	      }
  } else /* TXT LAYER */
    if(strncasecmp((end_file_ptr-4),".txt",4)==0) {
#ifdef WITH_FT2
	  nlayer = new TTFLayer();

      if(!nlayer->init( env )) {
	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
	delete nlayer; return NULL;
      }

	  if(!nlayer->open(file_ptr)) {
	    error("create_layer : TXT open failed");
	    delete nlayer; nlayer = NULL;
	  }
#else
	  error("TXT layer support not compiled");
	  act("can't load %s",file_ptr);
	  return(NULL);
#endif

  } else /* XHACKS LAYER */
    if(strstr(file_ptr,"xscreensaver")) {
#ifdef WITH_XHACKS
	    nlayer = new XHacksLayer();

      if(!nlayer->init( env )) {
	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
	delete nlayer; return NULL;
      }

	    if (!nlayer->open(file_ptr)) {
	      error("create_layer : XHACK open failed");
	      delete nlayer; nlayer = NULL;
	    }
#else
	    error("no xhacks layer support");
	    act("can't load %s",file_ptr);
	    return(NULL);
#endif
	  } else if(strncasecmp(file_ptr,"layer_gen",9)==0) {

	    nlayer = new GenLayer();
      if(!nlayer->init( env )) {
	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
	delete nlayer; return NULL;
      }

          } else if(strncasecmp(file_ptr,"layer_goom",10)==0) {

            nlayer = new GoomLayer();

      if(!nlayer->init( env )) {
	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
	delete nlayer; return NULL;
      }

      /*
	    if(!nlayer->open(NULL)) {
	      error("create_layer: Goom can't open audio device");
	      delete nlayer; nlayer = NULL;
	    }
      */

  } else if(strncasecmp(end_file_ptr-4,".swf",4)==0) {

	    nlayer = new FlashLayer();
      if(!nlayer->init( env )) {
	error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
	delete nlayer; return NULL;
      }

	    if(!nlayer->open(file_ptr)) {
	      error("create_layer : SWF open failed");
	      delete nlayer; nlayer = NULL;
	    }

  } else { /* FALLBACK TO SCROLL LAYER */

    func("opening scroll layer on generic file type for %s",file_ptr);
    nlayer = new ScrollLayer();
    
    if(!nlayer->init( env )) {
      error("failed initialization of layer %s for %s", nlayer->name, file_ptr);
      delete nlayer; return NULL;
    }
       
       if(!nlayer->open(file_ptr)) {
	 error("create_layer : SCROLL open failed");
	 delete nlayer; nlayer = NULL;
       }
       
  }

  if(!nlayer)
    error("can't create a layer with %s",file);
  else
    func("create_layer succesful, returns %p",nlayer);
  return nlayer;
}

