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

#ifndef __FREI0R_H__
#define __FREI0R_H__

#include <config.h>
#ifdef WITH_FREI0R

#include <vector>

#include <linklist.h>
#include <frei0r.h>
#include <filter.h>
#include <factory.h>


FREEJ_FORWARD_PTR(FreiorParameter)

FREEJ_FORWARD_PTR(Freior)
class Freior : public Filter {
    friend class FreiorInstance;
    friend class FreiorParameterInstance;
    friend class GenF0rLayer;
#ifdef WITH_COCOA
    friend class CVF0rLayer;
#endif
public:

    Freior();
    virtual ~Freior();

    virtual int type();
    int open(char *file);
    virtual FilterInstancePtr new_instance();
    inline const f0r_plugin_info_t getInfo() const {
        return info;
    }


protected:
    f0r_plugin_info_t info;
    void print_info();

    bool opened;
    std::list<FreiorParameterPtr> parameters;

    void (*f0r_set_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index);
    void (*f0r_get_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index);

    f0r_instance_t (*f0r_construct)(unsigned int width, unsigned int height);

    // Interface function pointers.
    int (*f0r_init)();
    void (*f0r_get_plugin_info)(f0r_plugin_info_t* pluginInfo);
    void (*f0r_get_param_info)(f0r_param_info_t* info, int param_index);
    void (*f0r_destruct)(f0r_instance_t instance);

    void (*f0r_update)(f0r_instance_t instance, double time,
                       const uint32_t* inframe, uint32_t* outframe);
    void (*f0r_update2)(f0r_instance_t instance, double time,
                        const uint32_t* inframe1, const uint32_t* inframe2,
                        const uint32_t* inframe3, uint32_t* outframe);
                        
private:
    void init();
    // dlopen handle
    void *handle;
    // full .so file path
    char filename[512];

    FACTORY_ALLOWED
};

#endif // WITH_FREI0R

#endif
