/*  FreeJ
 *  (c) Copyright 2010 Andrea Guzzo <xant@dyne.org>
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

#include <CVLayer.h>


CVCocoaLayer::CVCocoaLayer(Layer *lay, CVLayerController *vin)
{
    input = vin;
    blendMode = nil;

    layer = lay;
    if (input)
        [input setLayer:this];
    if (layer) {
        layer->set_name(input ? [input name] : "CVCocoaLayer");
        layer->set_data((void *)this); // XXX
    }
}

CVCocoaLayer::~CVCocoaLayer()
{
    deactivate();
    //if (input)
        //[input setLayer:nil];

}

void
CVCocoaLayer::activate()
{
    if (layer && !layer->active) {
        layer->opened = true;
        layer->active = true;
        layer->set_data((void *)this);
        notice("Activating %s", layer->name);
        layer->start();
    }
}

void
CVCocoaLayer::deactivate()
{
    if (layer) {
        if (layer->screen) {
            layer->screen->rem_layer(layer);
            layer->screen = NULL;
        }
        layer->active = false;
    }
}

bool CVCocoaLayer::isActive()
{
    if (layer)
        return layer->active;
    return false;
}

bool CVCocoaLayer::isVisible()
{
    if (layer && layer->screen)
        return true;
    return false;
}

Layer *CVCocoaLayer::fjLayer()
{
    return layer;
}

void CVCocoaLayer::setController(CVLayerController *vin)
{
    input = vin;
    if (input)
        [input setLayer:this];
}

char *CVCocoaLayer::fjName()
{
    if (layer)
        return layer->name;
    return (char *)"CVCocoaLayer";
}

int CVCocoaLayer::width()
{
	if (layer)
		return layer->geo.w;
	return 0;
}

int CVCocoaLayer::height()
{
	if (layer)
		return layer->geo.h;
	return 0;
}

void CVCocoaLayer::setOrigin(int x, int y)
{
	if (layer) {
		layer->geo.x = x;
		layer->geo.y = y;
	}
}

NSDictionary *CVCocoaLayer::imageParams() {
    if (input)
        return [input imageParams];
    return NULL;
}
