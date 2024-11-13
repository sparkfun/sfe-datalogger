/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

/*
 * Flux Framework based logger.
 *
 */

// Flux framework
#include "sfeDLBoard.h"
#include "sfeDataLogger.h"

#include "sfeDLLed.h"

// Our data logger application
sfeDataLogger theDataLogger;

//---------------------------------------------------------------------
// Arduino Setup
//
void setup()
{

    pinMode(kDLBoardEn3v3_SW, OUTPUT); // Enable Qwiic power and I2C
    digitalWrite(kDLBoardEn3v3_SW, HIGH);

    // Start up the framework
    flux.start();
}

//---------------------------------------------------------------------
// Arduino loop -
void loop()
{

    ///////////////////////////////////////////////////////////////////
    // Flux
    //
    // Just call the Flux framework loop() method. Flux will manage
    // the dispatch of processing to the components that were added
    // to the system during setup.
    if (flux.loop()) // will return true if an action did something
        sfeLED.flash(sfeLED.Blue);

    delay(1);
}
