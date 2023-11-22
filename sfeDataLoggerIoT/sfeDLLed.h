
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

// This are Handy Flash levels...
#define kLEDFlashSlow 600
#define kLEDFlashMedium 200
#define kLEDFlashFast 80

typedef uint32_t sfeLEDColor_t;
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

    // colors
    static constexpr sfeLEDColor_t Black = 0x000000;
    static constexpr sfeLEDColor_t Blue = 0x0000FF;
    static constexpr sfeLEDColor_t Green = 0x008000;
    static constexpr sfeLEDColor_t Yellow = 0xFFFF00;
    static constexpr sfeLEDColor_t Red = 0xFF0000;
    static constexpr sfeLEDColor_t Gray = 0x808080;
    static constexpr sfeLEDColor_t LightGray = 0x778899;
    static constexpr sfeLEDColor_t Orange = 0xFFA500;
    static constexpr sfeLEDColor_t White = 0xFFFFFF;
    static constexpr sfeLEDColor_t Purple = 0x80008;

    bool initialize(void);
    void on(sfeLEDColor_t color);
    void off(void);
    void blink(uint32_t);
    void blink(sfeLEDColor_t, uint32_t);
    void stop(bool off = true);
    void flash(sfeLEDColor_t color);

    void _timerCB(void);
    void _eventCB(uint32_t);

    void setDisabled(bool bDisable);

    bool disabled(void)
    {
        return _disabled;
    }

  private:
    _sfeLED();

    void popState(void);
    bool pushState(sfeLEDColor_t);
    void update(void);
    void start_blink(void);
    void stop_blink(void);

    typedef struct
    {
        uint32_t color;
        uint32_t ticks;
    } colorState_t;

    static constexpr uint16_t kStackSize = 10;
    colorState_t _colorStack[kStackSize];

    int _current;
    bool _isInitialized;

    bool _blinkOn;

    CRGB _theLED;

    bool _disabled;
};
extern _sfeLED &sfeLED;