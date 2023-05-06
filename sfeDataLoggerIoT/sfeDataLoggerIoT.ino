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

#define OPENLOG_ESP32
#ifdef OPENLOG_ESP32
#define EN_3V3_SW 32
#define LED_BUILTIN 25
#define LED_RGB_BUILTIN 26
#define BOOT_BUTTON 0
#endif


// Our data logger application

sfeDataLogger  theDataLogger;


// Button Things ...
#define kButtonNone    0
#define kButtonPress   1
#define kButtonRelease 2

static uint userButtonPressed = 0;
static uint userButtonEvent=kButtonNone;
uint32_t pressTime;

void userButtonISR(void)
{
    userButtonEvent = digitalRead(BOOT_BUTTON) == HIGH ? kButtonRelease : kButtonPress;
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

    // TESTING of button state/proessing
    // if (userButtonEvent != 0)
    // {
    //     Serial.printf("BUTTON EVENT %s\n\r", userButtonEvent == kButtonPress ? "Press" : "Relase");

    //     if (userButtonEvent == kButtonPress)
    //     {
    //         pressTime = millis();
    //         userButtonPressed = 1;
    //     }
    //     else
    //     { 
    //         dl_stopBlink();
    //         Serial.printf("Button pressed for %.2f seconds \n\r", (millis()-pressTime)/1000.);
    //         userButtonPressed = 0;
    //     }
    //     userButtonEvent=kButtonNone;
    // }
    // else if ( userButtonPressed)
    // {
    //     uint32_t delta = (millis() - pressTime)/1000;
    //     if ( userButtonPressed == 1 && delta > 10 )
    //     {
    //         userButtonPressed++;
    //         dl_startBlink(600);
    //     }
    //     else if (userButtonPressed == 2 && delta > 20)
    //     {
    //         userButtonPressed++;
    //         dl_startBlink(200);
    //     }
    //     else if (userButtonPressed == 3 && delta > 30)
    //     {
    //         userButtonPressed++;
    //         dl_stopBlink();
    //         dl_ledBusy(true);
    //     }else if (userButtonPressed == 4 && delta > 35)
    //     {
    //         Serial.printf("REBOOT!!");
    //     }

    // }

    delay(1);

}
