/*
 *  CKbdController.cpp
 *  freej
 *
 *  Created by xant on 8/28/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "CKbdController.h"

FACTORY_REGISTER_INSTANTIATOR(Controller, CKbdController, KeyboardController, cocoa);

CKbdController::CKbdController() 
    : Controller()
{
}

CKbdController::~CKbdController()
{
}

int CKbdController::dispatch()
{
    return 0;
}

int CKbdController::poll()
{
    return 0;
}
