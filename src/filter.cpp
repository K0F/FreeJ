/*  FreeJ - New Freior based Filter class
 *  Copyright (C) 2001-2010 Denis Roio <jaromil@dyne.org>
 *  Copyright (C) 2010    Andrea Guzzo <xant@dyne.org>
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

   $Id: $

 */

#include "config.h"
#include "layer.h"
#include "filter.h"

#include "frei0r_freej.h"
#include "freeframe_freej.h"


#include <algorithm>

#include "jutils.h"

Filter::Filter()
    : Entry() {
    description = "Unknown";
    author = "Unknown";
}

Filter::~Filter() {

}

FilterInstancePtr Filter::new_instance() {
    FilterInstancePtr instance = Factory<FilterInstance>::new_instance("FilterInstance");
    if(instance)
        instance->init(SharedFromThis(Filter));
    return instance;
}

const std::string &Filter::getDescription() const {
    return description;
}

const std::string &Filter::getAuthor() const {
    return author;
}
