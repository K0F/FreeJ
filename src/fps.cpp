/*  FreeJ
 *  (c) Copyright 2001-2009 Denis Roio <jaromil@dyne.org>
 *                2008-2009 Christoph Rudorff <goil@dyne.org>
 *
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <errno.h>

#include <config.h>

#include <time.h>
#include <sys/time.h>

#include <fps.h>
#include <jutils.h>


FPS::FPS() {
  _fps = 0;

  fpsd.sum = 0;
  fpsd.i = 0;
  fpsd.n = 30;  
  fpsd.data = (float*)calloc(fpsd.n, sizeof(float));
  
  start_tv.tv_sec = start_tv.tv_usec = 0;
  wake_ts.tv_sec = wake_ts.tv_nsec = 0;

  if(pthread_mutex_init (&_mutex,NULL) == -1)
    error("error initializing POSIX thread feed mutex");
  if(pthread_cond_init (&_cond, NULL) == -1)
    error("error initializing POSIX thread feed condtition"); 

}

FPS::~FPS() {
  free(fpsd.data);

  if(pthread_mutex_destroy(&_mutex) == -1)
    error("error destroying POSIX thread feed mutex");
  if(pthread_cond_destroy(&_cond) == -1)
    error("error destroying POSIX thread feed attribute");
  
}
void FPS::init(int rate) {


  this->set(25);

  fpsd.data = new float[30];
  for (int i=0; i<30; i++) {
    fpsd.data[i] = 0;
  }

}

void FPS::calc() {

  timeval done, now_tv;
  float curr_fps;
  uint32_t delta;
  

  gettimeofday(&now_tv,NULL);
  
//   done = now_tv.tv_sec - start_tv.tv_sec +
//     (float)(now_tv.tv_usec - start_tv.tv_usec) / 1000000;
  timersub(&now_tv, &start_tv, &done);

  curr_fps = 1000000L /  done.tv_usec;

  if ( (done.tv_sec > 0)
       || (curr_fps <= _fps) ) {
	 start_tv.tv_sec = now_tv.tv_sec;
	 start_tv.tv_usec = now_tv.tv_usec;
	 return;
  }

  //  delta = (1/_fps)*(curr_fps - _fps - 1);
  delta = ((1.0/ _fps)*1000000.0) - done.tv_usec;
//   func("curr fps: %f :: fps: %f :: delta %d",
//        curr_fps, _fps, delta);
  //  if( (done.tv_usec / 1000000) > _fps
//   if (curr_fps > _fps) // we are in time
//     curr_fps = _fps;
//   else
    //    timeout(0);

  timeout(delta); // force a little delay
  
  // statistic only
  fpsd.sum = fpsd.sum - fpsd.data[fpsd.i] + curr_fps;
  fpsd.data[fpsd.i] = curr_fps;
  if (++fpsd.i >= fpsd.n) fpsd.i = 0;
  
}

int FPS::get() {
  return (_fps ? fpsd.sum / fpsd.n : 0 );
}

int FPS::set(int rate) {
  func("FPS set to %u",rate);
  if (rate < 0) // invalid
    return fps_old;
  
  if (rate != _fps)
    fps_old = _fps;
  
  fps = rate; // public
  _fps = rate;
  if (_fps > 0)
    _delay = 1/_fps;
  return fps_old;
}

void FPS::timeout(uint32_t delta) {
  //  float d = (delta>0) ? delta : _delay;


  //  TIMEVAL_TO_TIMESPEC(&start_tv, &wake_ts);

//   wake_ts.tv_sec += (int)d;
//   wake_ts.tv_nsec += int(1000000000*(d - int(d)));


  wake_ts.tv_sec  = 0;
  wake_ts.tv_nsec = 1000 *delta;



//   if (wake_ts.tv_nsec >= 1000000000) {
//     wake_ts.tv_sec ++;
//     wake_ts.tv_nsec %=   1000000000;
//   }

}

void FPS::delay() {
  struct timespec remaining = { 0, 0 };
  //func("FPS::delay nanosleep : sec[%d] nsec[%d]",
  //      wake_ts.tv_sec, wake_ts.tv_nsec);

  do {
    nanosleep(&wake_ts, NULL);
  } while (remaining.tv_nsec > 0);
  // update lo start time
  gettimeofday(&start_tv,NULL);

}
