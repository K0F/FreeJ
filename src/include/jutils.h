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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <inttypes.h>
#include <errno.h>
#include <console.h>

// max length of (error)messages
#define MAX_ERR_MSG 1024
extern char msg[MAX_ERR_MSG+1];

extern void *(*jmemcpy)( void *to, const void *from, size_t len );

void set_debug(int lev);
int get_debug();
void set_osd(char *st);
void show_osd();
void show_osd(char *format, ...);
void set_console(Console *c);
void notice(char *format, ...);
void func(char *format, ...);
void error(char *format, ...);
void act(char *format, ...);
void warning(char *format, ...);
void *jalloc(size_t size);
bool jfree(void *point);
uint32_t fastrand();
void fastsrand(uint32_t seed);
double dtime();
bool set_rtpriority(bool max);
void jsleep(int sec, long nsec);
int rtc_open();
unsigned long rtc_tick();
void rtc_freq_set(unsigned long freq);
void rtc_freq_wait();
void rtc_close();

bool filecheck(char *file);
bool dircheck(char *file);

#endif


