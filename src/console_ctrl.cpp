/*  FreeJ - S-Lang console
 *
 *  (c) Copyright 2004-2009 Denis Roio <jaromil@dyne.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <signal.h>

#include "config.h"

#include <slang.h>

#include <slw_log.h>

#include "slang_console_ctrl.h"
#include "console_calls_ctrl.h"
#include "console_widgets_ctrl.h"
#include "console_readline_ctrl.h"

#include <slw_console.h>
#include <keycodes.h>

#include "context.h"
#include "blitter.h"

#include "fps.h"

#include "jutils.h"

#include "generator_layer.h"

static LayerPtr consoleSelectedLayer;
static ViewPortPtr consoleSelectedScreen;
static FilterInstancePtr consoleSelectedFilter;

const ViewPortPtr& getSelectedScreen() {
    return consoleSelectedScreen;
}

void setSelectedScreen(const ViewPortPtr& screen) {
    consoleSelectedScreen = screen;
    if(screen) {
        LinkList<Layer> &list = screen->getLayers();
        if(list.size() > 0) {
            setSelectedLayer(list.front());
        } else {
            setSelectedLayer(NULL);
        }
    }
}

const LayerPtr& getSelectedLayer() {
    return consoleSelectedLayer;
}

void setSelectedLayer(const LayerPtr& layer) {
    consoleSelectedLayer = layer;
    if(layer) {
        LinkList<FilterInstance> &list = layer->getFilters();
        if(list.size() > 0) {
            setSelectedFilter(list.front());
        } else {
            setSelectedFilter(NULL);
        }
    }
}

const FilterInstancePtr& getSelectedFilter() {
    return consoleSelectedFilter;
}

void setSelectedFilter(const FilterInstancePtr& filter) {
    consoleSelectedFilter = filter;
}

static bool screen_size_changed;
static void sigwinch_handler(int sig) {
    screen_size_changed = true;
    SLsignal(SIGWINCH, sigwinch_handler);
}

static bool real_quit;
static bool keyboard_quit = false;
static void sigint_handler(int sig) {
    SLsignal_intr(SIGINT, sigint_handler);
    keyboard_quit = true;
    func("%s : keyboard quit", __PRETTY_FUNCTION__);
#if SLANG_VERSION < 20000
    if(SLang_Ignore_User_Abort == 0)
        SLang_Error = USER_BREAK;
#endif
}

/* non blocking getkey */
static int getkey_handler() {
    unsigned int ch = 0;
    if(SLang_input_pending(0))
        //    return SLang_getkey();
        ch = SLang_getkey();

    //  SLang_flush_input(); // no slow repeat

    //  if(ch) func("SLang_getkey in getkey_handler detected char %u",ch);
    return ch;
}

// confirm quit
int quit_proc(ContextPtr env, char *cmd) {
    if(!cmd) return 0;
    if(cmd[0] == 'y') {
        real_quit = true;
        return 1;
    }
    real_quit = false;
    return 0;
}

SlwConsole::SlwConsole(const ContextPtr& env) : ConsoleController() {
    this->env = env;
    active = false;
    paramsel = 1;

    slw = NULL;
    sel = NULL;
    log = NULL;
    tit = NULL;
    rdl = NULL;

    // the console might be the main controller used in debugging
    // more often than in other cases, so we make it indestructible
    // by default, meaning that reset() won't delete it.
    indestructible = true;

    name = "Console";
}

SlwConsole::~SlwConsole() {
    SLtt_set_cursor_visibility(1);
}

bool SlwConsole::console_init() {
    slw_init();
    return true;
}

bool SlwConsole::slw_init() {
    ::func("%s", __PRETTY_FUNCTION__);

    slw = MakeShared<SLangConsole>();
    slw->init();

    /** register WINdow CHange signal handler (TODO) */
    SLsignal(SIGWINCH, sigwinch_handler);

    /** register SIGINT signal */
    signal(SIGINT, sigint_handler);
    SLang_set_abort_signal(sigint_handler);

    SLkp_set_getkey_function(getkey_handler);

    SLtt_set_cursor_visibility(0);

    // title
    tit = MakeShared<SlwTitle>(env);
    slw->place(tit, 0, 0, slw->w, 2);
    tit->init();
    /////////////////////////////////

    // layer and filter selector
    sel = MakeShared<SlwSelector>(env);
    slw->place(sel, 0, 2, slw->w, 8);
    sel->init();
    ////////////////////////////

    // log scroller
    log = MakeShared<SLW_Log>();
    slw->place(log, 0, 10, slw->w, slw->h - 3);
    log->init();
    ////////////////////////////


    // status line
    rdl = MakeShared<SlwReadline>();
    slw->place(rdl, 0, slw->h - 1, slw->w, slw->h);
    rdl->init();
    ////////////////////////////

    refresh();

    initialized = true;
    active = true;
    return true;
}

int SlwConsole::dispatch() {
    int key = SLkp_getkey();

//  if(key) ::func("SLkd_getkey: %u",key);
//  else return; /* return if key is zero */
    if(!key) return(0);

    if(key == KEY_CTRL_L) {
        tit->blank();
        log->blank();
        sel->blank();
        rdl->blank();
    }

    if(rdl->feed(key)) return(1);

    if(log->feed(key)) return(1);

    if(sel->feed(key)) return(1);



    return(0);
}

int SlwConsole::poll() {
    if(keyboard_quit) {
        rdl->readline("do you really want to quit? type yes to confirm:", &quit_proc, NULL);
        keyboard_quit = false;
        return 0;
    }

    if(real_quit) {
        notice("QUIT requested from console! bye bye");
        env->stop();
        real_quit = false;
        return 0;
    }

    dispatch();


    // refresh all widgets
    refresh();

    return(1);
}

void SlwConsole::refresh() {
    tit->refresh();
    log->refresh();
    sel->refresh();
    rdl->refresh();
    slw->refresh();

//   SLsmg_cls();
//   canvas();
//   speedmeter();
//   //  update_scroll();
//   if(!commandline)
//     statusline(NULL);
//   else
//     GOTO_CURSOR;

}

void SlwConsole::notice(const char *msg) {
    log->append(msg);
}

void SlwConsole::warning(const char *msg) {
    log->append(msg);
}

void SlwConsole::act(const char *msg) {
    log->append(msg);
}

void SlwConsole::error(const char *msg) {
    log->append(msg);
}

void SlwConsole::func(const char *msg) {
    log->append(msg);
}

void SlwConsole::old_printlog(const char *msg) {
    log->append(msg);
}

void SlwConsole::logmsg(LogLevel level, const char *msg) {
    old_printlog(msg);
}


