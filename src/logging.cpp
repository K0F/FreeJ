/*
 * Copyright (C) 2009 - Luca Bigliardi
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <logging.h>
#include <console_ctrl.h>


int Logger::printlog(LogLevel level, const char *format, ...) {
    va_list arg;
    int rv;
    va_start(arg, format);
    rv = vprintlog(level, format, arg);
    va_end(arg);
    return rv;
}

int Logger::vprintlog(LogLevel level, const char *format, va_list arg) {
    return 0;
}

Logger::~Logger() {
}

Loggable::Loggable() : logger_(NULL), loglevel_(INFO) {
    int r;
    if((r = pthread_mutex_init(&logger_mutex_, NULL)) != 0)
        throw Error("Initializing logger_mutex_", r);
}

Loggable::~Loggable() {
    int r;
    if((r = pthread_mutex_destroy(&logger_mutex_)) != 0)
        GlobalLogger::printlog(ERROR, "In %s , pthread_mutex_destroy(): %s",
                               __PRETTY_FUNCTION__, strerror(r));
}

bool Loggable::register_logger(LoggerPtr l) {
    pthread_mutex_lock(&logger_mutex_);
    if(logger_) {
        log(ERROR, "%s: a logger has already been registered",
            __PRETTY_FUNCTION__);
        return false;
    }
    logger_ = l;
    pthread_mutex_unlock(&logger_mutex_);
    return true;
}

bool Loggable::unregister_logger(LoggerPtr l) {
    pthread_mutex_lock(&logger_mutex_);
    if(logger_ != l) {
        log(ERROR, "%s: trying to unregister a non-registered logger",
            __PRETTY_FUNCTION__);
        return false;
    }
    logger_ = NULL;
    pthread_mutex_unlock(&logger_mutex_);
    return true;
}

int Loggable::log(LogLevel level, const char *format, ...) {
    va_list arg;
    int rv = 0;

    va_start(arg, format);
    rv = vlog(level, format, arg);
    va_end(arg);
    return rv;
}

int Loggable::vlog(LogLevel level, const char *format, va_list arg) {
    int rv = 0; // return 0 if nothing is logged

    if(level <= loglevel_) {
        pthread_mutex_lock(&logger_mutex_);
        if(logger_)
            rv = logger_->vprintlog(level, format, arg);
        else
            rv = GlobalLogger::vprintlog(level, format, arg);
        pthread_mutex_unlock(&logger_mutex_);
    }
    return rv;
}

LogLevel GlobalLogger::loglevel_ = INFO;
LoggerPtr GlobalLogger::logger_;
pthread_mutex_t GlobalLogger::logger_mutex_ = PTHREAD_MUTEX_INITIALIZER;
char GlobalLogger::logbuf_[MAX_LOG_MSG + 1] = {0};

LogLevel GlobalLogger::get_loglevel() {
    return loglevel_;
}

void GlobalLogger::set_loglevel(LogLevel level) {
    loglevel_ = level;
}

bool GlobalLogger::register_logger(LoggerPtr l) {
    pthread_mutex_lock(&logger_mutex_);
    if(logger_) {
        printlog(ERROR, "%s: a logger has already been registered",
                 __PRETTY_FUNCTION__);
        return false;
    }
    logger_ = l;
    pthread_mutex_unlock(&logger_mutex_);
    return true;
}

bool GlobalLogger::unregister_logger(LoggerPtr l) {
    pthread_mutex_lock(&logger_mutex_);
    if(logger_ != l) {
        printlog(ERROR, "%s: trying to unregister a non-registered logger",
                 __PRETTY_FUNCTION__);
        return false;
    }
    logger_ = NULL;
    pthread_mutex_unlock(&logger_mutex_);
    return true;
}

int GlobalLogger::printlog(LogLevel level, const char *format, ...) {
    va_list arg;
    int rv;
    va_start(arg, format);
    rv = vprintlog(level, format, arg);
    va_end(arg);
    return rv;
}

int GlobalLogger::vprintlog(LogLevel level, const char *format, va_list arg) {
    int rv = 0; // return 0 if nothing is logged

    if(level <= loglevel_) {
        pthread_mutex_lock(&logger_mutex_);
        if(logger_) {
            rv = logger_->vprintlog(level, format, arg);
        } else {
            vsnprintf(logbuf_, MAX_LOG_MSG, format, arg);
            const char *prefix = NULL;
            switch(level) {
            case ERROR:
                prefix = "[!]";
                break;
            case WARNING:
                prefix = "[W]";
                break;
            case NOTICE:
                prefix = "[*]";
                break;
            case INFO:
                prefix = " . ";
                break;
            case DEBUG:
                prefix = "[F]";
                break;
            default:
                prefix = "[WTF?]";
                break;
            }
            fprintf(stderr, "%s %s\n", prefix, logbuf_);
        }
        pthread_mutex_unlock(&logger_mutex_);
    }
    return rv;
}

WrapperLogger::WrapperLogger() {
    int r;
    if((r = pthread_mutex_init(&logbuf_mutex_, NULL)) != 0)
        throw Error("Initializing logbuf_mutex_", r);
    if((logbuf_ = (char *)malloc(sizeof(char) * (MAX_LOG_MSG + 1))) == NULL) {
        pthread_mutex_destroy(&logbuf_mutex_);
        throw Error("Allocating logbuf_", 0);
    }
}

WrapperLogger::~WrapperLogger() {
    int r;
    if((r = pthread_mutex_destroy(&logbuf_mutex_)) != 0)
        GlobalLogger::printlog(ERROR, "In %s , pthread_mutex_destroy(): %s",
                               __PRETTY_FUNCTION__, strerror(r));
    free(logbuf_);
}

int WrapperLogger::vprintlog(LogLevel level, const char *format, va_list arg) {
    int rv;
    pthread_mutex_lock(&logbuf_mutex_);
    rv = vsnprintf(logbuf_, MAX_LOG_MSG, format, arg);
    logmsg(level, logbuf_);
    pthread_mutex_unlock(&logbuf_mutex_);
    return rv;
}

void WrapperLogger::logmsg(LogLevel level, const char *msg) {
    return;
}

// These are for backward compatibility:
void set_debug(int lev) {
    // In the old logging there was no proper scale,
    // let's do something as compatible as possible..
    if(lev <= 1)
        GlobalLogger::set_loglevel(INFO);
    else
        GlobalLogger::set_loglevel(DEBUG);
}

int get_debug() {
    // In the old logging there was no proper scale,
    // let's do something as compatible as possible..
    switch(GlobalLogger::get_loglevel()) {

    case DEBUG:
        return 3;
    case WARNING:
    case INFO:
    case NOTICE:
    case ERROR:
        return 1;
    case QUIET:
        return 0;
    default:
        return 1;
    }
}

void error(const char *format, ...) {

    // avoid processing (faster when not debugging)
    if(GlobalLogger::get_loglevel() < ERROR) return;

    va_list arg;
    va_start(arg, format);
    GlobalLogger::vprintlog(ERROR, format, arg);
    va_end(arg);
}

void warning(const char *format, ...) {

    // avoid processing (faster when not debugging)
    if(GlobalLogger::get_loglevel() < WARNING) return;

    va_list arg;
    va_start(arg, format);
    GlobalLogger::vprintlog(WARNING, format, arg);
    va_end(arg);
}

void notice(const char *format, ...) {

    // avoid processing (faster when quiet)
    if(GlobalLogger::get_loglevel() < NOTICE) return;

    va_list arg;
    va_start(arg, format);
    GlobalLogger::vprintlog(NOTICE, format, arg);
    va_end(arg);
}

void act(const char *format, ...) {

    // avoid processing (faster when quiet)
    if(GlobalLogger::get_loglevel() < INFO) return;

    va_list arg;
    va_start(arg, format);
    GlobalLogger::vprintlog(INFO, format, arg);
    va_end(arg);
}

void func(const char *format, ...) {

    // avoid processing (faster when quiet)
    if(GlobalLogger::get_loglevel() < DEBUG) return;

    va_list arg;
    va_start(arg, format);
    GlobalLogger::vprintlog(DEBUG, format, arg);
    va_end(arg);
}

