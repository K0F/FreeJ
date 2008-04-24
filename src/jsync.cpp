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

#include <jsync.h>
#include <jutils.h>
#include <config.h>

typedef void* (kickoff)(void*);

JSyncThread::JSyncThread() {

  if(pthread_mutex_init (&_mutex,NULL) == -1)
    error("error initializing POSIX thread mutex");
  //if(pthread_cond_init (&_cond, NULL) == -1)
  //  error("error initializing POSIX thread condtition"); 
  if(pthread_attr_init (&_attr) == -1)
    error("error initializing POSIX thread attribute");

 if(pthread_mutex_init (&_mutex_feed,NULL) == -1)
    error("error initializing POSIX thread feed mutex");
  if(pthread_cond_init (&_cond_feed, NULL) == -1)
    error("error initializing POSIX thread feed condtition"); 

  pthread_attr_setdetachstate(&_attr,PTHREAD_CREATE_JOINABLE);

	set_fps(25);
	fpsd.sum = 0;
	fpsd.i = 0;
	fpsd.n = 30;
	fpsd.data = new float[30];
	for (int i=0; i<30; i++) {
		fpsd.data[i] = 0;
	}
	running = false;
	quit = false;
}


JSyncThread::~JSyncThread() {

  if(pthread_mutex_destroy(&_mutex) == -1)
    error("error destroying POSIX thread mutex");
  //if(pthread_cond_destroy(&_cond) == -1)
  //  error("error destroying POSIX thread condition");
  if(pthread_attr_destroy(&_attr) == -1)
    error("error destroying POSIX thread attribute");
  
  if(pthread_mutex_destroy(&_mutex_feed) == -1)
    error("error destroying POSIX thread feed mutex");
  if(pthread_cond_destroy(&_cond_feed) == -1)
    error("error destroying POSIX thread feed attribute");

	stop();
	delete[] fpsd.data;

}

int JSyncThread::start() {
	if (running)
		return EBUSY;
	quit = false;
	set_alarm(0.0001);
	return pthread_create(&_thread, &_attr, &kickoff, this);
}

void JSyncThread::_run() {
	running = true;
	run();
	running = false;
}

void JSyncThread::stop() {
	if (running) {
		lock_feed();
		quit=true;
		signal_feed();
		unlock_feed();
		join();
	}
}

int JSyncThread::sleep_feed() { 
	if (_fps == 0) {
			wait_feed();
			return EINTR;
	}
	calc_fps();
	//return ETIMEDOUT, EINTR=wecker
	int ret =  pthread_cond_timedwait (&_cond_feed, &_mutex_feed, &wake_ts);
	set_alarm(_delay);
	return ret;
};

void JSyncThread::wait_feed() {
	pthread_cond_wait(&_cond_feed,&_mutex_feed);
	set_alarm(_delay);
};

void JSyncThread::calc_fps() {
	timeval now_tv;
	gettimeofday(&now_tv,NULL);

	float done = now_tv.tv_sec - start_tv.tv_sec +
				(float)(now_tv.tv_usec - start_tv.tv_usec) / 1000000;
	if (done == 0) return;
	float curr_fps = 1 / done;

	if (curr_fps > _fps) // we are in time
		curr_fps = _fps;
	else
		set_alarm(0.0005); // force a little delay

	// statistic only
	fpsd.sum = fpsd.sum - fpsd.data[fpsd.i] + curr_fps;
	fpsd.data[fpsd.i] = curr_fps;
	if (++fpsd.i >= fpsd.n)
		fpsd.i = 0;
}





float JSyncThread::get_fps() {
	return (_fps ? fpsd.sum / fpsd.n : 0 );
}

float JSyncThread::set_fps(float fps_new) {
	float old = _fps;
	fps = fps_new;
	_fps = fps_new;
	if (_fps > 0)
		_delay = 1/_fps;
	return old;
}

void JSyncThread::set_alarm(float delay) {
	gettimeofday(&start_tv,NULL);
	TIMEVAL_TO_TIMESPEC(&start_tv, &wake_ts);
	wake_ts.tv_sec += (int)delay;
	wake_ts.tv_nsec += int(1000000000*(delay - int(delay)));
	if (wake_ts.tv_nsec >= 1000000000) {
		wake_ts.tv_sec ++;
		wake_ts.tv_nsec %= 1000000000;
	}
}


