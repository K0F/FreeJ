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

#include <stdlib.h>
#include <string.h>
#include <iostream.h>
#include <assert.h>
#include <getopt.h>

#include <context.h>
#include <keyboard.h>
#include <v4l.h>
#include <osd.h>
#include <jutils.h>
#include <lubrify.h>
#include <config.h>
#include "filters/filter-list.h"

/* BE WARNED: hires mode is not supported, if you change here
   you get strange results. */
#define LORES 1

#ifdef LORES
#define W 400
#define H 300
#define D 16
#define GW 256
#define GH 256
#else
#define W 512
#define H 384
#define D 16
#define GW 256
#define GH 256
#endif

/* ================ command line options */

static const char *help = 
" .  Usage: freej [options]\n"
" .  options:\n"
" .  -h --help     print this help\n"
" .  -v --version  version information\n"
" .  -d --device   video grabbing device - default /dev/video\n"
" .  -i --input    input channel         - default 1\n"
" .  -D --debug    debug verbosity level - default 1\n"
"\n";

static const struct option long_options[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"device", required_argument, NULL, 'd'},
  {"input", required_argument, NULL, 'i'},
  {"debug", required_argument, NULL, 'D'},
  {0, 0, 0, 0}
};

static const char *short_options = "hvd:i:D:";

char *v4ldevice;
int v4linput;
int debug;

void cmdline(int argc, char *argv[]) {
  int res;

  /* initializing defaults */
  v4ldevice = strdup("/dev/video");
  v4linput = 1;
  debug = 1;

  do {
    res = getopt_long (argc, argv, short_options, long_options, NULL);
    switch(res) {
    case 'h':
      fprintf(stderr, "%s", help);
      exit(0);
      break;
    case 'v':
      cerr << endl;
      exit(0);
      break;
    case 'd':
      free(v4ldevice);
      v4ldevice = strdup(optarg);
      break;
    case 'i':
      v4linput = atoi(optarg);
      if(v4linput>3) {
	fprintf(stderr, "[!] invalid input selected\n");
	exit(1);
      }
      break;
    case 'D':
      debug = atoi(optarg);
      if(debug>3) {
	warning("debug verbosity ranges from 1 to 3\n");
	debug = 3;
      }
      break;
    default:
      break;
    }
  } while (res > 0);
}

/* ===================================== */

int main (int argc, char **argv) {
  
  notice("%s version %s [ http://freej.dyne.org ]",PACKAGE,VERSION);
  act("(c)2001 Denis Roio <jaromil@dyne.org>");
  cmdline(argc,argv);
  set_debug(debug);

  /* sets realtime priority to maximum allowed for SCHED_RR (POSIX.1b) */
  if(set_rtpriority(true))
    notice("running as root: good, high priority realtime scheduling setted");

  /* turn on VERY VERBOSE debugging of multithreading kernel
     LTI::Kernel::trace = true; */

  /* this is the output context (screen) */
  Context screen(W,H,D,0x0);
//  Context screen(W,H,D,SDL_HWSURFACE|SDL_DOUBLEBUF);
  if(screen.surf==NULL) exit(0);

  /* this is a layer: actually only v4l is implemented */
  V4lGrabber grabber;
  
  /* this is the keyboard listener */
  KbdListener keyb;

  /* this is the On Screen Display */
  Osd osd;

  /* detect v4l grabber layer */
  assert( grabber.detect(v4ldevice) );
  
  /* init the v4l grabber */
  assert( grabber.init(&screen,GW,GH,v4linput) );

  /* center the v4l layer */
  grabber.x = (Uint16)(W-GW)>>1;
  grabber.y = (Uint16)(H-GH)>>1;
  /* --------------------- */

  /* init the keyb listener */
  keyb.init(&screen);
  
  /* update the fps counter every 25 frames */
  screen.set_fps_interval(24);

  /* create and initialize the osd */
  osd.init(&screen);
  osd.active();

  grabber.start();
  keyb.start();

  Layer *lay;
  while(!keyb.quit) {
    /* main loop */
    screen.calc_fps();
    lay = (Layer *)screen.layers.begin();

    while(lay != NULL) {
      lay->cafudda();
      lay = (Layer *)lay->next;
    }

    screen.flip();
  }

  /* quitting */
  osd.splash_screen();
  screen.flip();
  grabber.quit = true;

  /* we need to wait here to be sure the threads quitted:
     can't use join because threads are detached for better performance */
  SDL_Delay(3000);

  /* this calls all _delete() methods to safely free all memory */
  screen.close();
  
  exit(1);
}
