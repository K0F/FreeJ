/*  FreeJ - blitter layer component
 *
 *  (c) Copyright 2004-2009 Denis Roio aka jaromil <jaromil@dyne.org>
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
 *
 */
 
#include "blit.h"
#include "blit_instance.h"

Blit::Blit(BlitType type, const std::string &name, const std::string &description, void *fun, LinkList<Parameter> &parameters) : Entry() {
    this->type = type;
    this->name = name;
    this->description = description;
    this->fun = fun;
    this->parameters.insert(this->parameters.end(), parameters.begin(), parameters.end());
}

Blit::Blit(BlitType type, const std::string &name, const std::string &description, void *fun, LinkList<Parameter> &&parameters) : Blit(type, name, description, fun, parameters) {
}

Blit::~Blit() {
}

BlitInstancePtr Blit::new_instance() {
    BlitInstancePtr instance = MakeShared<BlitInstance>();
    if(instance)
        instance->init(SharedFromThis(Blit));
    return instance;
}

