/*  FreeJ
 *  (c) Copyright 2001-2003 Denis Roio aka jaromil <jaromil@dyne.org>
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

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <context.h>
#include <keyboard.h>

#include <osd.h>
#include <plugger.h>
#include <jutils.h>
#include <config.h>

#ifdef WITH_GLADE2
#include <gtk_ctrl.h>
#endif

#define MAX_CLI_CHARS 4096

/* ================ command line options */

static const char *help = 
" .  Usage: freej [options] [files and video devices]\n"
" .  options:\n"
" .   -h   print this help\n"
" .   -v   version information\n"
" .   -D   debug verbosity level - default 1\n"
" .   -C   start without graphical interface\n"
" .   -s   size of screen - default 400x300\n"
" .   -d   double screen size\n"
" .   -n   start with deactivated layers\n"
" .  files:\n"
" .   you can specify any number of files or devices to be loaded,\n"
" .   this binary is compiled to support the following layer formats:\n";

/*
static const struct option long_options[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"debug", required_argument, NULL, 'D'},
  {"size", required_argument, NULL, 's'},
  {"double", no_argument, NULL, '2'},
  {"zero", no_argument, NULL, '0'},
  {0, 0, 0, 0}
};
*/

static const char *short_options = "-hvD:Cs:dn";

int debug;
char layer_files[MAX_CLI_CHARS];
int cli_chars = 0;
int width = 400;
int height = 300;
bool doublesize = false;
bool startstate = true;
#ifdef WITH_GLADE2
bool gtkgui = true;
#else
bool gtkgui = false;
#endif

void cmdline(int argc, char **argv) {
  int res, optlen;

  /* initializing defaults */
  char *p = layer_files;

  debug = 1;

  do {
    res = getopt (argc, argv, short_options);
    switch(res) {
    case 'h':
      fprintf(stderr, "%s", help);
      fprintf(stderr, "%s", layers_description);
      exit(0);
      break;
    case 'v':
      fprintf(stderr,"\n");
      exit(0);
      break;
    case 'D':
      debug = atoi(optarg);
      if(debug>3) {
	warning("debug verbosity ranges from 1 to 3\n");
	debug = 3;
      }
      break;
    case 'C':
      gtkgui = false;
      break;
    case 's':
      sscanf(optarg,"%ux%u",&width,&height);
      if(width<320) {
	error("display width can't be smaller than 400 pixels");
	width = 320;
      }
      if(height<240) {
	error("display height can't be smaller than 300 pixels");
	width = 240;
      }
      break;

    case 'n':
      startstate = false;
      break;

    case 'd':
      doublesize = true;
      break;
      
    case '?':
      warning("unrecognized option: %s",optarg);
      break;

    case 1:
      optlen = strlen(optarg);
      if( (cli_chars+optlen) < MAX_CLI_CHARS ) {
	sprintf(p,"%s#",optarg);
	cli_chars+=optlen+1;
	p+=optlen+1;
      } else warning("too much files on commandline, list truncated");
      break;

    default:
      // act("received commandline parser code %i with optarg %s",res,optarg);
      break;
    }
  } while (res != -1);

#ifdef HAVE_DARWIN
  for(;optind<argc;optind++) {
    optlen = strlen(argv[optind]);
    if( (cli_chars+optlen) < MAX_CLI_CHARS ) {
      sprintf(p,"%s#",argv[optind]);
	cli_chars+=optlen+1;
	p+=optlen+1;
      } else warning("too much files on commandline, list truncated");
  }
#endif
}

/* ===================================== */

int main (int argc, char **argv) {

  Layer *lay = NULL;

  notice("%s version %s [ http://freej.dyne.org ]",PACKAGE,VERSION);
  act("(c)2001-2003 Denis Rojo < jaromil @ dyne.org >");
  act("----------------------------------------------");
  cmdline(argc,argv);
  set_debug(debug);

  /* sets realtime priority to maximum allowed for SCHED_RR (POSIX.1b)
     this hangs on some kernels - anybody knows what's wrong with it?
     set_rtpriority is inside jutils.cpp 
  if(set_rtpriority(true))
    notice("running as root: high priority realtime scheduling allowed.");
   */


  /* this is the output context (screen) */
  Context freej;
  assert( freej.init(width,height) );

  /* create layers requested on commandline */
  {
    char *l, *p, *pp = layer_files;
    while(cli_chars>0) {

      p = pp;

      while(*p!='#' && cli_chars>0) { p++; cli_chars--; }
      l = p+1;
      if(cli_chars<=0) break; *p='\0';

      lay = create_layer(pp);
      if(lay) {
	lay->init(&freej);
	freej.layers.add(lay);
      }
      pp = l;
    }
  }

  /* even if not specified on commandline
     try to open the default video device */
  lay = create_layer("/dev/video0");
  if(lay) {
    lay->init(&freej);
    freej.layers.add(lay);
  }
  
  /* (no layers && no GUI) then show credits */
  if(freej.layers.len()<1)
    freej.osd.credits();

  /* launch layer threads
  func("rock the house");
  freej.rocknroll(startstate);
  func("OK, rolling"); */

  /* launches the keyboard controller thread*/
  KbdListener keyb;
  assert( keyb.init(&freej));

#ifdef WITH_GLADE2
  /* check if we have an X11 display running */
  if(!getenv("DISPLAY")) gtkgui = false; 
  /* this launches gtk2 interface controller thread */
  if(gtkgui) {
    gtk_ctrl_init(&freej,&argc,argv);
  }
#endif




  /* main loop */
  freej.cafudda();
  


  
  /* quit */

#ifdef WITH_GLADE2
  if(gtkgui) gtk_ctrl_quit();
#endif

  notice("cu on http://freej.dyne.org");
  jsleep(1,0);

  exit(1);
}
