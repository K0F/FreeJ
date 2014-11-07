/*
 *  CVFilter.h
 *  freej
 *
 *  Created by xant on 5/4/10.
 *  Copyright 2010 dyne.org. All rights reserved.
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
 *
 * "$Id:$"
 *
 */

/*  FreeJ - Frei0r wrapper
 *  (c) Copyright 2007 Denis Rojo <jaromil@dyne.org>
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
 *
 */

#ifndef __CVFILTER_H__
#define __CVFILTER_H__

#include "config.h"
//#ifdef WITH_COCOA
#include <filter.h>
#include <factory.h>
#include <linklist.h>

#define FILTERS_MAX 18

typedef struct __FilterParams {
    int nParams;
    struct __ParamDescr {
        char *label;
        double min;
        double max;
    } params[4];
} FilterParams;

class CVFilter: public Filter {

public:
    static void listFilters(Linklist<Filter> &outputList);

    CVFilter();
    virtual ~CVFilter();
    int type();
    int open(char *name);
    bool apply(Layer *lay, FilterInstance *instance);
    const char *description();
    void print_info();
    char *get_parameter_description(int i);
    bool opened;
protected:
    void destruct(FilterInstance *inst);
    void update(FilterInstance *inst, double time, uint32_t *inframe, uint32_t *outframe);
	void init_parameters(Linklist<Parameter> &parameters);  

private:
    FilterParams *desc;
    FACTORY_ALLOWED
};

//#endif // WITH_COCOA

#endif
