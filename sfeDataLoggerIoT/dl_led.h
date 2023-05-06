
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

// LED Display for the datalogger - led display in a task
#pragma once

// quiet a damn pragma message - silly
#define FASTLED_INTERNAL
#include <FastLED.h>

//---------------------------------------------------------------
class _sfeLED
{
  public:
    // this is a singleton
    static _sfeLED &get(void)
    {
        static _sfeLED instance;
        return instance;
    }

    //-------------------------------------------------------------------------
    // Delete copy and assignment constructors - b/c this is singleton.
    _sfeLED(_sfeLED const &) = delete;
    void operator=(_sfeLED const &) = delete;

    typedef enum
    {
        Black = 0x000000,
        Blue = 0x0000FF,
        Green = 0x008000,
        Yellow = 0xFFFF00,
        Red = 0xFF0000,
        Gray = 0x808080,
        LightGray = 0x778899,
        Orange = 0xFFA500,
        White = 0xFFFFFF,
        Purple = 0x800080
    } LEDColor_t;

    bool initialize(void);
    void on(_sfeLED::LEDColor_t color);
    void off(void);
    void blink(uint32_t);
    void stopBlink(bool off = true);
    void flash(_sfeLED::LEDColor_t color);

    void _timerCB(void);
    void _eventCB(uint32_t);

  private:
    _sfeLED();

    void update(void);
    void start_blink(void);
    void stop_blink(void);

    typedef struct
    {
        uint32_t color;
        uint32_t ticks;
    } colorState_t;

    static constexpr uint16_t kStackSize = 6;
    colorState_t _colorStack[kStackSize];

    int _current;
    bool _isInitialized;

    bool _blinkOn;

    CRGB _theLED;
};
extern _sfeLED &sfeLED;