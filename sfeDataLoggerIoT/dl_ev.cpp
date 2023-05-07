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

#include "dl_ev.h"
#include "dl_led.h"


static uint userButtonPressed = 0;

static uint32_t pressEventTime;

// Flash levels...
#define kFlashSlow 		600
#define kFlashMedium	200
#define kFlashFast		80

// our button hold stages - times in secs
#define kButtonState1 		10
#define kButtonState2		20
#define kButtonState3  		30
#define kButtonStateFinal	33

void dl_evButtonEvent(uint32_t userButtonEvent)
{
	// Did a button event happen?
	if (userButtonEvent != kEventNone)
	{
        if (userButtonEvent == kEventButtonPress)
        {
            pressEventTime = millis();
            userButtonPressed = 1;
        }
        else
        { 
            sfeLED.off();
            userButtonPressed = 0;
        }
    }

}

bool dl_evButtonReset(void)
{

    // has the button been pressed?
    if (userButtonPressed == 0)
        return false;


    bool retval = false;

    uint32_t delta = (millis() - pressEventTime) / 1000;

    if (userButtonPressed == 1 && delta > kButtonState1)
    {
        userButtonPressed++;
        sfeLED.blink(sfeLED.Yellow, kFlashSlow);
    }
    else if (userButtonPressed == 2 && delta > kButtonState2)
    {
        userButtonPressed++;
        sfeLED.blink(kFlashMedium);
    }
    else if (userButtonPressed == 3 && delta > kButtonState3)
    {
        userButtonPressed++;
        sfeLED.blink(kFlashFast);
    }
    else if (userButtonPressed == 4 && delta > kButtonStateFinal )
    {
        retval = true;
        sfeLED.stop();
        sfeLED.on(sfeLED.Red);
    }

    return retval;
}