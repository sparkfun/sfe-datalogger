/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "sfeDLButton.h"
#include "sfeDLBoard.h"

const uint8_t kEventNone = 0;

// Button Event Types ...
const uint8_t kEventButtonPress = 1;
const uint8_t kEventButtonRelease = 2;

//---------------------------------------------------------------------------
// for the button ISR
static uint userButtonEvent = kEventNone;

static void userButtonISR(void)
{
    userButtonEvent = digitalRead(kDLBoardBootButton) == HIGH ? kEventButtonRelease : kEventButtonPress;
}

//---------------------------------------------------------------------------

bool sfeDLButton::initialize(void)
{

    // setup our event handler
    // setup the button
    pinMode(kDLBoardBootButton, INPUT_PULLUP);
    attachInterrupt(kDLBoardBootButton, userButtonISR, CHANGE);

    userButtonEvent = kEventNone;

    // job/timer for when we should check button state
    _jobCheckButton.setup("buttoncheck", 300, this, &sfeDLButton::checkButton);
    flxAddJobToQueue(_jobCheckButton);

    return true;
}

//---------------------------------------------------------------------------
void sfeDLButton::checkButton(void)
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
        if ((millis() - _incEventTime) / 1000 >= _pressIncrement)
        {
            _currentInc++;
            on_buttonPressed.emit(_currentInc);
            _incEventTime = millis();
        }
    }
}
