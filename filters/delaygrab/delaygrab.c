/*
  FreeJ effect Delaygrab

  delaygrab - blockwise, controllable image delay

  Copyright (C) 1999/2000  A. Schiffler <aschiffler@home.com>
  Copyright (C) 2001/2002  Denis Rojo <jaromil@dyne.org>
  
  original sourcecode is from libbgrab 2.1f
  ported to FreeJ and successively modified
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
*/

#include <freej.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>



#define QUEUEDEPTH 71 /* was 76 */
#define MODES 4

static char *name = "Delaymap";
static char *author = "Andreas Schiffler";
static char *info = "map delayed frame blitting";
static int version = 2;
char *getname() { return name; };
char *getauthor() { return author; };
char *getinfo() { return info; };
int getversion() { return version; };

static int x,y,i,xyoff,v;
static uint8_t *imagequeue,*curqueue;
static int curqueuenum;
static uint32_t *curdelaymap;
static uint8_t *curpos,*curimage;
static int curposnum;
static void *delaymap;
static void *procbuf;

/* initialized from the init */
static int delaymapwidth;  /* width/blocksize */
static int delaymapheight; /* height/blocksize */
static int delaymapsize;   /* delaymapheight*delaymapwidth */

static int blocksize;
static int block_per_pitch;
static int block_per_bytespp;
static int block_per_res;

static int current_mode;

static ScreenGeometry *geo;

static void createDelaymap(int mode);
static void set_blocksize(int bs);
static int isqrt(unsigned int x);
static uint32_t fastrand();
void fastsrand(uint32_t seed);

int init(ScreenGeometry *sg) {
  geo = sg;

  delaymap=NULL;
  imagequeue = (uint8_t *) malloc(QUEUEDEPTH*(geo->size));
  procbuf = malloc(geo->size);

  /* starting mode */
  current_mode = 4;
  /* starting blocksize */
  set_blocksize(2);
  
  curqueue=imagequeue;
  curqueuenum=0;

  fastsrand(time(NULL));
  return(1);
}

int clean() {
  if(delaymap) free(delaymap);
  free(imagequeue);
  free(procbuf);
  return(1);
}

void *process(void *buffo) {

  /* Update queue pointer */
  if (curqueuenum==0) {
    curqueuenum=QUEUEDEPTH-1;
    curqueue = imagequeue;
    curqueue += (geo->size*(QUEUEDEPTH-1));
  } else {
    curqueuenum--;
    curqueue -= geo->size;
  }

   /* Copy image to queue */
  memcpy(curqueue,buffo,geo->size);

     /* Copy image blockwise to screenbuffer */
  curdelaymap= (uint32_t *)delaymap;
  for (y=0; y<delaymapheight; y++) {
    for (x=0; x<delaymapwidth; x++) {

      curposnum=((curqueuenum + (*curdelaymap)) % QUEUEDEPTH);
      
      xyoff= (x*block_per_bytespp) + (y*block_per_pitch);
      /* source */
      curpos= imagequeue;
      curpos += (geo->size*curposnum);
      curpos += xyoff;
      /* target */
      curimage = (uint8_t *)procbuf;
      curimage += xyoff;
      /* copy block */
      for (i=0; i<blocksize; i++) {
	memcpy(curimage,curpos,block_per_res);
	curpos += geo->pitch;
	curimage += geo->pitch;
      }
      curdelaymap++;
    }
  }

  return(procbuf);
}

int kbd_input(char key) {
  int res = 1;
  switch(key) {
  case 'w':
    if(current_mode<MODES) createDelaymap(current_mode+1);
    break;
  case 'q':
    if(current_mode>1) createDelaymap(current_mode-1);
    break;
  case 's':
    set_blocksize(blocksize+1);
    break;
  case 'a':
    if(blocksize>2) set_blocksize(blocksize-1);
    break;
  default:
    res = 0;
    break;
  }
  return res;
}

static void createDelaymap(int mode) {
  double d;

  curdelaymap=(uint32_t *)delaymap;
  fastsrand(time(NULL));

  for (y=delaymapheight; y>0; y--) {
    for (x=delaymapwidth; x>0; x--) {
      switch (mode) {
      case 1:	
	/* Random delay with square distribution */
	d = (double)fastrand()/(double)RAND_MAX;
	*curdelaymap = (int)(d*d*16.0);
	break;
      case 2:
	/* Vertical stripes of increasing delay outward from center */
	if (x<(delaymapwidth/2)) {
	  v=(delaymapwidth/2)-x;
	} else if (x>(delaymapwidth/2)) {
	  v=x-(delaymapwidth/2);
	} else {
	  v=0;
	}
	*curdelaymap=v/2;
	break;
      case 3:
	/* Horizontal stripes of increasing delay outward from center */
	if(y<(delaymapheight/2)) {
	  v = (delaymapheight/2)-y;
	} else if(y>(delaymapheight/2)) {
	  v = y-(delaymapheight/2);
	} else {
	  v=0;
	}
	*curdelaymap=v/2;
	break;
      case 4:
	/* Rings of increasing delay outward from center */
	v = (int)isqrt((unsigned int)((x-(delaymapwidth/2))*
				      (x-(delaymapwidth/2))+
				      (y-(delaymapheight/2))*
				      (y-(delaymapheight/2))));
	*curdelaymap=v/2;
	break;
      } // switch
      /* Clip values */
      if (*curdelaymap<0) {
	*curdelaymap=0;
      } else if (*curdelaymap>(QUEUEDEPTH-1)) {
	*curdelaymap=(QUEUEDEPTH-1);
      }
      curdelaymap++;
    }
  }
  current_mode = mode;
}

static void set_blocksize(int bs) {

  blocksize = bs;
  block_per_pitch = blocksize*(geo->pitch);
  block_per_bytespp = blocksize*(geo->bpp>>3);
  block_per_res = blocksize<<(geo->bpp>>4);
  
  delaymapwidth = (geo->w)/blocksize;
  delaymapheight = (geo->h)/blocksize;
  delaymapsize = delaymapheight*delaymapwidth;

  if(delaymap) { free(delaymap); delaymap = NULL; }
  delaymap = malloc(delaymapsize*4);

  createDelaymap(current_mode);
}

/* i learned this on books // by jaromil */
static int isqrt(unsigned int x) {
  unsigned int m, y, b; m = 0x40000000;
  y = 0; while(m != 0) { b = y | m; y = y>>1;
  if(x>=b) { x=x-b; y=y|m; }
  m=m>>2; } return y;
}

/*
 * fastrand - fast fake random number generator
 * by Fukuchi Kentarou
 * Warning: The low-order bits of numbers generated by fastrand()
 *          are bad as random numbers. For example, fastrand()%4
 *          generates 1,2,3,0,1,2,3,0...
 *          You should use high-order bits.
 *
 */

static uint32_t randval;

uint32_t fastrand()
{
	return (randval=randval*1103515245+12345);
}

void fastsrand(uint32_t seed)
{
	randval = seed;
}
