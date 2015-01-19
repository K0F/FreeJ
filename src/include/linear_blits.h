/*  C++ linear_blits
 *
 *  (c) Copyright 2014 Yann Diorcet <diorcet.yann@gmail.com>
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

#ifndef __linear_blits_h__
#define __linear_blits_h__

#include "blit_instance.h"

LinkList<Blit> &get_linear_blits();


typedef void (*linear_fct)(void *, void *, int , LinkList<ParameterInstance> &);

FREEJ_FORWARD_PTR(LinearBlit)
class LinearBlit: public Blit {
    friend class AbstractLinearBlitInstance;
public:
    LinearBlit(const std::string &name, const std::string &description, linear_fct fct, LinkList<Parameter> &parameters);
    LinearBlit(const std::string &name, const std::string &description, linear_fct fct, LinkList<Parameter> &&parameters = LinkList<Parameter>());
    ~LinearBlit();

protected:
    linear_fct fct;

};

class AbstractLinearBlitInstance: public BlitInstance {

public:
    virtual void operator()(LayerPtr layer);
    virtual void init(LinearBlitPtr blit);

protected:
    AbstractLinearBlitInstance(LinearBlitPtr proto);
    virtual void* getSurface() = 0;
    virtual ViewPortPtr getScreen() = 0;

private:
    LinearBlitPtr proto;
    void crop(LayerPtr lay, ViewPortPtr scr);

    int32_t scr_stride_dx;
    int32_t scr_stride_sx;
    int32_t scr_stride_up;
    int32_t scr_stride;
    uint32_t scr_offset;

    int32_t lay_pitch;
    int32_t lay_bytepitch;
    int32_t lay_stride;
    int32_t lay_stride_sx;
    int32_t lay_stride_dx;
    int32_t lay_stride_up;
    int32_t lay_height;
    uint32_t lay_offset;
};


#endif //__linear_blits_h__
