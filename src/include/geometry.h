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

#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <inttypes.h>

/**
   This class is made to hold informations about the geometry of
   various video components in FreeJ, describing their format and
   image bounds.

   @brief Collects geometrical information about Layer, Screen and other components
*/
class Geometry {

public:

    Geometry();
    ~Geometry();

    void init(int nw, int nh, int nbpp);

    int16_t x; ///< x axis position coordinate
    int16_t y; ///< y axis position coordinate
    uint16_t w; ///< width of frame in pixels
    uint16_t h; ///< height of frame in pixels
    uint8_t bpp; ///< bits per pixel
    uint32_t pixelsize; ///< size of the whole frame in pixels
    uint32_t bytesize; ///< size of the whole frame in bytes
    uint16_t bytewidth; ///< width of frame in bytes (also called pitch or stride)

};

#endif /* __GEOMETRY_H__ */

