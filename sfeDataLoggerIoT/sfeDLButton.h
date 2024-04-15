/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.  All rights reserved.
 * This software includes information which is proprietary to and a
 * trade secret of SparkFun Electronics Inc.  It is not to be disclosed
 * to anyone outside of this organization. Reproduction by any means
 * whatsoever is  prohibited without express written permission.
 *
 *---------------------------------------------------------------------------------
 */
#pragma once

#include <cstdint>

#include <Flux/flxCore.h>
#include <Flux/flxCoreJobs.h>

// A class to encapsulate the event logic/handling of the on-board button of the DataLogger.
//
// Note: For button press events, the class will send out "increment" events if the button is pressed for a
// longer that "momentary" period

class sfeDLButton : public flxActionType<sfeDLButton>
{

  public:
    sfeDLButton() : _pressIncrement{5}, _userButtonPressed{false}, _pressEventTime{0}, _currentInc{0}
    {
        setName("DataLogger Button", "Manage DataLogger Button Events");
        setHidden();
    }

    bool initialize(void);

    // An Pressed event is sent after each increment value.
    void setPressIncrement(uint inc)
    {
        if (inc > 0)
            _pressIncrement = inc;
    }

    // Our events - signals ...
    flxSignalVoid on_momentaryPress;
    flxSignalUInt32 on_buttonRelease;
    flxSignalUInt32 on_buttonPressed;

  private:
    void checkButton(void);
    // How many seconds per increment on a button press
    uint32_t _pressIncrement;

    // button pressed?
    bool _userButtonPressed;

    // ticks when the button was pressed
    uint32_t _pressEventTime;

    // Ticks since last increment event
    uint32_t _incEventTime;

    // the current increment count
    uint16_t _currentInc;

    flxJob _jobCheckButton;
};