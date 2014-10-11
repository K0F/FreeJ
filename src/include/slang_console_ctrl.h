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

#ifndef __SLANG_CONSOLE_H__
#define __SLANG_CONSOLE_H__

#include <controller.h>
#include <linklist.h>
#include <console_ctrl.h>
#include <slang.h>



#define PLAIN_COLOR 1
#define TITLE_COLOR 1
#define LAYERS_COLOR 3
#define FILTERS_COLOR 4
#define SCROLL_COLOR 5



/* TODO port to slang 2
   #if SLANG_VERSION >= 20000
   #define SLANG_ERROR()  SLang_get_error()
   #else
   #define SLANG_ERROR()  SLang_Error
   #endif
 */

extern volatile int SLang_Error; // hack for fucking debian!

FREEJ_FORWARD_PTR(SLangConsole)
FREEJ_FORWARD_PTR(SlwSelector)
FREEJ_FORWARD_PTR(SlwTitle)
FREEJ_FORWARD_PTR(SLW_Log)
FREEJ_FORWARD_PTR(SlwReadline)

FREEJ_FORWARD_PTR(SlwConsole)
class SlwConsole : public ConsoleController {
public:

    SlwConsole(const ContextPtr &env);
    ~SlwConsole();

    int poll();
    int dispatch();

    bool console_init();

    bool slw_init();

    void close() {
    };

    void notice(const char *msg);
    void error(const char *msg);
    void warning(const char *msg);
    void act(const char *msg);
    void func(const char *msg);
    void old_printlog(const char *msg);
    void logmsg(LogLevel level, const char *msg);

    void refresh();

private:
    bool active;

    ContextPtr env;
    SLangConsolePtr slw;
    SlwSelectorPtr sel;
    SlwTitlePtr tit;
    SLW_LogPtr log;
    SlwReadlinePtr rdl;

    int x, y;

    int paramsel;

    int movestep;


};



#endif
