/*  FreeJ
 *  (c) Copyright 2001-2010 Denis Roio <jaromil@dyne.org>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 3 of the License,
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

/* @file plugger.h
   @brief Plugin Filter dispatcher header
 */

#ifndef __plugger_h__
#define __plugger_h__

#include <string.h>

#include "jutils.h"
#include "filter.h"

FREEJ_FORWARD_PTR(Context)

template <class T> class LinkList;

/**
   This class implements the object storing all available filter
   instances and dispatching them for FreeJ operations.

   It reads thru paths ($(prefix)/lib/freej and ~/.freej) looking for
   valid plugins and creates instances of them which are ready to be
   returned upon request to the host application of FreeJ controllers.

   @brief Collects DLO plugins that can be used as Effect or Layer
 */
class Plugger {
    friend Context;
public:
    Plugger(); ///< Plugger onstructor
    ~Plugger(); ///< Plugger destructor

    LinkList<Filter> getFilters();
    LinkList<Filter> getGenerators();

private:

    bool open(ContextPtr env, char *file);

    /* checks if file/directory exist */
    void addsearchdir(const char *dir);
    void _setsearchpath(const char *path) {
        if(_searchpath) free(_searchpath);
        _searchpath = strdup(path);
    };
    char *_getsearchpath() {
        return(_searchpath);
    };

    char *_searchpath;

};

#endif
