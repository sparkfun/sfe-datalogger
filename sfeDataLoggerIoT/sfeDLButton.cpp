/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023, SparkFun Electronics Inc.  All rights reserved.
 * This software includes information which is proprietary to and a
 * trade secret of SparkFun Electronics Inc.  It is not to be disclosed
 * to anyone outside of this organization. Reproduction by any means
 * whatsoever is  prohibited without express written permission.
 *
 *---------------------------------------------------------------------------------
 */

#include "sfeDLButton.h"


#define BOOT_BUTTON 0


#define kEventNone    0

// Button Event Types ...
#define kEventButtonPress   1
#define kEventButtonRelease 2

//---------------------------------------------------------------------------
// for the button ISR 
static uint userButtonEvent = kEventNone;

static void userButtonISR(void)
{
    userButtonEvent = digitalRead(BOOT_BUTTON) == HIGH ? kEventButtonRelease : kEventButtonPress;
}


//---------------------------------------------------------------------------

bool sfeDLButton::initialize(void)
{

    // setup our event handler
    // setup the button
    pinMode(BOOT_BUTTON, INPUT_PULLUP);
    attachInterrupt(BOOT_BUTTON, userButtonISR, CHANGE);

    userButtonEvent = kEventNone;

    return true;
}

//---------------------------------------------------------------------------
bool sfeDLButton::loop(void)
{
    // Button event / state change?
    if (userButtonEvent != kEventNone)
    {
        if (userButtonEvent == kEventButtonPress)
        {
            _currentInc = 0;
            _incEventTime = millis();
            _pressEventTime = _incEventTime;

            _userButtonPressed = true;

            on_buttonPressed.emit(0);
        }
        else
        {
            _userButtonPressed = false;
            if (millis() - _pressEventTime < 1001)
                on_momentaryPress.emit();
            else
                on_buttonRelease.emit(_currentInc);
        }
        userButtonEvent = kEventNone;
    }
    // Is the button pressed ? 
    else if (_userButtonPressed)
    {
        // go over the increment time?
        if( (millis() - _incEventTime) / 1000 >= _pressIncrement )
        {
            _currentInc++;
            on_buttonPressed.emit(_currentInc);
            _incEventTime = millis();
        }


    }

    return false;
}
