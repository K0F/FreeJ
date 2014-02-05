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

 * this file includes a malloc wrapper, it acts very verbose when debug
 * mode allow it
 */

#include <stdio.h>

#ifndef WIN32
#ifdef linux
/* we try to use the realtime linux clock on /dev/rtc */
#include <linux/rtc.h>
#endif
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

/* freej specific: */
#include <console_ctrl.h>

#include <jutils.h>
#include <config.h>

/*
 * Obsolete memory-aligned implementation
 * void *jalloc(size_t size) {
 * 	void *buf;
 * #ifndef HAVE_DARWIN
 * 	int res;
 * 	res = posix_memalign(&buf, 32, size);
 * 	if(res!=0) {
 * 		if(res==ENOMEM)
 * 			error("insufficient memory to allocate buffer");
 * 		if(res==EINVAL)
 * 			error("invalid memory alignement to 32 bytes in buffer allocation");
 * 		return NULL;
 * 	}
 * #else
 * 	buf = malloc(size);
 * #endif
 * 	func("allocated %u bytes of memory at %p",size,buf);
 * 	return(buf);
 * }
 */

/*
 * Obsolete
 * bool jfree(void *point) {
 *
 *   if(point==NULL) {
 *     warning("requested free on a NULL pointer");
 *     return(false);
 *   }
 *
 *   //  if(verbosity>=FUNC)
 *   //    fprintf(stderr,"[M] freeing memory at address %p\n",point);
 *
 *   free(point);
 *   point = NULL;
 *   return(true);
 * }
 */

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

uint32_t fastrand() {
    //    kentaro's original one:
    //	return (randval=randval*1103515245+12345);
    //15:55  <salsaman2> mine uses two prime numbers and the cycling is much reduced
    //15:55  <salsaman2>   return (randval=randval*1073741789+32749);
    return(randval = randval * 1073741789 + 32749 );
}

void fastsrand(uint32_t seed) {
    randval = seed;
}

double dtime() {
    struct timeval mytv;
    gettimeofday(&mytv,NULL);
    return((double)mytv.tv_sec+1.0e-6*(double)mytv.tv_usec);
}

#ifdef linux
#include <sched.h>
/* sets the process to "policy" policy,  if max=1 then set at max priority,
   else use min priority */

bool set_rtpriority(bool max) {
    struct sched_param schp;
    // set the process to realtime privs

    memset(&schp, 0, sizeof(schp));

    if(max)
        schp.sched_priority = sched_get_priority_max(SCHED_RR);
    else
        schp.sched_priority = sched_get_priority_min(SCHED_RR);

    if (sched_setscheduler(0, SCHED_RR, &schp) != 0)
        return false;
    else
        return true;
}
#endif

/* handle signals.
 * From the manpage:
 * nanosleep  delays  the execution of the program for at least
 * the time specified in *req.  The function can return earlier
 * if a signal has been delivered to the process. In this case,
 * it returns -1, sets errno to EINTR, and writes the remaining
 * time into the structure pointed to by rem unless rem is
 * NULL.  The value of *rem can then be used to call nanosleep
 * again and complete the specified pause.
 */
void jsleep(int sec, long nsec) {
    struct timespec tmp_rem,*rem;
    rem = &tmp_rem;
    timespec timelap;
    timelap.tv_sec = sec;
    timelap.tv_nsec = nsec;
    while (nanosleep (&timelap, rem) == -1 && (errno == EINTR));
}


/* small RTC interface by jaromil
   all comes from the Linux Kernel Documentation */
#ifdef linux
/* better to use /dev/rtc */
static int rtcfd = -1;
static fd_set readfds;
static timeval rtctv = { 0,0 };
static unsigned long rtctime;
int rtc_open() {
    int res;
    rtcfd = open("/dev/rtc",O_RDONLY);
    if(!rtcfd) {
        perror("/dev/rtc");
        return 0;
    }
    /* set the alarm event to 1 second */
    res = ioctl(rtcfd, RTC_UIE_ON, 0);
    if(res<0) {
        perror("rtc ioctl");
        return 0;
    }
    notice("realtime clock successfully initialized");
    return 1;
}
/* tick returns 0 if 1 second didn't passed since last tick,
   positive number if 1 second passed */
unsigned long rtc_tick() {
    FD_ZERO(&readfds);
    FD_SET(rtcfd,&readfds);
    if ( ! select(rtcfd+1,&readfds,NULL,NULL,&rtctv) )
        return 0; /* a second didn't passed yet */
    read(rtcfd,&rtctime,sizeof(unsigned long));
    return rtctime;
}
void rtc_freq_set(unsigned long freq) {
    int res;

    res = ioctl(rtcfd,RTC_IRQP_SET,freq);
    if(res<0) {
        perror("rtc freq set");
    }

    res = ioctl(rtcfd,RTC_IRQP_READ,&freq);
    if(res<0) {
        perror("rtc freq read");
    }

    act("realtime clock frequency set to %ld",freq);

    res = ioctl(rtcfd,RTC_PIE_ON,0);
    if(res<0) {
        perror("rtc freq on");
        return;
    }

}
void rtc_freq_wait() {
    int res;
    res = read(rtcfd,&rtctime,sizeof(unsigned long));
    if(res < 0) {
        perror("read rtc frequency interrupt");
        return;
    }
}
void rtc_close() {
    if(rtcfd<=0) return;
    ioctl(rtcfd, RTC_UIE_OFF, 0);
    //  ioctl(rtcfd,RTC_PIE_OFF,0);
    close(rtcfd);
}
#endif

void *(* jmemcpy)(void *to, const void *from, size_t len);


bool filecheck(const char *file) {
    bool res = true;
    FILE *f = fopen(file,"r");
    if(!f) res = false;
    else fclose(f);
    return(res);
}

bool dircheck(const char *dir) {
    bool res = true;
    DIR *d = opendir(dir);
    if(!d) res = false;
    else closedir(d);
    return(res);
}
