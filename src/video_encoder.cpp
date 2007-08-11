/*  FreeJ
 *  (c) Copyright 2005 Silvano Galliani <kysucix@dyne.org>
 *                2007 Denis Rojo       <jaromil@dyne.org>
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


#include <config.h>

#include <string.h>
#include <context.h>

#include <video_encoder.h>


VideoEncoder::VideoEncoder()
  : Entry(), JSyncThread() {

  env = NULL;	

  quit = false;

  running = false;
  initialized = false;

  use_audio = false;

  write_to_disk   = false;
  write_to_stream = false;

  filedump_fd = NULL;

  // initialize the encoded data pipe
  //  encpipe = new Pipe();
  //  encpipe->set_output_type("copy_byte");
  //  encpipe->set_block(true,true);
  //  encpipe->set_block_timeout(500,500);
  //  encpipe->set_block(false,false);
  //  encpipe->debug = true;

  //  pipe(fifopipe);

  ringbuffer = jack_ringbuffer_create(2048*8);

}

VideoEncoder::~VideoEncoder() {
  //  delete encpipe;
  jack_ringbuffer_free(ringbuffer);
}

void VideoEncoder::run() {
  int encnum;
  bool res;

  func("ok, encoder %s in rolling loop",name);
  func("VideoEncoder::run : begin thread %d",pthread_self());
  
  lock_feed();

  running = true;

  wait_feed();
  
  while(!quit) {

    lock();

    res = encode_frame();

    unlock();

    if(!res)
      warning("encoder %s reports error encoding frame",name);

    /// proceed writing and streaming encoded data in encpipe

    if( write_to_disk | write_to_stream )
      encnum = jack_ringbuffer_read(ringbuffer, encbuf, 2048);
      //      encnum = encpipe->read(2048, encbuf);
      //      encnum = read(fifopipe[0], encbuf, 2048);



    if(encnum > 0) {

      func("%s has encoded %i bytes", name, encnum);
      
      if(write_to_disk && filedump_fd) {
	size_t nn;
	nn = fwrite(encbuf, 1, encnum, filedump_fd);
	func("%u bytes written into encoded file", nn);
	
      }
      
      if(write_to_stream) {
	// QUAAA TODO
      }
      
    }
    
    wait_feed();
    
  }
  
  func("VideoEncoder::run : end thread %d", pthread_self() );
  running = false;

}

bool VideoEncoder::cafudda() {
  bool res;

  if(!active) return false;
  

  lock();

  env->screen->lock();
  res = feed_video();
  env->screen->unlock();

  unlock();


  signal_feed();

  return(res);
}

bool VideoEncoder::set_filedump(char *filename) {
  int filename_number=1;
  FILE *fp;

  if(write_to_disk) { // stop current filedump
    if(filedump_fd) {
      fclose(filedump_fd);
      filedump_fd = NULL;
    }
    act("Encoder %s stopped recording to file %s", name, filedump);
    write_to_disk = false;
  }

  // if filename is NULL, stop recording and that's it
  if(!filename) return false;

  // another filename is provided, start recording to it

  // store the filename
  strncpy(filedump,filename,512);


  // file already exists?
  fp = fopen(filedump, "r");
  while(fp) {

    // append a number to the filename
    char tmp[512];
    char *point;
    int lenght_without_extension;

    fclose(fp);
    
    // take point extension pointer ;P
    point = strrchr(filedump,'.');
    lenght_without_extension = (point - filedump);
    
    // copy the string before the  point
    strncpy (tmp, filedump, lenght_without_extension);
    
    // insert -n
    sprintf (tmp + lenght_without_extension, "-%d%s",
	      filename_number, filedump + lenght_without_extension);

    strncpy (filedump, tmp, 512);

    // increment number inside filename
    filename_number++;

    fp = fopen(filedump, "r");
  }

  filedump_fd = fopen(filedump,"w");
  if(!filedump_fd) {
    error("can't record to file %s: %s", filedump_fd, strerror(errno));
    return false;
  }

  act("Encoder %s recording to file %s", name, filedump);
  write_to_disk = true;

	  // if it's a number handle it as a file descriptor
	  // TODO QUAAA
	  //  if (isdigit (atoi (filename)))
	  //    return true;

  return true;
}
