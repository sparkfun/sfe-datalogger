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
#pragma once

#include <cstdint>

#include <Flux/flxCore.h>



class sfeDLButton : public flxActionType<sfeDLButton>
{

public:

    sfeDLButton() : _pressIncrement{5}, _userButtonPressed{false}, _pressEventTime{0}, _currentInc{0}
    {
        setHidden();
    }

    bool initialize(void);
    bool loop(void);

    // An Pressed event is sent after each increment value.
    void setPressIncrement(uint inc)
    {
        if (inc > 0)
            _pressIncrement = inc;
    }


    flxSignalVoid on_momentaryPress;
    flxSignalUInt on_buttonRelease;
    flxSignalUInt on_buttonPressed;

private:

    uint32_t _pressIncrement;
    bool _userButtonPressed;
    uint32_t _pressEventTime;
    uint32_t _incEventTime;

    uint16_t _currentInc;
};