/*  FreeJ
 *  (c) Copyright 2008 Denis Roio aka jaromil <jaromil@dyne.org>
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

#ifndef __FREEJ_AUDIO_H__
#define __FREEJ_AUDIO_H__

#include <sys/types.h>

#include "context.h"

FREEJ_FORWARD_PTR(AudioLayer)
class AudioLayer : public Layer {

public:
    AudioLayer();
    virtual ~AudioLayer();

    bool open(const char *devfile);


    void *feed(double time);
    void close();

protected:
    bool _init() {
        return true;
    };

private:

    void AudioCallback_i(unsigned int);
    static void AudioCallback(void *, unsigned int);

    float *m_JackBuffer;
};

#endif
