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
 
/*
 * Spark Framework demo - logging
 *   
 */

// Spark framework 
#include "sfeDataLogger.h"

#include "dl_led.h"
#include "dl_ev.h"

#define OPENLOG_ESP32
#ifdef OPENLOG_ESP32
#define EN_3V3_SW 32
#define LED_BUILTIN 25
#define LED_RGB_BUILTIN 26
#define BOOT_BUTTON 0
#endif


// Our data logger application

sfeDataLogger  theDataLogger;

static bool userButtonPressed;
static uint userButtonEvent;

void userButtonISR(void)
{
    userButtonEvent = digitalRead(BOOT_BUTTON) == HIGH ? kEventButtonRelease : kEventButtonPress;
}



//---------------------------------------------------------------------
// Arduino Setup
//
void setup() {


    pinMode(EN_3V3_SW, OUTPUT); // Enable Qwiic power and I2C
    digitalWrite(EN_3V3_SW, HIGH);

    // Begin setup - turn on board LED during setup.
    pinMode(LED_RGB_BUILTIN, OUTPUT);

    // Start up the framework
    flux.start();

    // setup the button
    pinMode(BOOT_BUTTON, INPUT_PULLUP);
    attachInterrupt(BOOT_BUTTON, userButtonISR, CHANGE);

    userButtonPressed = false;
    userButtonEvent = kEventNone;

}

//---------------------------------------------------------------------
// Arduino loop - 
void loop() {

    ///////////////////////////////////////////////////////////////////
    // Flux
    //
    // Just call the Flux framework loop() method. Flux will manage
    // the dispatch of processing to the components that were added 
    // to the system during setup.
    if(flux.loop() && theDataLogger.ledEnabled == true)        // will return true if an action did something
        sfeLED.flash(sfeLED.Blue);

    // user button event?
    if (userButtonEvent != kEventNone)
    {
        dl_evButtonEvent(userButtonEvent);
        userButtonPressed = (userButtonEvent == kEventButtonPress);
        userButtonEvent = kEventNone;
    }
    // In a press event
    else if (userButtonPressed)
    {
        // Reset the system?
        if (dl_evButtonReset())
            theDataLogger.resetDevice();
    }

    delay(1);

}
